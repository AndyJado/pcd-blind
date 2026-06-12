package handler

import (
	"fmt"
	"net/http"
	"strconv"
	"sync"
	"time"

	"pcd-blind-web/internal/config"
	"pcd-blind-web/internal/engine"
	"pcd-blind-web/internal/store"
)

type RunState struct {
	LogLines []string
	Done     bool
	Result   engine.RunResult
	mu       sync.Mutex
}

var (
	runStates   = make(map[string]*RunState)
	runStatesMu sync.Mutex
)

func getOrCreateState(runID string) *RunState {
	runStatesMu.Lock()
	defer runStatesMu.Unlock()
	if s, ok := runStates[runID]; ok {
		return s
	}
	s := &RunState{}
	runStates[runID] = s
	return s
}

// StartRun launches a new pipeline run.
func (h *Handler) StartRun(w http.ResponseWriter, r *http.Request) {
	r.ParseForm()
	sourceFile := r.FormValue("source")
	if sourceFile == "" {
		http.Error(w, "未选择 PCD 文件", http.StatusBadRequest)
		return
	}

	// Merge form values into current params
	params := h.mergeFormParams(h.Store.Params(), r)
	runner := engine.NewRunner(h.BinPath, h.Store.DataDir())

	state := &RunState{}
	runID, err := runner.Start(sourceFile, params,
		func(line string) {
			state.mu.Lock()
			state.LogLines = append(state.LogLines, line)
			if len(state.LogLines) > 50 {
				state.LogLines = state.LogLines[len(state.LogLines)-50:]
			}
			state.mu.Unlock()
		},
		func(result engine.RunResult) {
			// Update persistent store FIRST, then in-memory state
			h.Store.UpdateRun(result.RunID, func(rec *store.RunRecord) {
				if result.Success {
					rec.Status = "done"
					rec.NCandidates = result.NCandidates
					rec.NResults = result.NResults
				} else {
					rec.Status = "error"
					rec.Error = result.Error
				}
			})
			state.mu.Lock()
			state.Done = true
			state.Result = result
			state.mu.Unlock()
		},
	)

	if err != nil {
		http.Error(w, "启动失败: "+err.Error(), http.StatusInternalServerError)
		return
	}

	h.Store.AddRun(store.RunRecord{
		ID:         runID,
		SourceFile: sourceFile,
		Status:     "running",
	})

	runStatesMu.Lock()
	runStates[runID] = state
	runStatesMu.Unlock()

	// Return the results tab content with running status
	h.renderResultsTab(w, r, runID)
}

