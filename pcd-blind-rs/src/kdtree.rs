use kiddo::ImmutableKdTree;

use crate::types::Point3;

pub struct KdTree3D {
    tree: Option<ImmutableKdTree<f32, 3>>,
}

impl KdTree3D {
    pub fn new() -> Self {
        Self { tree: None }
    }

    /// Build tree from points (z is used as 3rd dim, even for 2D data z=0)
    pub fn build(&mut self, pts: &[Point3]) {
        let coords: Vec<[f32; 3]> = pts.iter().map(|p| [p.x, p.y, p.z]).collect();
        self.tree = Some(ImmutableKdTree::new_from_slice(&coords));
    }

    /// Radius search: find indices of all points within `radius` of `center`
    pub fn radius_search(&self, center: &[f32; 3], radius: f32) -> Vec<usize> {
        match &self.tree {
            Some(tree) => {
                let neighbors = tree.within::<kiddo::SquaredEuclidean>(center, radius);
                neighbors.iter().map(|n| n.item as usize).collect()
            }
            None => Vec::new(),
        }
    }
}
