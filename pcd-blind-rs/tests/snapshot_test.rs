use pcd_blind::features;
use pcd_blind::pcd;
use pcd_blind::types::{BoxCloud, Centroid, FeatureSet};
use serde::Deserialize;
use std::collections::HashMap;
use std::path::Path;

type Snapshot = HashMap<String, Expected>;

#[derive(Debug, Deserialize)]
struct Expected {
    ratio: f64,
    compact: f64,
    tripod: f64,
    dxy: f64,
    #[serde(default)]
    ground_fraction: f64,
}

/// 8 known template PCDs with hand-verified feature values.
/// Any code change that shifts these values will fail the test.
#[test]
fn test_template_snapshots() {
    let snapshot: Snapshot =
        serde_json::from_str(include_str!("snapshot.json")).expect("parse snapshot");

    // Template PCDs on mz (relative to project root)
    let template_dir = Path::new(env!("CARGO_MANIFEST_DIR"))
        .parent()
        .unwrap();

    let templates = [
        "output/targets/bf_t1_colored.pcd",
        "output/targets/bf_t2_colored.pcd",
        "output/targets/bf_t3_colored.pcd",
        "output/targets/bf_t4_colored.pcd",
        "output/targets_ql3/qy1_t1.pcd",
        "output/targets_ql3/qy3_t1.pcd",
        "output/targets_ql3/qy3_t2.pcd",
        "output/targets_ql3/qy3_t3.pcd",
    ];

    let tol_ratio = 0.1;
    let tol_compact = 0.05;
    let tol_tripod = 0.1;
    let tol_dxy = 0.05;

    for tpl_path in &templates {
        let full_path = template_dir.join(tpl_path);
        let name = Path::new(tpl_path)
            .file_stem()
            .unwrap()
            .to_str()
            .unwrap();

        let expected = snapshot.get(name).unwrap_or_else(|| {
            panic!("no snapshot entry for {}", name);
        });

        // Load template PCD
        let points = pcd::read_pcd(&full_path).unwrap_or_else(|e| {
            panic!("failed to read {}: {}", tpl_path, e);
        });

        // Split by intensity (template convention: hi=255, lo=50)
        let mut lo_indices = Vec::new();
        let mut hi_indices = Vec::new();
        for (i, p) in points.iter().enumerate() {
            if p.intensity > 200.0 {
                hi_indices.push(i);
            } else if p.intensity < 100.0 {
                lo_indices.push(i);
            }
        }

        let box_cloud = BoxCloud::new(points);

        // Use default feature config
        let cfg = pcd_blind::config::FeatureConfig::default();

        let features = features::extract_features(&box_cloud, &lo_indices, &hi_indices, 0.0, &cfg);

        // Assert each feature within tolerance
        assert!(
            (features.ratio as f64 - expected.ratio).abs() < tol_ratio,
            "{} ratio: expected {:.3}, got {:.3}",
            name,
            expected.ratio,
            features.ratio
        );
        assert!(
            (features.compact as f64 - expected.compact).abs() < tol_compact,
            "{} compact: expected {:.3}, got {:.3}",
            name,
            expected.compact,
            features.compact
        );
        assert!(
            (features.tripod as f64 - expected.tripod).abs() < tol_tripod,
            "{} tripod: expected {:.3}, got {:.3}",
            name,
            expected.tripod,
            features.tripod
        );
        assert!(
            (features.dxy as f64 - expected.dxy).abs() < tol_dxy,
            "{} dxy: expected {:.3}, got {:.3}",
            name,
            expected.dxy,
            features.dxy
        );

        println!(
            "  ✓ {} ratio={:.2} tripod={:.3} compact={:.3} dxy={:.3}",
            name, features.ratio, features.tripod, features.compact, features.dxy
        );
    }

    println!("All 8 template snapshots match.");
}
