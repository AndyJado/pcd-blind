use pcd_blind::cluster::extract_clusters;
use pcd_blind::types::Point3;

#[test]
fn test_euclidean_cluster_two_blobs() {
    // Two well-separated blobs
    let mut points = Vec::new();
    // Blob 1: around (0,0)
    for i in 0..20 {
        let x = (i % 5) as f32 * 0.01;
        let y = (i / 5) as f32 * 0.01;
        points.push(Point3 { x, y, z: 0.0 });
    }
    // Blob 2: around (1,1)
    for i in 0..20 {
        let x = 1.0 + (i % 5) as f32 * 0.01;
        let y = 1.0 + (i / 5) as f32 * 0.01;
        points.push(Point3 { x, y, z: 0.0 });
    }

    let clusters = extract_clusters(&points, 0.15, 5, 1000);
    assert_eq!(clusters.len(), 2, "should find 2 clusters, got {}", clusters.len());
}

#[test]
fn test_euclidean_cluster_single_blob() {
    let mut points = Vec::new();
    for i in 0..30 {
        points.push(Point3 {
            x: (i as f32) * 0.02,
            y: 0.0,
            z: 0.0,
        });
    }
    let clusters = extract_clusters(&points, 0.15, 5, 1000);
    assert_eq!(clusters.len(), 1, "should find 1 cluster, got {}", clusters.len());
}

#[test]
fn test_pcd_read_write_roundtrip() {
    use pcd_blind::pcd::{read_pcd, save_pcd_binary};
    use pcd_blind::types::PointI;
    use std::path::Path;

    let points = vec![
        PointI { x: 1.0, y: 2.0, z: 3.0, intensity: 100.0 },
        PointI { x: 4.0, y: 5.0, z: 6.0, intensity: 200.0 },
    ];

    let tmp = std::env::temp_dir().join("test_roundtrip.pcd");
    save_pcd_binary(&tmp, &points).unwrap();
    let loaded = read_pcd(&tmp).unwrap();
    std::fs::remove_file(&tmp).ok();

    assert_eq!(loaded.len(), 2);
    assert!((loaded[0].x - 1.0).abs() < 0.01);
    assert!((loaded[0].intensity - 100.0).abs() < 0.01);
}

#[test]
fn test_feature_dxy() {
    use pcd_blind::features::dxy;
    use pcd_blind::types::Centroid;

    let lo = Centroid { x: 0.0, y: 0.0, z: 0.0 };
    let hi = Centroid { x: 3.0, y: 4.0, z: 0.0 };
    let d = dxy::compute(&lo, &hi);
    assert!((d - 5.0).abs() < 0.01, "dxy should be 5.0, got {}", d);
}

#[test]
fn test_ground_fraction() {
    use pcd_blind::ground::peel_ground;
    use pcd_blind::types::PointI;

    // Create a box with ground (low Z) and ball (high Z)
    let mut pts = Vec::new();
    // Ground: flat at z≈0
    for x in -5..=5 {
        for y in -5..=5 {
            let x = x as f32 * 0.05;
            let y = y as f32 * 0.05;
            pts.push(PointI { x, y, z: 0.01, intensity: 30.0 });
        }
    }
    // Ball: cluster at z≈0.3
    for _ in 0..50 {
        pts.push(PointI { x: 0.1, y: 0.0, z: 0.3, intensity: 200.0 });
    }

    let n_before = pts.len();
    let (cleaned, gf) = peel_ground(&pts, 0.0, 0.0, 0.35, 0.5, 0.05, 100, 0.3, 0.10);
    assert!(gf > 0.05, "ground fraction should be >0.05, got {}", gf);
    assert!(cleaned.len() < n_before, "should remove some ground points");
    // Ball points should survive
    let ball_count = cleaned.iter().filter(|p| p.intensity > 150.0).count();
    assert_eq!(ball_count, 50, "all 50 ball points should survive");
}
