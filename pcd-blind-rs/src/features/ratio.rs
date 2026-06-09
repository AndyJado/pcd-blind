use crate::types::PointI;

/// Result of k-means intensity splitting (already split externally)
pub struct SplitResult {
    pub lo_mean: f32,
    pub hi_mean: f32,
}

/// Compute k-means split by intensity and return lo/hi means.
/// The indices are already pre-computed by the recenter module.
/// This function just computes the means.
pub fn kmeans_split_by_index(
    points: &[PointI],
    lo_indices: &[usize],
    hi_indices: &[usize],
) -> SplitResult {
    let lo_sum: f32 = lo_indices.iter().map(|&i| points[i].intensity).sum();
    let hi_sum: f32 = hi_indices.iter().map(|&i| points[i].intensity).sum();

    SplitResult {
        lo_mean: if lo_indices.is_empty() {
            0.0
        } else {
            lo_sum / lo_indices.len() as f32
        },
        hi_mean: if hi_indices.is_empty() {
            0.0
        } else {
            hi_sum / hi_indices.len() as f32
        },
    }
}

/// Find the optimal intensity split point (k-means with k=2 on 1D intensity).
/// Returns the split index into the sorted array, plus lo_indices and hi_indices
/// mapped back to original point indices.
pub fn kmeans_split_by_intensity(points: &[PointI]) -> SplitByIntensity {
    let n = points.len();
    if n < 10 {
        return SplitByIntensity {
            split_idx: 0,
            lo_indices: Vec::new(),
            hi_indices: (0..n).collect(),
        };
    }

    // Sort by intensity
    let mut indexed: Vec<(f32, usize)> = points
        .iter()
        .enumerate()
        .map(|(i, p)| (p.intensity, i))
        .collect();
    indexed.sort_by(|a, b| a.0.partial_cmp(&b.0).unwrap_or(std::cmp::Ordering::Equal));

    let intensities: Vec<f32> = indexed.iter().map(|(v, _)| *v).collect();

    // Find best split point that minimizes within-group variance
    let mut best_var = f32::MAX;
    let mut best_split = n / 10;

    let start = (n as f32 * 0.1) as usize;
    let end = (n as f32 * 0.9) as usize;
    for q in start..end {
        let mut lo_sum = 0.0f32;
        let mut hi_sum = 0.0f32;
        for i in 0..q {
            lo_sum += intensities[i];
        }
        for i in q..n {
            hi_sum += intensities[i];
        }
        let lo_mean = lo_sum / q as f32;
        let hi_mean = hi_sum / (n - q) as f32;

        let mut var = 0.0f32;
        for i in 0..q {
            let d = intensities[i] - lo_mean;
            var += d * d;
        }
        for i in q..n {
            let d = intensities[i] - hi_mean;
            var += d * d;
        }

        if var < best_var {
            best_var = var;
            best_split = q;
        }
    }

    // Map back to original point indices
    let lo_indices: Vec<usize> = indexed[..best_split].iter().map(|(_, idx)| *idx).collect();
    let hi_indices: Vec<usize> = indexed[best_split..].iter().map(|(_, idx)| *idx).collect();

    SplitByIntensity {
        split_idx: best_split,
        lo_indices,
        hi_indices,
    }
}

pub struct SplitByIntensity {
    pub split_idx: usize,
    pub lo_indices: Vec<usize>,
    pub hi_indices: Vec<usize>,
}
