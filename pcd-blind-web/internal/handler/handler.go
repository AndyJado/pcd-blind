package handler

import (
	"fmt"
	"io"
	"net/http"
	"os"
	"path/filepath"
	"strings"

	"pcd-blind-web/internal/store"
)

// Handler holds dependencies for HTTP handlers.
type Handler struct {
	Store   *store.Store
	BinPath string
}

// New creates a Handler.
func New(s *store.Store, binPath string) *Handler {
	return &Handler{Store: s, BinPath: binPath}
}

// ── Full page (initial load) ──

func (h *Handler) Index(w http.ResponseWriter, r *http.Request) {
	render(w, "base", map[string]any{
		"ActiveTab": "params",
		"Params":    h.Store.Params(),
	})
}

// ── Tab pages ──

func (h *Handler) TabParams(w http.ResponseWriter, r *http.Request) {
	renderPartial(w, "params-body", map[string]any{
		"Params": h.Store.Params(),
	})
}

func (h *Handler) TabFiles(w http.ResponseWriter, r *http.Request) {
	files := listSourceFiles(h.Store.DataDir())
	renderPartial(w, "files-body", map[string]any{
		"Files":  files,
		"Params": h.Store.Params(),
	})
}

func (h *Handler) TabResults(w http.ResponseWriter, r *http.Request) {
	runID := r.URL.Query().Get("run")
	if runID == "" {
		runs := h.Store.Runs()
		for _, run := range runs {
			if run.Status == "done" || run.Status == "running" {
				runID = run.ID
				break
			}
		}
	}

	if runID == "" {
		renderPartial(w, "results-body", map[string]any{})
		return
	}

	h.renderResultsTab(w, r, runID)
}

func (h *Handler) TabHistory(w http.ResponseWriter, r *http.Request) {
	runs := h.Store.Runs()
	renderPartial(w, "history-body", map[string]any{
		"Runs": runs,
	})
}

// ── Run view (click from history) ──

func (h *Handler) RunView(w http.ResponseWriter, r *http.Request) {
	runID := r.PathValue("runID")
	h.renderResultsTab(w, r, runID)
}

func (h *Handler) renderResultsTab(w http.ResponseWriter, r *http.Request, runID string) {
	run := h.Store.GetRun(runID)
	if run == nil {
		renderPartial(w, "content", map[string]any{})
		return
	}

	p := h.Store.Params()

	// Parse filter overrides
	ratioMin := p.RatioMin
	compactMin := p.CompactMin
	dxyMax := p.DxyMax
	if v := r.URL.Query().Get("ratio_min"); v != "" {
		ratioMin = atof(v)
	}
	if v := r.URL.Query().Get("compact_min"); v != "" {
		compactMin = atof(v)
	}
	if v := r.URL.Query().Get("dxy_max"); v != "" {
		dxyMax = atof(v)
	}

	// Build detection list
	var detections []DetectionCard
	totalResults := 0

	if run.Status == "done" {
		runDir := filepath.Join(h.Store.DataDir(), "output", runID)
		candidates, _ := loadCandidatesForDisplay(runDir, ratioMin, compactMin, dxyMax, p)
		selections := h.Store.GetSelection(runID)
		approvedSet := make(map[int]bool)
		for _, s := range selections {
			approvedSet[s] = true
		}

		screenshotDir := filepath.Join(runDir, "screenshots")

		for i, c := range candidates {
			// Check if screenshot exists
			ssFile := ""
			hasSS := false
			for _, fn := range []string{
				filepath.Join(screenshotDir, fmt.Sprintf("%02d", i) + ".png"),
				filepath.Join(screenshotDir, fmt.Sprintf("detection_%02d", i) + ".png"),
			} {
				for _, ext := range []string{".png"} {
					tryPath := fn
					if !strings.HasSuffix(tryPath, ext) {
						continue
					}
					if _, err := os.Stat(tryPath); err == nil {
						ssFile = filepath.Base(tryPath)
						hasSS = true
						break
					}
				}
				if hasSS {
					break
				}
			}

			detections = append(detections, DetectionCard{
				Idx:            c.Idx,
				Rank:           i,
				Cx:             c.Cx,
				Cy:             c.Cy,
				Cz:             c.Cz,
				Ratio:          c.Ratio,
				Compact:        c.Compact,
				Tripod:         c.Tripod,
				Dxy:            c.Dxy,
				HasScreenshot:  hasSS,
				ScreenshotFile: ssFile,
			})
		}
		totalResults = len(candidates) // after gating
	} else {
		// Still running - just get count from candidates
		runDir := filepath.Join(h.Store.DataDir(), "output", runID)
		candPath := filepath.Join(runDir, "candidates.csv")
		if fi, err := os.Stat(candPath); err == nil && fi.Size() > 0 {
			totalResults = 0 // will update when done
		}
	}

	// Compute approved count
	selections := h.Store.GetSelection(runID)
	hasApproved := len(selections) > 0

	data := map[string]any{
		"ActiveRunID":   runID,
		"RunStatus":     run.Status,
		"RunError":      run.Error,
		"TotalResults":  totalResults,
		"Detections":    detections,
		"FilterRatio":   ratioMin,
		"FilterCompact": compactMin,
		"FilterDxy":     dxyMax,
		"HasApproved":   hasApproved,
		"ApprovedSet":   buildApprovedSet(selections),
	}
	renderPartial(w, "results-body", data)
}

