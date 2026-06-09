use serde::Deserialize;
use std::path::Path;

#[derive(Debug, Clone, Deserialize)]
pub struct Config {
    #[serde(default)]
    pub detect: DetectConfig,
    #[serde(default)]
    pub candidates: CandidateConfig,
    #[serde(default)]
    pub ground: GroundConfig,
    #[serde(default)]
    pub recenter: RecenterConfig,
    #[serde(default)]
    pub features: FeatureConfig,
    #[serde(default)]
    pub scoring: ScoringConfig,
    #[serde(default)]
    pub output: OutputConfig,
}

#[derive(Debug, Clone, Deserialize)]
pub struct DetectConfig {
    #[serde(default = "default_pct")]
    pub pct: f32,
}

#[derive(Debug, Clone, Deserialize)]
pub struct CandidateConfig {
    #[serde(default = "default_z_start")]
    pub z_start: f32,
    #[serde(default = "default_z_step")]
    pub z_step: f32,
    #[serde(default = "default_z_end")]
    pub z_end: f32,
    #[serde(default = "default_cluster_tolerance")]
    pub cluster_tolerance: f32,
    #[serde(default = "default_min_cluster")]
    pub min_cluster_size: usize,
    #[serde(default = "default_max_cluster")]
    pub max_cluster_size: usize,
    #[serde(default = "default_dedup_xy")]
    pub dedup_xy: f32,
    #[serde(default = "default_dedup_z")]
    pub dedup_z: f32,
}

#[derive(Debug, Clone, Deserialize)]
pub struct GroundConfig {
    #[serde(default = "default_cyl_radius")]
    pub cyl_radius: f32,
    #[serde(default = "default_box_dz_up")]
    pub box_dz_up: f32,
    #[serde(default = "default_box_dz_dn")]
    pub box_dz_dn: f32,
    #[serde(default = "default_outer_min")]
    pub outer_ring_min: f32,
    #[serde(default = "default_outer_max")]
    pub outer_ring_max: f32,
    #[serde(default = "default_ransac_dist")]
    pub ransac_distance: f32,
    #[serde(default = "default_ransac_iters")]
    pub ransac_iters: usize,
    #[serde(default = "default_wall_nz_max")]
    pub wall_nz_max: f32,
    #[serde(default = "default_ground_z_pct")]
    pub ground_z_pct: f32,
}

#[derive(Debug, Clone, Deserialize)]
pub struct RecenterConfig {
    #[serde(default = "default_max_iters")]
    pub max_iters: usize,
    #[serde(default = "default_dxy_thr")]
    pub dxy_threshold: f32,
}

#[derive(Debug, Clone, Deserialize)]
pub struct FeatureConfig {
    #[serde(default = "default_tripod_enabled")]
    pub tripod_enabled: bool,
    #[serde(default = "default_tripod_bins")]
    pub tripod_bins: usize,
    #[serde(default = "default_peak_min_ratio")]
    pub tripod_peak_min_ratio: f32,
    #[serde(default = "default_min_peaks")]
    pub tripod_min_peaks: usize,
    #[serde(default = "default_angle_tol")]
    pub tripod_angle_tolerance_deg: f32,
    #[serde(default = "default_z_window")]
    pub compact_z_window: f32,
    #[serde(default = "default_ratio_min")]
    pub ratio_min: f32,
    #[serde(default = "default_compact_min")]
    pub compact_min: f32,
    #[serde(default = "default_dxy_max_residual")]
    pub dxy_max_residual: f32,
    #[serde(default = "default_ground_fraction_min")]
    pub ground_fraction_min: f32,
}

#[derive(Debug, Clone, Deserialize)]
pub struct ScoringConfig {
    #[serde(default = "default_tripod_weight")]
    pub tripod_weight: f32,
    #[serde(default = "default_ratio_weight")]
    pub ratio_weight: f32,
    #[serde(default = "default_compact_weight")]
    pub compact_weight: f32,
    #[serde(default = "default_dxy_weight")]
    pub dxy_weight: f32,
    #[serde(default = "default_ground_weight")]
    pub ground_weight: f32,
    #[serde(default = "default_dedup_radius")]
    pub dedup_radius: f32,
    #[serde(default = "default_dedup_z_radius")]
    pub dedup_z: f32,
}

#[derive(Debug, Clone, Deserialize)]
pub struct OutputConfig {
    #[serde(default = "default_save_matches")]
    pub save_matches: bool,
}

// --- Default values ---

fn default_pct() -> f32 { 80.0 }
fn default_z_start() -> f32 { 3.0 }
fn default_z_step() -> f32 { 0.2 }
fn default_z_end() -> f32 { -5.0 }
fn default_cluster_tolerance() -> f32 { 0.15 }
fn default_min_cluster() -> usize { 20 }
fn default_max_cluster() -> usize { 5000 }
fn default_dedup_xy() -> f32 { 0.3 }
fn default_dedup_z() -> f32 { 0.3 }
fn default_cyl_radius() -> f32 { 0.5 }
fn default_box_dz_up() -> f32 { 0.3 }
fn default_box_dz_dn() -> f32 { 2.0 }
fn default_outer_min() -> f32 { 0.35 }
fn default_outer_max() -> f32 { 0.5 }
fn default_ransac_dist() -> f32 { 0.05 }
fn default_ransac_iters() -> usize { 1000 }
fn default_wall_nz_max() -> f32 { 0.3 }
fn default_ground_z_pct() -> f32 { 0.10 }
fn default_max_iters() -> usize { 3 }
fn default_dxy_thr() -> f32 { 0.15 }
fn default_tripod_enabled() -> bool { true }
fn default_tripod_bins() -> usize { 36 }
fn default_peak_min_ratio() -> f32 { 1.5 }
fn default_min_peaks() -> usize { 3 }
fn default_angle_tol() -> f32 { 20.0 }
fn default_z_window() -> f32 { 0.24 }
fn default_ratio_min() -> f32 { 5.0 }
fn default_compact_min() -> f32 { 0.88 }
fn default_dxy_max_residual() -> f32 { 0.15 }
fn default_ground_fraction_min() -> f32 { 0.05 }
fn default_tripod_weight() -> f32 { 4.0 }
fn default_ratio_weight() -> f32 { 1.0 }
fn default_compact_weight() -> f32 { 2.0 }
fn default_dxy_weight() -> f32 { -3.0 }
fn default_ground_weight() -> f32 { 3.0 }
fn default_dedup_radius() -> f32 { 0.5 }
fn default_dedup_z_radius() -> f32 { 1.0 }
fn default_save_matches() -> bool { true }

impl Config {
    pub fn load(path: &Path) -> anyhow::Result<Self> {
        let content = std::fs::read_to_string(path)?;
        let config: Self = toml::from_str(&content)?;
        Ok(config)
    }
}

impl Default for Config {
    fn default() -> Self {
        toml::from_str("").unwrap()
    }
}

// Provide Default for sub-configs
macro_rules! impl_default {
    ($t:ty) => {
        impl Default for $t {
            fn default() -> Self {
                <Self as Deserialize>::deserialize(toml::Value::Table(Default::default())).unwrap()
            }
        }
    };
}

impl_default!(DetectConfig);
impl_default!(CandidateConfig);
impl_default!(GroundConfig);
impl_default!(RecenterConfig);
impl_default!(FeatureConfig);
impl_default!(ScoringConfig);
impl_default!(OutputConfig);
