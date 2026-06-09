use crate::types::PointI;

/// Plane parameters: ax + by + cz + d = 0
#[derive(Debug, Clone)]
struct Plane {
    a: f32,
    b: f32,
    c: f32,
    d: f32,
}

/// Peel ground and walls from the cylinder box.
/// Returns (cleaned_cloud, ground_fraction) where ground_fraction = removed/original.
pub fn peel_ground(
    box_cloud: &[PointI],
    cx: f32,
    cy: f32,
    outer_ring_min: f32,    // e.g. 0.35
    outer_ring_max: f32,    // e.g. 0.50
    ransac_distance: f32,   // e.g. 0.05
    ransac_iters: usize,    // e.g. 1000
    wall_nz_max: f32,       // e.g. 0.3 — |nz| below this = vertical wall
    ground_z_pct: f32,      // e.g. 0.10 — use bottom 10% of Z to find ground
) -> (Vec<PointI>, f32) {
    let original_n = box_cloud.len();
    if original_n < 50 {
        return (box_cloud.to_vec(), 0.0);
    }

    let mut keep: Vec<bool> = vec![true; box_cloud.len()];

    // === Step 1: Z-based ground removal ===
    // Sort by Z, find the plane of the lowest points
    {
        let mut zs: Vec<(f32, usize)> = box_cloud
            .iter()
            .enumerate()
            .map(|(i, p)| (p.z, i))
            .collect();
        zs.sort_by(|a, b| a.0.partial_cmp(&b.0).unwrap_or(std::cmp::Ordering::Equal));

        let n_ground = ((box_cloud.len() as f32) * ground_z_pct).max(30.0) as usize;
        let n_ground = n_ground.min(zs.len());

        // Fit horizontal plane: mean Z of bottom points
        let mut sum_z = 0.0f32;
        for &(z, _) in &zs[..n_ground] {
            sum_z += z;
        }
        let mean_z = sum_z / n_ground as f32;

        // Remove points within ransac_distance of z = mean_z
        for &(z, idx) in &zs {
            if (z - mean_z).abs() < ransac_distance {
                keep[idx] = false;
            }
        }
    }

    // === Step 2: Outer-ring wall removal ===
    // Collect outer ring points for wall detection
    {
        let outer_ring: Vec<&PointI> = box_cloud
            .iter()
            .filter(|p| {
                let rx = p.x - cx;
                let ry = p.y - cy;
                let r = (rx * rx + ry * ry).sqrt();
                r >= outer_ring_min && r <= outer_ring_max
            })
            .collect();

        if outer_ring.len() >= 30 {
            // RANSAC: find ANY plane in the outer ring (unconstrained)
            let plane = ransac_plane(&outer_ring, ransac_distance, ransac_iters);

            // If this is a wall (|nz| small, meaning normal is horizontal),
            // remove those wall points from the entire box
            if plane.c.abs() < wall_nz_max {
                for (i, p) in box_cloud.iter().enumerate() {
                    if keep[i] {
                        let dist = (plane.a * p.x + plane.b * p.y + plane.c * p.z + plane.d).abs();
                        if dist < ransac_distance {
                            keep[i] = false;
                        }
                    }
                }
            }
        }
    }

    let cleaned: Vec<PointI> = box_cloud
        .iter()
        .enumerate()
        .filter(|(i, _)| keep[*i])
        .map(|(_, p)| *p)
        .collect();
    let removed_n = original_n - cleaned.len();
    let ground_fraction = removed_n as f32 / original_n as f32;
    (cleaned, ground_fraction)
}

/// Unconstrained RANSAC plane fitting. Returns best plane regardless of orientation.
fn ransac_plane(points: &[&PointI], distance_threshold: f32, max_iters: usize) -> Plane {
    if points.len() < 3 {
        return Plane { a: 0.0, b: 0.0, c: 1.0, d: 0.0 };
    }

    let n = points.len();
    let mut best_plane = Plane { a: 0.0, b: 0.0, c: 1.0, d: 0.0 };
    let mut best_inliers = 0usize;
    let mut state: u64 = 123456789;

    for _ in 0..max_iters {
        let i1 = rand_usize(&mut state, n);
        let i2 = rand_usize(&mut state, n);
        let i3 = rand_usize(&mut state, n);
        if i1 == i2 || i2 == i3 || i1 == i3 {
            continue;
        }

        let p1 = points[i1];
        let p2 = points[i2];
        let p3 = points[i3];

        let ux = p2.x - p1.x;
        let uy = p2.y - p1.y;
        let uz = p2.z - p1.z;
        let vx = p3.x - p1.x;
        let vy = p3.y - p1.y;
        let vz = p3.z - p1.z;

        let nx = uy * vz - uz * vy;
        let ny = uz * vx - ux * vz;
        let nz = ux * vy - uy * vx;
        let norm = (nx * nx + ny * ny + nz * nz).sqrt();

        if norm < 1e-10 {
            continue;
        }

        let a = nx / norm;
        let b = ny / norm;
        let c = nz / norm;
        let d = -(a * p1.x + b * p1.y + c * p1.z);

        let mut inliers = 0;
        for p in points.iter() {
            let dist = (a * p.x + b * p.y + c * p.z + d).abs();
            if dist < distance_threshold {
                inliers += 1;
            }
        }

        if inliers > best_inliers {
            best_inliers = inliers;
            best_plane = Plane { a, b, c, d };
        }
    }

    if best_inliers < (points.len() / 10) {
        return Plane { a: 0.0, b: 0.0, c: 0.0, d: 0.0 };
    }

    best_plane
}

fn rand_usize(state: &mut u64, max: usize) -> usize {
    *state ^= *state << 13;
    *state ^= *state >> 7;
    *state ^= *state << 17;
    (*state as usize) % max
}