// ── Screenshot serving ──

func (h *Handler) ServeScreenshot(w http.ResponseWriter, r *http.Request) {
	runID := r.PathValue("runID")
	file := r.PathValue("file")

	screenshotPath := filepath.Join(h.Store.DataDir(), "output", runID, "screenshots", file)
	http.ServeFile(w, r, screenshotPath)
}

// ── Upload ──

func (h *Handler) UploadFile(w http.ResponseWriter, r *http.Request) {
	r.ParseMultipartForm(100 << 20)
	f, header, err := r.FormFile("pcd_file")
	if err != nil {
		renderPartial(w, "upload-area", map[string]any{
			"Error":  "未选择文件",
			"Params": h.Store.Params(),
		})
		return
	}
	defer f.Close()

	name := strings.TrimSuffix(header.Filename, ".pcd") + ".pcd"
	name = sanitizeFilename(name)

	dst, err := os.Create(filepath.Join(h.Store.DataDir(), "source", name))
	if err != nil {
		renderPartial(w, "upload-area", map[string]any{
			"Error": "创建文件失败: " + err.Error(),
		})
		return
	}
	defer dst.Close()

	io.Copy(dst, f)

	files := listSourceFiles(h.Store.DataDir())
	renderPartial(w, "upload-area", map[string]any{
		"Files": files,
		"OK":    "已上传: " + name,
	})
}

func (h *Handler) DeleteSource(w http.ResponseWriter, r *http.Request) {
	name := r.PathValue("name")
	os.Remove(filepath.Join(h.Store.DataDir(), "source", name))
	files := listSourceFiles(h.Store.DataDir())
	if len(files) == 0 {
		renderPartial(w, "source-list", nil)
	} else {
		renderPartial(w, "source-list", files)
	}
}

func (h *Handler) SourceList(w http.ResponseWriter, r *http.Request) {
	files := listSourceFiles(h.Store.DataDir())
	if len(files) == 0 {
		renderPartial(w, "source-list", nil)
	} else {
		renderPartial(w, "source-list", files)
	}
}

// ── Helpers ──

func listSourceFiles(dataDir string) []string {
	dir := filepath.Join(dataDir, "source")
	entries, err := os.ReadDir(dir)
	if err != nil {
		return nil
	}
	var files []string
	for _, e := range entries {
		if !e.IsDir() && strings.HasSuffix(e.Name(), ".pcd") {
			files = append(files, e.Name())
		}
	}
	return files
}

func sanitizeFilename(name string) string {
	return strings.Map(func(r rune) rune {
		if r == '/' || r == '\\' || r == ':' || r == '*' || r == '?' || r == '"' || r == '<' || r == '>' || r == '|' {
			return '_'
		}
		return r
	}, name)
}

func buildApprovedSet(indices []int) map[int]bool {
	m := make(map[int]bool, len(indices))
	for _, i := range indices {
		m[i] = true
	}
	return m
}
