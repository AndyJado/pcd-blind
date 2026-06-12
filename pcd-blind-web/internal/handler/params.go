package handler

import (
	"encoding/json"
	"net/http"

	"pcd-blind-web/internal/config"
)

// UpdateParams handles PATCH /params.
func (h *Handler) UpdateParams(w http.ResponseWriter, r *http.Request) {
	var p config.Params
	if err := json.NewDecoder(r.Body).Decode(&p); err != nil {
		http.Error(w, "invalid JSON", http.StatusBadRequest)
		return
	}
	if p.Pct < 1 || p.Pct > 100 {
		p = h.Store.Params()
	}
	if p.ZStart <= p.ZEnd {
		p = h.Store.Params()
	}
	h.Store.SetParams(p)
	w.Write([]byte("ok"))
}
