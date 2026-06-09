pub mod compact;
pub mod dxy;
pub mod ratio;
pub mod tripod;

use crate::types::FeatureSet;

/// Compute all features from a recentered box cloud.
/// This is a pure function — no side effects, no decisions.
pub fn extract_features(
    box_cloud: &crate::types::BoxCloud,
    lo_indices: &[usize],
    hi_indices: &[usize],
    ground_fraction: f32,
    cfg: &super::config::FeatureConfig,
) -> FeatureSet {
    let split = ratio::kmeans_split_by_index(&box_cloud.points, lo_indices, hi_indices);
    let hi_mean = split.hi_mean;
    let lo_mean = split.lo_mean;
    let ratio_val = hi_mean / (lo_mean + 0.1);

    let lo_centroid = crate::types::Centroid::from_points_by_index(&box_cloud.points, lo_indices);
    let hi_centroid = crate::types::Centroid::from_points_by_index(&box_cloud.points, hi_indices);
    let dxy_val = dxy::compute(&lo_centroid, &hi_centroid);

    let tripod_val = if cfg.tripod_enabled {
        tripod::compute(
            &box_cloud.points,
            lo_indices,
            &lo_centroid,
            &hi_centroid,
            cfg.tripod_bins,
            cfg.tripod_peak_min_ratio,
            cfg.tripod_min_peaks,
            cfg.tripod_angle_tolerance_deg,
        )
    } else {
        0.0
    };

    let compact_val = compact::compute(
        &box_cloud.points,
        hi_indices,
        cfg.compact_z_window,
    );

    FeatureSet {
        ratio: ratio_val,
        tripod: tripod_val,
        compact: compact_val,
        dxy: dxy_val,
        ground_fraction,
        hi_n: hi_indices.len(),
        lo_n: lo_indices.len(),
        total_n: box_cloud.len(),
        hi_centroid,
        lo_centroid,
        split_idx: lo_indices.len(),
    }
}
