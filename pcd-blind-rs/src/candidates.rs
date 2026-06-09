use crate::cluster::extract_clusters;
use crate::types::{Point3, PointI};

/// A candidate from Z-slice + XY clustering
#[derive(Debug, Clone)]
pub struct Candidate {
    /// XY centroid of the 2D cluster
    pub cx: f32,
    pub cy: f32,
    /// Z-slice base (bottom of the slice window)
    pub z_base: f32,
}

/// Generate candidates from full point cloud.
///
/// 1. Filter by I > Pxx threshold
/// 2. For each Z-slice, project to XY (z=0), cluster in 2D
/// 3. De-duplicate across slices
pub fn generate_candidates(
    full_cloud: &[PointI],
    intensity_threshold: f32,
    z_start: f32,
    z_step: f32,
    z_end: f32,
    cluster_tolerance: f32,
    min_cluster_size: usize,
    _max_cluster_size: usize,
    dedup_xy: f32,
    dedup_z: f32,
) -> Vec<Candidate> {
    let mut candidates = Vec::new();
    let mut used: Vec<(f32, f32, f32)> = Vec::new(); // (cx, cy, z) for dedup

    let mut z = z_start;
    while z > z_end {
        // Collect high-intensity points in this Z-slice and project to XY
        let slice_z_hi = z + z_step;
        let mut slice_points: Vec<Point3> = Vec::new();
        let mut slice_indices: Vec<usize> = Vec::new(); // maps slice index -> full_cloud index

        for (i, p) in full_cloud.iter().enumerate() {
            if p.z >= z && p.z < slice_z_hi && p.intensity >= intensity_threshold {
                slice_points.push(Point3 {
                    x: p.x,
                    y: p.y,
                    z: 0.0,
                });
                slice_indices.push(i);
            }
        }

        if slice_points.len() < 50 {
            z -= z_step;
            continue;
        }

        // 2D Euclidean clustering
        let clusters = extract_clusters(
            &slice_points,
            cluster_tolerance,
            min_cluster_size,
            5000,
        );

        for cl in &clusters {
            if cl.len() < 20 {
                continue;
            }

            // Compute centroid
            let mut cx = 0.0f32;
            let mut cy = 0.0f32;
            for &idx in cl {
                cx += slice_points[idx].x;
                cy += slice_points[idx].y;
            }
            let n = cl.len() as f32;
            cx /= n;
            cy /= n;

            // Dedup: skip if there's already a candidate at nearby XY+Z
            let mut duplicate = false;
            for &(ux, uy, uz) in &used {
                let dx = cx - ux;
                let dy = cy - uy;
                let dz = (z - uz).abs();
                if dx * dx + dy * dy < dedup_xy * dedup_xy && dz < dedup_z {
                    duplicate = true;
                    break;
                }
            }
            if duplicate {
                continue;
            }

            used.push((cx, cy, z));
            candidates.push(Candidate {
                cx,
                cy,
                z_base: z,
            });
        }

        z -= z_step;
    }

    candidates
}
