use crate::types::PointI;

/// Compute compactness of hi points: take the top Z-window from hi's max Z,
/// compute 3D spatial variance, return 1/(1+σ) as a 0-1 score.
pub fn compute(points: &[PointI], hi_indices: &[usize], z_window: f32) -> f32 {
    if hi_indices.is_empty() {
        return 0.0;
    }

    // Find max Z among hi points
    let z_max = hi_indices
        .iter()
        .map(|&i| points[i].z)
        .fold(f32::NEG_INFINITY, f32::max);

    // Collect hi points within [z_max - z_window, z_max]
    let crop: Vec<&PointI> = hi_indices
        .iter()
        .filter(|&&i| points[i].z >= z_max - z_window)
        .map(|&i| &points[i])
        .collect();

    let crop_n = crop.len();
    if crop_n == 0 {
        return 0.0;
    }

    // Centroid
    let (mut cmx, mut cmy, mut cmz) = (0.0f32, 0.0f32, 0.0f32);
    for p in &crop {
        cmx += p.x;
        cmy += p.y;
        cmz += p.z;
    }
    cmx /= crop_n as f32;
    cmy /= crop_n as f32;
    cmz /= crop_n as f32;

    // Variance
    let mut cvar = 0.0f32;
    for p in &crop {
        let dx = p.x - cmx;
        let dy = p.y - cmy;
        let dz = p.z - cmz;
        cvar += dx * dx + dy * dy + dz * dz;
    }

    let sigma = (cvar / crop_n as f32).sqrt();
    1.0 / (1.0 + sigma)
}
