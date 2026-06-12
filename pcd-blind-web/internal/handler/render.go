package handler

import (
	"html/template"
	"io/fs"
	"net/http"
	"strings"
)

var tmpl *template.Template

// InitTemplatesFS loads templates from an embedded filesystem.
func InitTemplatesFS(tfs fs.FS, pattern string) error {
	tmpl = template.New("").Funcs(template.FuncMap{
		"add": func(a, b int) int { return a + b },
		"countSelected": func(sel any) int {
			n := 0
			if m, ok := sel.(map[int]bool); ok {
				for _, v := range m {
					if v {
						n++
					}
				}
			}
			return n
		},
		"tagClass": func(val, lo, hi, good float64) string {
			if val < lo {
				return "tag-bad"
			} else if val < hi {
				return "tag-warn"
			} else if val >= good {
				return "tag-good"
			}
			return "tag-warn"
		},
	})

	_, err := tmpl.ParseFS(tfs, pattern)
	return err
}

// render executes the full page template.
func render(w http.ResponseWriter, name string, data any) {
	var buf strings.Builder
	err := tmpl.ExecuteTemplate(&buf, name, data)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	w.Header().Set("Content-Type", "text/html; charset=utf-8")
	w.Write([]byte(buf.String()))
}

// renderPartial executes a named template as an HTMX fragment.
func renderPartial(w http.ResponseWriter, name string, data any) {
	w.Header().Set("Content-Type", "text/html; charset=utf-8")
	if err := tmpl.ExecuteTemplate(w, name, data); err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
	}
}
