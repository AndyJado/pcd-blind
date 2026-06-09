use crate::ground::peel_ground;
use crate::types::{BoxCloud, Centroid, PointI};
use crate::features::ratio::kmeans_split_by_intensity;

/// Result of the recentering loop
#[derive(Debug, Clone)]
pub struct RecenterResult {
    /// The final box cloud (after ground peel)
    pub box_cloud: BoxCloud,
    /// The corrected (cx, cy)
    pub cx: f32,
    pub cy: f32,
    /// Split into lo/hi
    pub lo_indices: Vec<usize>,
    pub hi_indices: Vec<usize>,
    /// Centroids
    pub lo_centroid: Centroid,
    pub hi_centroid: Centroid,
    /// Final dxy after convergence
    pub dxy: f32,
    /// Whether convergence was achieved
    pub converged: bool,
    /// Number of iterations taken
    pub iterations: usize,
    /// Fraction of box removed as ground
    pub ground_fraction: f32,
}

/// Perform dxy-driven recentering loop.
///
/// For each candidate, repeatedly crop → peel ground → split → compute dxy,
/// adjusting (cx,cy) toward the midpoint of lo/hi centroids until dxy is small.
pub fn recenter(
    full_cloud: &[PointI],
    mut cx: f32,
    mut cy: f32,
    cz_center: f32,
    cyl_radius: f32,
    box_dz_dn: f32,
    box_dz_up: f32,
    ground_config: &GroundConfig,
    recenter_config: &RecenterConfig,
) -> Option<RecenterResult> {
    for iter in 0..recenter_config.max_iters {
        // Cylinder crop
        let bf: Vec<PointI> = full_cloud
            .iter()
            .filter(|p| {
                let rx = p.x - cx;
                let ry = p.y - cy;
                rx * rx + ry * ry <= cyl_radius * cyl_radius
                    && p.z >= cz_center - box_dz_dn
                    && p.z <= cz_center + box_dz_up
            })
            .copied()
            .collect();

        if bf.len() < 200 {
            return None;
        }

        // Peel ground (Z-base + outer-ring wall check)
        let (box_pts, _ground_fraction) = peel_ground(
            &bf,
            cx,
            cy,
            ground_config.outer_ring_min,
            ground_config.outer_ring_max,
            ground_config.ransac_distance,
            ground_config.ransac_iters,
            ground_config.wall_nz_max,
            ground_config.ground_z_pct,
        );

        let ground_removed = (_ground_fraction * bf.len() as f32) as usize;

        if box_pts.len() < 200 {
            return None;
        }

        // K-means intensity split
        let split_data = kmeans_split_by_intensity(&box_pts);
        let lo_indices = split_data.lo_indices;
        let hi_indices = split_data.hi_indices;

        let lo_centroid = Centroid::from_points_by_index(&box_pts, &lo_indices);
        let hi_centroid = Centroid::from_points_by_index(&box_pts, &hi_indices);
        let dxy = lo_centroid.xy_distance(&hi_centroid);

        let converged = dxy <= recenter_config.dxy_threshold;

        if converged || iter + 1 >= recenter_config.max_iters {
            return Some(RecenterResult {
                box_cloud: BoxCloud::new(box_pts),
                cx,
                cy,
                lo_indices,
                hi_indices,
                lo_centroid,
                hi_centroid,
                dxy,
                converged,
                iterations: iter + 1,
                ground_fraction: _ground_fraction,
            });
        }

        // Adjust center toward midpoint
        cx = (lo_centroid.x + hi_centroid.x) / 2.0;
        cy = (lo_centroid.y + hi_centroid.y) / 2.0;
        // Note: cz_center stays the same (Z from initial slice)
    }

    None
}

#[derive(Debug, Clone)]
pub struct GroundConfig {
    pub outer_ring_min: f32,
    pub outer_ring_max: f32,
    pub ransac_distance: f32,
    pub ransac_iters: usize,
    pub wall_nz_max: f32,
    pub ground_z_pct: f32,
}

#[derive(Debug, Clone)]
pub struct RecenterConfig {
    pub max_iters: usize,
    pub dxy_threshold: f32,
}
