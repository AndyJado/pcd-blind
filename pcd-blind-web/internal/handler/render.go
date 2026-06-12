package handler

import (
	"html/template"
	"net/http"
	"path/filepath"
	"strings"
)

var tmpl *template.Template

// InitTemplates loads templates from the given directory.
func InitTemplates(templateDir string) error {
	tmpl = template.New("").Funcs(template.FuncMap{
		"add": func(a, b int) int { return a + b },
		"sub": func(a, b int) int { return a - b },
		"dict": func(values ...any) map[string]any {
			d := make(map[string]any, len(values)/2)
			for i := 0; i+1 < len(values); i += 2 {
				key, ok := values[i].(string)
				if ok {
					d[key] = values[i+1]
				}
			}
			return d
		},
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

	pattern := filepath.Join(templateDir, "*.html")
	_, err := tmpl.ParseGlob(pattern)
	return err
}

// render executes the full page template (wrapping in base).
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
