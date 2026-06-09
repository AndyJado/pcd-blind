use crate::types::Centroid;

/// Compute XY distance between lo and hi centroids.
/// Small dxy means the ball is centered over the tripod.
pub fn compute(lo: &Centroid, hi: &Centroid) -> f32 {
    lo.xy_distance(hi)
}
