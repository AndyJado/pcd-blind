package handler

import (
	"fmt"
	"net/http"
	"path/filepath"
	"strconv"

	"pcd-blind-web/internal/config"
	"pcd-blind-web/internal/engine"
)

// DetectionCard holds data for one detection card in the image gallery.
type DetectionCard struct {
	Idx            int
	Rank           int
	Cx, Cy, Cz     float64
	Ratio          float64
	Compact        float64
	Tripod         float64
	Dxy            float64
	HasScreenshot  bool
	ScreenshotFile string
}

// loadCandidatesForDisplay loads and gates candidates from a run's output.
func loadCandidatesForDisplay(runDir string, ratioMin, compactMin, dxyMax float64, p config.Params) ([]engine.CandidateRow, error) {
	candidates, err := engine.LoadCandidates(runDir)
	if err != nil {
		return nil, err
	}
	// Override gate with requested values
	p.RatioMin = ratioMin
	p.CompactMin = compactMin
	p.DxyMax = dxyMax
	return engine.GateAndScore(candidates, p), nil
}

// SelectCandidate toggles approval of a candidate.
func (h *Handler) SelectCandidate(w http.ResponseWriter, r *http.Request) {
	runID := r.PathValue("runID")
	r.ParseForm()
	idxStr := r.FormValue("idx")
	checked := r.FormValue("checked") == "true"

	idx, err := strconv.Atoi(idxStr)
	if err != nil {
		http.Error(w, "bad idx", http.StatusBadRequest)
		return
	}

	sel := h.Store.GetSelection(runID)
	selSet := make(map[int]bool)
	for _, s := range sel {
		selSet[s] = true
	}

	if checked {
		selSet[idx] = true
	} else {
		delete(selSet, idx)
	}

	newSel := make([]int, 0, len(selSet))
	for k := range selSet {
		newSel = append(newSel, k)
	}
	h.Store.SetSelection(runID, newSel)

	// Return approved count
	n := len(newSel)
	w.Header().Set("Content-Type", "text/html")
	if n > 0 {
		fmt.Fprintf(w, `<span style="font-size:12px;color:var(--green)">已确认 %d 个</span>
<a href="/runs/%s/export" class="btn btn-primary" download>📥 导出已确认</a>`, n, runID)
	} else {
		fmt.Fprint(w, `<span style="font-size:12px;color:var(--muted)">未确认任何靶球</span>`)
	}
}

// ExportSelected returns CSV of approved candidates.
func (h *Handler) ExportSelected(w http.ResponseWriter, r *http.Request) {
	runID := r.PathValue("runID")
	run := h.Store.GetRun(runID)
	if run == nil || run.Status != "done" {
		http.Error(w, "run not found", http.StatusNotFound)
		return
	}

	sel := h.Store.GetSelection(runID)
	if len(sel) == 0 {
		http.Error(w, "no selections", http.StatusBadRequest)
		return
	}

	selSet := make(map[int]bool)
	for _, s := range sel {
		selSet[s] = true
	}

	runDir := filepath.Join(h.Store.DataDir(), "output", runID)
	p := h.Store.Params()
	candidates, err := engine.LoadAndFilter(runDir, p)
	if err != nil {
		http.Error(w, "load failed", http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Type", "text/csv; charset=utf-8")
	w.Header().Set("Content-Disposition", fmt.Sprintf("attachment; filename=selected_%s.csv", runID))
	fmt.Fprintf(w, "idx,ratio,compact,tripod,dxy,ground_frac,cx,cy,cz\n")

	for _, c := range candidates {
		if selSet[c.Idx] {
			fmt.Fprintf(w, "%d,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f\n",
				c.Idx, c.Ratio, c.Compact, c.Tripod, c.Dxy, c.GroundFraction,
				c.Cx, c.Cy, c.Cz)
		}
	}
}

func atof(s string) float64 {
	v, _ := strconv.ParseFloat(s, 64)
	return v
}
