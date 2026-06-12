package store

import (
	"encoding/json"
	"os"
	"path/filepath"
	"sync"
	"time"

	"pcd-blind-web/internal/config"
)

// RunRecord describes one pipeline run.
type RunRecord struct {
	ID         string    `json:"id"`
	SourceFile string    `json:"source_file"`
	Status     string    `json:"status"` // "running", "done", "error"
	Error      string    `json:"error,omitempty"`
	CreatedAt  time.Time `json:"created_at"`
	NCandidates int     `json:"n_candidates"`
	NResults    int     `json:"n_results"`
	LogLine     string   `json:"log_line,omitempty"` // last log line for progress
}

// Store manages persistent state.
type Store struct {
	mu         sync.RWMutex
	dataDir    string
	params     config.Params
	runs       []RunRecord
	selections map[string][]int // runID → selected candidate indices
}

// New creates or loads a Store.
func New(dataDir string) *Store {
	s := &Store{
		dataDir:    dataDir,
		selections: make(map[string][]int),
	}

	// Ensure directories exist
	os.MkdirAll(filepath.Join(dataDir, "source"), 0755)
	os.MkdirAll(filepath.Join(dataDir, "output"), 0755)

	// Load params
	p, _ := config.LoadParams(dataDir)
	s.params = p

	// Load runs
	s.loadRuns()

	// Load selections
	s.loadSelections()

	return s
}

// Params returns the current parameters (thread-safe copy).
func (s *Store) Params() config.Params {
	s.mu.RLock()
	defer s.mu.RUnlock()
	return s.params
}

// SetParams updates parameters and persists.
func (s *Store) SetParams(p config.Params) error {
	s.mu.Lock()
	s.params = p
	s.mu.Unlock()
	return config.SaveParams(s.dataDir, p)
}

// DataDir returns the data directory path.
func (s *Store) DataDir() string {
	return s.dataDir
}

func (s *Store) loadRuns() {
	path := filepath.Join(s.dataDir, "runs.json")
	data, err := os.ReadFile(path)
	if err != nil {
		s.runs = []RunRecord{}
		return
	}
	json.Unmarshal(data, &s.runs)
}

func (s *Store) saveRuns() {
	data, _ := json.MarshalIndent(s.runs, "", "  ")
	os.WriteFile(filepath.Join(s.dataDir, "runs.json"), data, 0644)
}

func (s *Store) loadSelections() {
	path := filepath.Join(s.dataDir, "selections.json")
	data, err := os.ReadFile(path)
	if err != nil {
		s.selections = make(map[string][]int)
		return
	}
	json.Unmarshal(data, &s.selections)
}

func (s *Store) saveSelections() {
	data, _ := json.MarshalIndent(s.selections, "", "  ")
	os.WriteFile(filepath.Join(s.dataDir, "selections.json"), data, 0644)
}

// Run methods

func (s *Store) Runs() []RunRecord {
	s.mu.RLock()
	defer s.mu.RUnlock()
	out := make([]RunRecord, len(s.runs))
	copy(out, s.runs)
	return out
}

func (s *Store) GetRun(id string) *RunRecord {
	s.mu.RLock()
	defer s.mu.RUnlock()
	for i := range s.runs {
		if s.runs[i].ID == id {
			r := s.runs[i]
			return &r
		}
	}
	return nil
}

func (s *Store) AddRun(r RunRecord) {
	s.mu.Lock()
	s.runs = append([]RunRecord{r}, s.runs...)
	s.mu.Unlock()
	s.saveRuns()
}

func (s *Store) UpdateRun(id string, fn func(*RunRecord)) {
	s.mu.Lock()
	for i := range s.runs {
		if s.runs[i].ID == id {
			fn(&s.runs[i])
			break
		}
	}
	s.mu.Unlock()
	s.saveRuns()
}

func (s *Store) DeleteRun(id string) {
	s.mu.Lock()
	filtered := make([]RunRecord, 0, len(s.runs))
	for _, r := range s.runs {
		if r.ID != id {
			filtered = append(filtered, r)
		}
	}
	s.runs = filtered
	s.mu.Unlock()
	s.saveRuns()
	// Remove output directory
	os.RemoveAll(filepath.Join(s.dataDir, "output", id))
}

// Selection methods

func (s *Store) GetSelection(runID string) []int {
	s.mu.RLock()
	defer s.mu.RUnlock()
	sel, ok := s.selections[runID]
	if !ok {
		return nil
	}
	out := make([]int, len(sel))
	copy(out, sel)
	return out
}

func (s *Store) SetSelection(runID string, indices []int) {
	s.mu.Lock()
	if len(indices) == 0 {
		delete(s.selections, runID)
	} else {
		selections := make([]int, len(indices))
		copy(selections, indices)
		s.selections[runID] = selections
	}
	s.mu.Unlock()
	s.saveSelections()
}