// RunStatus returns SSE stream for a running pipeline.
func (h *Handler) RunStatus(w http.ResponseWriter, r *http.Request) {
	runID := r.PathValue("runID")

	flusher, ok := w.(http.Flusher)
	if !ok {
		http.Error(w, "streaming not supported", http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Type", "text/event-stream")
	w.Header().Set("Cache-Control", "no-cache")
	w.Header().Set("Connection", "keep-alive")

	runStatesMu.Lock()
	state, ok := runStates[runID]
	runStatesMu.Unlock()

	if !ok {
		// Run state not in memory (server restarted). Check persistent store.
		run := h.Store.GetRun(runID)
		if run == nil {
			fmt.Fprintf(w, "data: {\"done\": true, \"error\": \"未找到运行记录\"}\n\n")
			flusher.Flush()
			return
		}
		if run.Status == "done" {
			fmt.Fprintf(w, "data: {\"done\": true, \"success\": true, \"n_results\": %d}\n\n", run.NResults)
			flusher.Flush()
			return
		}
		if run.Status == "error" {
			fmt.Fprintf(w, "data: {\"done\": true, \"success\": false, \"error\": \"%s\"}\n\n", escapeJSON(run.Error))
			flusher.Flush()
			return
		}
		// Status is "running" but no in-memory state — keep polling store
		h.pollStoreForSSE(w, r, runID)
		return
	}

	lastLineCount := 0
	ticker := time.NewTicker(300 * time.Millisecond)
	defer ticker.Stop()

	for {
		select {
		case <-r.Context().Done():
			return
		case <-ticker.C:
		}

		state.mu.Lock()
		done := state.Done
		lines := make([]string, len(state.LogLines))
		copy(lines, state.LogLines)
		result := state.Result
		state.mu.Unlock()

		for i := lastLineCount; i < len(lines); i++ {
			escaped := escapeJSON(lines[i])
			fmt.Fprintf(w, "data: {\"line\": %s}\n\n", escaped)
		}
		lastLineCount = len(lines)

		if done {
			if result.Success {
				fmt.Fprintf(w, "data: {\"done\": true, \"success\": true, \"n_results\": %d}\n\n", result.NResults)
			} else {
				fmt.Fprintf(w, "data: {\"done\": true, \"success\": false, \"error\": %s}\n\n", escapeJSON(result.Error))
			}
			flusher.Flush()
			// Sleep briefly so the browser JS has time to close the EventSource
			time.Sleep(500 * time.Millisecond)
			return
		}
		flusher.Flush()
	}
}

// pollStoreForSSE polls the persistent store when no in-memory run state exists.
func (h *Handler) pollStoreForSSE(w http.ResponseWriter, r *http.Request, runID string) {
	flusher, _ := w.(http.Flusher)
	ticker := time.NewTicker(1 * time.Second)
	defer ticker.Stop()

	for {
		select {
		case <-r.Context().Done():
			return
		case <-ticker.C:
		}

		run := h.Store.GetRun(runID)
		if run == nil {
			fmt.Fprintf(w, "data: {\"done\": true, \"error\": \"记录丢失\"}\n\n")
			flusher.Flush()
			return
		}
		if run.Status == "done" {
			fmt.Fprintf(w, "data: {\"done\": true, \"success\": true, \"n_results\": %d}\n\n", run.NResults)
			flusher.Flush()
			return
		}
		if run.Status == "error" {
			fmt.Fprintf(w, "data: {\"done\": true, \"success\": false, \"error\": \"%s\"}\n\n", escapeJSON(run.Error))
			flusher.Flush()
			return
		}
		// Still running, send heartbeat to keep connection alive
		fmt.Fprintf(w, "data: {\"line\": \"等待管线完成...\"}\n\n")
		flusher.Flush()
	}
}

// mergeFormParams overrides store params with values from an HTTP form.
func (h *Handler) mergeFormParams(p config.Params, r *http.Request) config.Params {
	getf := func(key string) *float64 {
		v := r.FormValue(key)
		if v == "" {
			return nil
		}
		f, err := strconv.ParseFloat(v, 64)
		if err != nil {
			return nil
		}
		return &f
	}
	geti := func(key string) *int {
		v := r.FormValue(key)
		if v == "" {
			return nil
		}
		i, err := strconv.Atoi(v)
		if err != nil {
			return nil
		}
		return &i
	}

	if v := getf("pct"); v != nil { p.Pct = *v }
	if v := getf("z_start"); v != nil { p.ZStart = *v }
	if v := getf("z_step"); v != nil { p.ZStep = *v }
	if v := getf("z_end"); v != nil { p.ZEnd = *v }
	if v := getf("cluster_tolerance"); v != nil { p.ClusterTol = *v }
	if v := geti("min_cluster_size"); v != nil { p.MinClusterSize = *v }
	if v := getf("cyl_radius"); v != nil { p.CylRadius = *v }
	if v := getf("box_dz_up"); v != nil { p.BoxDzUp = *v }
	if v := getf("box_dz_dn"); v != nil { p.BoxDzDn = *v }
	if v := getf("ground_z_pct"); v != nil { p.GroundZPct = *v }
	if v := getf("wall_nz_max"); v != nil { p.WallNzMax = *v }
	if v := geti("max_iters"); v != nil { p.MaxIters = *v }
	if v := getf("dxy_threshold"); v != nil { p.DxyThreshold = *v }
	if v := getf("ball_diameter"); v != nil { p.BallDiameter = *v }

	h.Store.SetParams(p)
	return p
}

func escapeJSON(s string) string {
	result := "\""
	for _, c := range s {
		switch c {
		case '"':
			result += "\\\""
		case '\\':
			result += "\\\\"
		case '\n':
			result += "\\n"
		case '\r':
			result += "\\r"
		case '\t':
			result += "\\t"
		default:
			if c < 0x20 {
				result += fmt.Sprintf("\\u%04x", c)
			} else {
				result += string(c)
			}
		}
	}
	result += "\""
	return result
}

// DeleteRun removes a run.
func (h *Handler) DeleteRun(w http.ResponseWriter, r *http.Request) {
	runID := r.PathValue("runID")
	h.Store.DeleteRun(runID)

	runStatesMu.Lock()
	delete(runStates, runID)
	runStatesMu.Unlock()

	// Return updated run list
	runs := h.Store.Runs()
	renderPartial(w, "run-list", map[string]any{"Runs": runs})
}

// RunList renders run history.
func (h *Handler) RunList(w http.ResponseWriter, r *http.Request) {
	runs := h.Store.Runs()
	renderPartial(w, "run-list", map[string]any{"Runs": runs})
}
