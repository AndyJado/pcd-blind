package config

import (
	"encoding/json"
	"os"
	"path/filepath"
)

// Params holds all tunable parameters — both pipeline and gate/score.
type Params struct {
	// ── Pipeline (affect candidate generation) ──
	Pct             float64 `json:"pct"`
	BallDiameter    float64 `json:"ball_diameter"`
	ZStart          float64 `json:"z_start"`
	ZStep           float64 `json:"z_step"`
	ZEnd            float64 `json:"z_end"`
	ClusterTol      float64 `json:"cluster_tolerance"`
	MinClusterSize  int     `json:"min_cluster_size"`
	CylRadius       float64 `json:"cyl_radius"`
	BoxDzUp         float64 `json:"box_dz_up"`
	BoxDzDn         float64 `json:"box_dz_dn"`
	OuterRingMin    float64 `json:"outer_ring_min"`
	OuterRingMax    float64 `json:"outer_ring_max"`
	RansacDist      float64 `json:"ransac_distance"`
	RansacIters     int     `json:"ransac_iters"`
	WallNzMax       float64 `json:"wall_nz_max"`
	GroundZPct      float64 `json:"ground_z_pct"`
	MaxIters        int     `json:"max_iters"`
	DxyThreshold    float64 `json:"dxy_threshold"`

	// ── Gate ──
	RatioMin    float64 `json:"ratio_min"`
	CompactMin  float64 `json:"compact_min"`
	DxyMax      float64 `json:"dxy_max"`
	TripodMin   float64 `json:"tripod_min"`
	GroundMin   float64 `json:"ground_min"`

	// ── Score weights ──
	TripodWeight  float64 `json:"tripod_weight"`
	RatioWeight   float64 `json:"ratio_weight"`
	CompactWeight float64 `json:"compact_weight"`
	DxyWeight     float64 `json:"dxy_weight"`
	GroundWeight  float64 `json:"ground_weight"`

	// ── Dedup ──
	DedupRadius float64 `json:"dedup_radius"`
	DedupZ      float64 `json:"dedup_z"`

	// ── Tripod ──
	TripodEnabled bool `json:"tripod_enabled"`
}

// DefaultParams returns the factory defaults matching config/default.toml.
func DefaultParams() Params {
	return Params{
		Pct:             80.0,
		BallDiameter:    0.2,
		ZStart:          3.0,
		ZStep:           0.2,
		ZEnd:            -5.0,
		ClusterTol:      0.15,
		MinClusterSize:  20,
		CylRadius:       0.5,
		BoxDzUp:         0.3,
		BoxDzDn:         2.0,
		OuterRingMin:    0.35,
		OuterRingMax:    0.5,
		RansacDist:      0.05,
		RansacIters:     1000,
		WallNzMax:       0.3,
		GroundZPct:      0.10,
		MaxIters:        3,
		DxyThreshold:    0.15,
		RatioMin:        5.0,
		CompactMin:      0.88,
		DxyMax:          0.15,
		TripodMin:       0.0,
		GroundMin:       0.05,
		TripodWeight:    4.0,
		RatioWeight:     1.0,
		CompactWeight:   2.0,
		DxyWeight:      -3.0,
		GroundWeight:    3.0,
		DedupRadius:     0.5,
		DedupZ:          1.0,
		TripodEnabled:   true,
	}
}

// AppConfig holds the runtime configuration.
type AppConfig struct {
	Port    int    `json:"port"`
	DataDir string `json:"data_dir"`
	BinPath string `json:"bin_path"` // path to pcd-blind binary
}

func DefaultAppConfig() AppConfig {
	return AppConfig{
		Port:    8080,
		DataDir: "./data",
		BinPath: "./pcd-blind",
	}
}

// LoadParams reads params.json from data dir, or returns defaults.
func LoadParams(dataDir string) (Params, error) {
	p := DefaultParams()
	path := filepath.Join(dataDir, "params.json")
	data, err := os.ReadFile(path)
	if os.IsNotExist(err) {
		return p, nil
	}
	if err != nil {
		return p, err
	}
	if err := json.Unmarshal(data, &p); err != nil {
		return DefaultParams(), err
	}
	return p, nil
}

// SaveParams writes params to data dir.
func SaveParams(dataDir string, p Params) error {
	os.MkdirAll(dataDir, 0755)
	data, err := json.MarshalIndent(p, "", "  ")
	if err != nil {
		return err
	}
	return os.WriteFile(filepath.Join(dataDir, "params.json"), data, 0644)
}
