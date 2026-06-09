use std::ops::{Index, IndexMut};

/// 3D point with intensity
#[derive(Debug, Clone, Copy, Default)]
pub struct PointI {
    pub x: f32,
    pub y: f32,
    pub z: f32,
    pub intensity: f32,
}

/// 3D point without intensity (for 2D projection, kd-tree)
#[derive(Debug, Clone, Copy, Default)]
pub struct Point3 {
    pub x: f32,
    pub y: f32,
    pub z: f32,
}

/// A box cloud: all points within a cylindrical/spatial region
#[derive(Debug, Clone)]
pub struct BoxCloud {
    pub points: Vec<PointI>,
}

/// Result of k-means intensity split
#[derive(Debug, Clone)]
pub struct KMeansSplit {
    /// split index (lo: [0..split), hi: [split..n))
    pub split_idx: usize,
    /// lo points (indices into BoxCloud.points)
    pub lo_indices: Vec<usize>,
    /// hi points (indices into BoxCloud.points)
    pub hi_indices: Vec<usize>,
    /// mean intensity of lo
    pub lo_mean: f32,
    /// mean intensity of hi
    pub hi_mean: f32,
}

/// Centroid of a set of points
#[derive(Debug, Clone, Copy, Default)]
pub struct Centroid {
    pub x: f32,
    pub y: f32,
    pub z: f32,
}

/// All features computed for a candidate
#[derive(Debug, Clone)]
pub struct FeatureSet {
    pub ratio: f32,
    pub tripod: f32,
    pub compact: f32,
    pub dxy: f32,
    pub hi_n: usize,
    pub lo_n: usize,
    pub total_n: usize,
    pub hi_centroid: Centroid,
    pub lo_centroid: Centroid,
    pub split_idx: usize,
}

/// A single detection result
#[derive(Debug, Clone)]
pub struct Detection {
    pub cx: f32,
    pub cy: f32,
    pub cz: f32,
    pub features: FeatureSet,
    pub score: f32,
}

impl BoxCloud {
    pub fn new(points: Vec<PointI>) -> Self {
        Self { points }
    }

    pub fn len(&self) -> usize {
        self.points.len()
    }

    pub fn is_empty(&self) -> bool {
        self.points.is_empty()
    }
}

impl Index<usize> for BoxCloud {
    type Output = PointI;
    fn index(&self, idx: usize) -> &PointI {
        &self.points[idx]
    }
}

impl IndexMut<usize> for BoxCloud {
    fn index_mut(&mut self, idx: usize) -> &mut PointI {
        &mut self.points[idx]
    }
}

impl Centroid {
    pub fn from_points(points: &[PointI]) -> Self {
        let n = points.len() as f32;
        if n == 0.0 {
            return Self::default();
        }
        let (mut sx, mut sy, mut sz) = (0.0f32, 0.0f32, 0.0f32);
        for p in points {
            sx += p.x;
            sy += p.y;
            sz += p.z;
        }
        Self {
            x: sx / n,
            y: sy / n,
            z: sz / n,
        }
    }

    pub fn from_points_by_index(points: &[PointI], indices: &[usize]) -> Self {
        let n = indices.len() as f32;
        if n == 0.0 {
            return Self::default();
        }
        let (mut sx, mut sy, mut sz) = (0.0f32, 0.0f32, 0.0f32);
        for &i in indices {
            let p = &points[i];
            sx += p.x;
            sy += p.y;
            sz += p.z;
        }
        Self {
            x: sx / n,
            y: sy / n,
            z: sz / n,
        }
    }

    pub fn xy_distance(&self, other: &Centroid) -> f32 {
        let dx = self.x - other.x;
        let dy = self.y - other.y;
        (dx * dx + dy * dy).sqrt()
    }
}
