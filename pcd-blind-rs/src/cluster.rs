use std::collections::VecDeque;

use crate::kdtree::KdTree3D;
use crate::types::Point3;

/// Euclidean cluster extraction.
/// Returns clusters as groups of point indices.
pub fn extract_clusters(
    points: &[Point3],
    tolerance: f32,
    min_size: usize,
    max_size: usize,
) -> Vec<Vec<usize>> {
    if points.is_empty() {
        return Vec::new();
    }

    let mut tree = KdTree3D::new();
    tree.build(points);

    let n = points.len();
    let mut visited = vec![false; n];
    let mut clusters = Vec::new();

    for i in 0..n {
        if visited[i] {
            continue;
        }
        visited[i] = true;

        let mut cluster = Vec::new();
        let mut queue = VecDeque::new();
        queue.push_back(i);

        while let Some(cur) = queue.pop_front() {
            cluster.push(cur);
            let center = [points[cur].x, points[cur].y, points[cur].z];
            for &nb in &tree.radius_search(&center, tolerance) {
                if !visited[nb] {
                    visited[nb] = true;
                    queue.push_back(nb);
                }
            }
        }

        if cluster.len() >= min_size && cluster.len() <= max_size {
            clusters.push(cluster);
        }
    }

    clusters
}
