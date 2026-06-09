use crate::cluster::extract_clusters;
use crate::types::{Point3, PointI};

/// A candidate from Z-slice + XY clustering
#[derive(Debug, Clone)]
pub struct Candidate {
    /// XY centroid (intensity-weighted average after slice merging)
    pub cx: f32,
    pub cy: f32,
    /// Z height estimate (intensity-weighted average Z of the merged cluster)
    pub z_center: f32,
}

/// Generate candidates from full point cloud.
///
/// 1. Filter by I > Pxx threshold
/// 2. For each Z-slice, project to XY (z=0), cluster in 2D
/// 3. Filter clusters by XY extent (must be ball-scale: 0.3D ~ 3D)
/// 4. Merge clusters across Z-slices by XY proximity → one candidate per ball
pub fn generate_candidates(
    full_cloud: &[PointI],
    intensity_threshold: f32,
    z_start: f32,
    z_step: f32,
    z_end: f32,
    cluster_tolerance: f32,
    min_cluster_size: usize,
    ball_diameter: f32,
) -> Vec<Candidate> {
    // Collect all clusters across all Z-slices as (cx, cy, cz, weight)
    // where cz is the mean Z of the points in the cluster, weight = cluster size
    let mut raw: Vec<(f32, f32, f32, usize)> = Vec::new();

    let xy_min_extent = 0.3 * ball_diameter; // ~0.06m
    let xy_max_extent = 3.0 * ball_diameter; // ~0.60m

    let mut z = z_start;
    while z > z_end {
        let slice_z_hi = z + z_step;
        let mut slice_points: Vec<Point3> = Vec::new();
        let mut slice_full: Vec<&PointI> = Vec::new(); // reference to full_cloud points for Z computation

        for p in full_cloud.iter() {
            if p.z >= z && p.z < slice_z_hi && p.intensity >= intensity_threshold {
                slice_points.push(Point3 { x: p.x, y: p.y, z: 0.0 });
                slice_full.push(p);
            }
        }

        if slice_points.len() < 50 {
            z -= z_step;
            continue;
        }

        let clusters = extract_clusters(&slice_points, cluster_tolerance, min_cluster_size, 5000);

        for cl in &clusters {
            if cl.len() < 20 {
                continue;
            }

            // XY centroid
            let mut cx = 0.0f32;
            let mut cy = 0.0f32;
            for &idx in cl {
                cx += slice_points[idx].x;
                cy += slice_points[idx].y;
            }
            let n = cl.len() as f32;
            cx /= n;
            cy /= n;

            // XY extent filter: cluster must be ball-scale
            let (mut xmin, mut xmax) = (f32::MAX, f32::MIN);
            let (mut ymin, mut ymax) = (f32::MAX, f32::MIN);
            for &idx in cl {
                let x = slice_points[idx].x;
                let y = slice_points[idx].y;
                if x < xmin { xmin = x; }
                if x > xmax { xmax = x; }
                if y < ymin { ymin = y; }
                if y > ymax { ymax = y; }
            }
            let extent = ((xmax - xmin).powi(2) + (ymax - ymin).powi(2)).sqrt();
            if extent < xy_min_extent || extent > xy_max_extent {
                continue;
            }

            // Mean Z of the cluster (from original 3D points)
            let mut sum_z = 0.0f32;
            for &idx in cl {
                sum_z += slice_full[idx].z;
            }
            let cz = sum_z / n;

            raw.push((cx, cy, cz, cl.len()));
        }

        z -= z_step;
    }

    if raw.is_empty() {
        return Vec::new();
    }

    // Merge clusters across Z-slices: group by XY proximity, merge into candidates
    let merge_xy = 0.3f32; // 0.3m XY radius for same-ball clusters
    let merge_z = 0.5f32;  // 0.5m Z tolerance for same-ball clusters

    let mut groups: Vec<Vec<usize>> = Vec::new();
    let mut assigned = vec![false; raw.len()];

    for i in 0..raw.len() {
        if assigned[i] {
            continue;
        }
        assigned[i] = true;
        let mut group = vec![i];
        let mut queue = vec![i];

        while let Some(cur) = queue.pop() {
            let (cx, cy, cz, _) = raw[cur];
            for j in 0..raw.len() {
                if assigned[j] {
                    continue;
                }
                let dx = cx - raw[j].0;
                let dy = cy - raw[j].1;
                let dz = (cz - raw[j].2).abs();
                if dx * dx + dy * dy < merge_xy * merge_xy && dz < merge_z {
                    assigned[j] = true;
                    queue.push(j);
                    group.push(j);
                }
            }
        }
        groups.push(group);
    }

    // For each group, compute intensity-weighted centroid as a single candidate
    let mut candidates = Vec::new();
    for group in &groups {
        let mut sum_wx = 0.0f32;
        let mut sum_wy = 0.0f32;
        let mut sum_wz = 0.0f32;
        let mut sum_w = 0.0f32;
        for &idx in group {
            let w = raw[idx].3 as f32; // cluster size as weight
            sum_wx += raw[idx].0 * w;
            sum_wy += raw[idx].1 * w;
            sum_wz += raw[idx].2 * w;
            sum_w += w;
        }
        if sum_w > 0.0 {
            candidates.push(Candidate {
                cx: sum_wx / sum_w,
                cy: sum_wy / sum_w,
                z_center: sum_wz / sum_w,
            });
        }
    }

    candidates
}
