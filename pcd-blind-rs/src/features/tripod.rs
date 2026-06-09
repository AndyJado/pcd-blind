use crate::types::{Centroid, PointI};

/// Compute tripod score: how well do lo points' XY angles form 3 equally-spaced peaks?
///
/// Projects lo points to XY plane and computes angular histogram from the lo centroid.
/// Tripod legs naturally separate in XY angle; ground scatter is roughly uniform.
///
/// Returns 0.0 if no tripod structure detected, up to ~1.0 for a perfect 3-leg tripod.
pub fn compute(
    points: &[PointI],
    lo_indices: &[usize],
    lo_centroid: &Centroid,
    _hi_centroid: &Centroid,
    bins: usize,
    peak_min_ratio: f32,
    min_peaks: usize,
    angle_tolerance_deg: f32,
) -> f32 {
    if lo_indices.len() < 30 {
        return 0.0;
    }

    let bin_width = 360.0f32 / bins as f32;
    let mut hist = vec![0u32; bins];

    for &i in lo_indices {
        let p = &points[i];
        let dx = p.x - lo_centroid.x;
        let dy = p.y - lo_centroid.y;
        let dist2 = dx * dx + dy * dy;
        if dist2 < 0.0001 {
            continue;
        }
        let mut angle = dy.atan2(dx).to_degrees();
        if angle < 0.0 {
            angle += 360.0;
        }
        let bin = (angle / bin_width) as usize % bins;
        hist[bin] += 1;
    }

    let total: u32 = hist.iter().sum();
    if total == 0 {
        return 0.0;
    }

    let mean = total as f32 / bins as f32;
    if mean < 1.0 {
        return 0.0;
    }

    let mut peaks = Vec::new();
    for i in 0..bins {
        let prev = if i == 0 { bins - 1 } else { i - 1 };
        let next = if i == bins - 1 { 0 } else { i + 1 };
        let v = hist[i] as f32;
        let vp = hist[prev] as f32;
        let vn = hist[next] as f32;
        if v > vp && v > vn && v >= mean * peak_min_ratio {
            peaks.push((i, v));
        }
    }

    if peaks.len() < min_peaks {
        return 0.0;
    }

    peaks.sort_by(|a, b| b.1.partial_cmp(&a.1).unwrap_or(std::cmp::Ordering::Equal));
    peaks.truncate(3);
    if peaks.len() < 3 {
        return 0.0;
    }

    peaks.sort_by_key(|(bin, _)| *bin);

    let ideal = 360.0f32 / 3.0;
    let mut spacing_err = 0.0f32;
    for i in 0..3 {
        let a1 = peaks[i].0 as f32 * bin_width;
        let a2 = peaks[(i + 1) % 3].0 as f32 * bin_width;
        let mut diff = (a2 - a1).abs();
        if diff > 180.0 {
            diff = 360.0 - diff;
        }
        spacing_err += (diff - ideal).abs();
    }
    spacing_err /= 3.0;

    let spacing_score = (1.0 - spacing_err / angle_tolerance_deg).max(0.0);
    let height_ratio = peaks[2].1 / peaks[0].1;

    spacing_score * height_ratio
}

#[cfg(test)]
mod tests {
    use super::*;

    fn make_centroid(x: f32, y: f32) -> Centroid {
        Centroid { x, y, z: 0.0 }
    }

    #[test]
    fn test_tripod_perfect_3_legs() {
        // 3 legs at 0°, 120°, 240° from centroid
        let mut points = Vec::new();
        for leg in 0..3 {
            let angle = (leg as f32) * 120.0_f32.to_radians();
            let dx = angle.cos() * 0.2;
            let dy = angle.sin() * 0.2;
            for _ in 0..20 {
                points.push(PointI { x: 0.0 + dx, y: 0.0 + dy, z: 0.0, intensity: 50.0 });
            }
        }
        let lo: Vec<usize> = (0..60).collect();
        let lc = make_centroid(0.0, 0.0);
        let hc = make_centroid(0.0, 0.0);
        let score = compute(&points, &lo, &lc, &hc, 36, 1.5, 3, 20.0);
        assert!(score > 0.5, "perfect 3-leg tripod should score >0.5, got {}", score);
    }

    #[test]
    fn test_tripod_single_leg() {
        // Only 1 leg
        let mut points = Vec::new();
        for _ in 0..30 {
            points.push(PointI { x: 0.2, y: 0.0, z: 0.0, intensity: 50.0 });
        }
        let lo: Vec<usize> = (0..30).collect();
        let lc = make_centroid(0.0, 0.0);
        let hc = make_centroid(0.0, 0.0);
        let score = compute(&points, &lo, &lc, &hc, 36, 1.5, 3, 20.0);
        assert_eq!(score, 0.0, "single leg should score 0");
    }

    #[test]
    fn test_tripod_random_uniform() {
        // Uniform angular distribution → no tripod
        let mut points = Vec::new();
        for i in 0..120 {
            let angle = (i as f32) * 3.0_f32.to_radians();
            points.push(PointI { x: angle.cos() * 0.2, y: angle.sin() * 0.2, z: 0.0, intensity: 50.0 });
        }
        let lo: Vec<usize> = (0..120).collect();
        let lc = make_centroid(0.0, 0.0);
        let hc = make_centroid(0.0, 0.0);
        let score = compute(&points, &lo, &lc, &hc, 36, 1.5, 3, 20.0);
        assert_eq!(score, 0.0, "uniform distribution should score 0, got {}", score);
    }
}
