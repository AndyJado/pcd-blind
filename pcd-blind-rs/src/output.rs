use std::fs::File;
use std::io::Write;
use std::path::Path;

use crate::pcd::save_pcd_binary;
use crate::types::{Detection, PointI};

/// Write ALL candidates (pre-gate) for post-hoc filtering in spreadsheet.
pub fn write_candidates_csv(detections: &[Detection], out_dir: &Path) -> anyhow::Result<()> {
    let csv_path = out_dir.join("candidates.csv");
    let mut f = File::create(&csv_path)?;

    writeln!(
        f,
        "idx,ratio,compact,tripod,dxy,ground_frac,cx,cy,cz,hi_n,lo_n,total_n"
    )?;

    for (i, d) in detections.iter().enumerate() {
        let feat = &d.features;
        writeln!(
            f,
            "{},{:.4},{:.4},{:.4},{:.4},{:.4},{:.4},{:.4},{:.4},{},{},{}",
            i,
            feat.ratio,
            feat.compact,
            feat.tripod,
            feat.dxy,
            feat.ground_fraction,
            d.cx,
            d.cy,
            d.cz,
            feat.hi_n,
            feat.lo_n,
            feat.total_n,
        )?;
    }

    Ok(())
}

/// Write detection results to CSV
pub fn write_results_csv(detections: &[Detection], out_dir: &Path) -> anyhow::Result<()> {
    let csv_path = out_dir.join("results.csv");
    let mut f = File::create(&csv_path)?;

    writeln!(
        f,
        "rank,ratio,compact,tripod,dxy,ground_frac,score,cx,cy,cz,hi_n,lo_n,total_n"
    )?;

    for (i, d) in detections.iter().enumerate() {
        let feat = &d.features;
        writeln!(
            f,
            "{},{:.4},{:.4},{:.4},{:.4},{:.4},{:.4},{:.4},{:.4},{:.4},{},{},{}",
            i,
            feat.ratio,
            feat.compact,
            feat.tripod,
            feat.dxy,
            feat.ground_fraction,
            d.score,
            d.cx,
            d.cy,
            d.cz,
            feat.hi_n,
            feat.lo_n,
            feat.total_n,
        )?;
    }

    Ok(())
}

/// Save match PCDs for visualization in CloudCompare.
/// hi points get intensity=255 (bright), lo points get intensity=50 (dim).
pub fn save_match_pcd(
    box_points: &[PointI],
    hi_indices: &[usize],
    out_dir: &Path,
    index: usize,
    ratio: f32,
) -> anyhow::Result<()> {
    let mut colored: Vec<PointI> = Vec::with_capacity(box_points.len());
    let hi_set: std::collections::HashSet<usize> = hi_indices.iter().copied().collect();

    for (i, p) in box_points.iter().enumerate() {
        colored.push(PointI {
            x: p.x,
            y: p.y,
            z: p.z,
            intensity: if hi_set.contains(&i) { 255.0 } else { 50.0 },
        });
    }

    let path = out_dir.join(format!("match_{:02}_r{:.1}.pcd", index, ratio));
    save_pcd_binary(&path, &colored)?;

    Ok(())
}
