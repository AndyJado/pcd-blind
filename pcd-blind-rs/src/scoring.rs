use crate::config::ScoringConfig;
use crate::types::Detection;

/// Gate candidates by feature thresholds, compute scores, dedup, sort.
pub fn score_and_filter(
    detections: &mut Vec<Detection>,
    features_cfg: &crate::config::FeatureConfig,
    scoring_cfg: &ScoringConfig,
) {
    // Gate: filter out detections that fail threshold checks
    detections.retain(|d| {
        let f = &d.features;
        f.ratio >= features_cfg.ratio_min
            && f.compact >= features_cfg.compact_min
            && f.dxy <= features_cfg.dxy_max_residual
            && f.ground_fraction >= features_cfg.ground_fraction_min
    });

    // Compute score
    for d in detections.iter_mut() {
        let f = &d.features;
        d.score = scoring_cfg.tripod_weight * f.tripod
            + scoring_cfg.ratio_weight * f.ratio
            + scoring_cfg.compact_weight * f.compact
            + scoring_cfg.dxy_weight * f.dxy
            + scoring_cfg.ground_weight * f.ground_fraction;
    }

    // Dedup: for detections within dedup_radius in XY+Z, keep the one with lowest dxy
    let mut i = 0;
    while i < detections.len() {
        let mut j = i + 1;
        while j < detections.len() {
            let dx = detections[i].cx - detections[j].cx;
            let dy = detections[i].cy - detections[j].cy;
            let dz = (detections[i].cz - detections[j].cz).abs();
            if dx * dx + dy * dy < scoring_cfg.dedup_radius * scoring_cfg.dedup_radius
                && dz < scoring_cfg.dedup_z
            {
                if detections[j].features.dxy < detections[i].features.dxy {
                    detections.swap(i, j);
                }
                detections.remove(j);
            } else {
                j += 1;
            }
        }
        i += 1;
    }

    // Sort by score descending
    detections.sort_by(|a, b| b.score.partial_cmp(&a.score).unwrap_or(std::cmp::Ordering::Equal));
}
