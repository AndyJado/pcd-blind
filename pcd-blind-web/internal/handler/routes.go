package handler

import (
	"net/http"

	"github.com/go-chi/chi/v5"
	"github.com/go-chi/chi/v5/middleware"
)

func (h *Handler) BuildRoutes() http.Handler {
	r := chi.NewRouter()
	r.Use(middleware.Logger)
	r.Use(middleware.Recoverer)

	// Static files (CSS etc)
	fileServer := http.FileServer(http.Dir("web/static"))
	r.Handle("/static/*", http.StripPrefix("/static/", fileServer))

	// Home page
	r.Get("/", h.Index)

	// Tab pages
	r.Get("/tab/params", h.TabParams)
	r.Get("/tab/files", h.TabFiles)
	r.Get("/tab/results", h.TabResults)
	r.Get("/tab/history", h.TabHistory)

	// Upload
	r.Post("/upload", h.UploadFile)
	r.Delete("/source/{name}", h.DeleteSource)
	r.Get("/source-list", h.SourceList)

	// Params
	r.Patch("/params", h.UpdateParams)

	// Runs
	r.Post("/runs", h.StartRun)
	r.Delete("/runs/{runID}", h.DeleteRun)
	r.Get("/run-list", h.RunList)
	r.Get("/runs/{runID}/status", h.RunStatus)
	r.Get("/runs/{runID}/view", h.RunView)

	// Screenshots
	r.Get("/runs/{runID}/screenshot/{file}", h.ServeScreenshot)

	// Selection & export
	r.Post("/runs/{runID}/select", h.SelectCandidate)
	r.Get("/runs/{runID}/export", h.ExportSelected)

	return r
}
