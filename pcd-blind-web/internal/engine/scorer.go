package engine

import (
	"encoding/csv"
	"math"
	"os"
	"path/filepath"
	"sort"
	"strconv"

	"pcd-blind-web/internal/config"
)

// CandidateRow represents one row from candidates.csv.
type CandidateRow struct {
	Idx             int
	Ratio           float64
	Compact         float64
	Tripod          float64
	Dxy             float64
	GroundFraction  float64
	Cx              float64
	Cy              float64
	Cz              float64
	HiN             int
	LoN             int
	TotalN          int
	Score           float64
	Gated           bool
}

// LoadCandidates reads candidates.csv from a run output directory.
func LoadCandidates(runDir string) ([]CandidateRow, error) {
	path := filepath.Join(runDir, "candidates.csv")
	f, err := os.Open(path)
	if err != nil {
		return nil, err
	}
	defer f.Close()

	reader := csv.NewReader(f)
	records, err := reader.ReadAll()
	if err != nil {
		return nil, err
	}

	if len(records) < 2 {
		return nil, nil
	}

	candidates := make([]CandidateRow, 0, len(records)-1)
	for i := 1; i < len(records); i++ {
		row := records[i]
		if len(row) < 11 {
			continue
		}
		c := CandidateRow{
			Idx:            atoi(row[0]),
			Ratio:          atof(row[1]),
			Compact:        atof(row[2]),
			Tripod:         atof(row[3]),
			Dxy:            atof(row[4]),
			GroundFraction: atof(row[5]),
			Cx:             atof(row[6]),
			Cy:             atof(row[7]),
			Cz:             atof(row[8]),
			HiN:            atoi(row[9]),
			LoN:            atoi(row[10]),
			TotalN:         atoi(row[11]),
		}
		candidates = append(candidates, c)
	}

	return candidates, nil
}

// GateAndScore applies gating, scoring, dedup, and sorting.
func GateAndScore(candidates []CandidateRow, p config.Params) []CandidateRow {
	// 1. Gate
	var passed []CandidateRow
	for _, c := range candidates {
		gated := c.Ratio >= p.RatioMin &&
			c.Compact >= p.CompactMin &&
			c.Dxy <= p.DxyMax &&
			c.GroundFraction >= p.GroundMin
		if p.TripodEnabled {
			gated = gated && c.Tripod >= p.TripodMin
		}
		c.Gated = gated
		if gated {
			passed = append(passed, c)
		}
	}

	// 2. Score
	for i := range passed {
		passed[i].Score = p.TripodWeight*passed[i].Tripod +
			p.RatioWeight*passed[i].Ratio +
			p.CompactWeight*passed[i].Compact +
			p.DxyWeight*passed[i].Dxy +
			p.GroundWeight*passed[i].GroundFraction
	}

	// 3. Dedup by XY+Z proximity (keep lower dxy, matching Rust behavior)
	deduped := make([]CandidateRow, 0, len(passed))
	used := make([]bool, len(passed))
	for i := range passed {
		if used[i] {
			continue
		}
		best := i
		for j := i + 1; j < len(passed); j++ {
			if used[j] {
				continue
			}
			dx := passed[i].Cx - passed[j].Cx
			dy := passed[i].Cy - passed[j].Cy
			dz := math.Abs(passed[i].Cz - passed[j].Cz)
			if dx*dx+dy*dy < p.DedupRadius*p.DedupRadius && dz < p.DedupZ {
				if passed[j].Dxy < passed[best].Dxy {
					best = j
				}
				used[j] = true
			}
		}
		deduped = append(deduped, passed[best])
	}

	// 4. Sort by score descending
	sort.Slice(deduped, func(i, j int) bool {
		return deduped[i].Score > deduped[j].Score
	})

	return deduped
}

// LoadAndFilter loads candidates from a run and applies the current gate params.
func LoadAndFilter(runDir string, p config.Params) ([]CandidateRow, error) {
	candidates, err := LoadCandidates(runDir)
	if err != nil {
		return nil, err
	}
	return GateAndScore(candidates, p), nil
}

func atof(s string) float64 {
	v, _ := strconv.ParseFloat(s, 64)
	return v
}

func atoi(s string) int {
	v, _ := strconv.Atoi(s)
	return v
}
