//! 2D XY-projection screenshot renderer for match PCDs.
//! Replaces the Python numpy+matplotlib dependency.

use std::fs;
use std::path::{Path, PathBuf};

use anyhow::{Context, Result};
use image::{ImageBuffer, Rgb, RgbImage};

use crate::pcd;
use crate::types::PointI;

/// Width/height of the output image in pixels.
const IMG_SIZE: u32 = 600;
/// Circle radius in meters (0.1m = standard ball radius).
const BALL_R: f32 = 0.1;
/// Padding around the point cloud in meters.
const PADDING: f32 = 0.3;

/// Entry point: generate screenshots for all detections in a run output directory.
pub fn render_screenshots(output_dir: &Path) -> Result<()> {
    let results_path = output_dir.join("results.csv");
    if !results_path.exists() {
        return Ok(());
    }

    let ss_dir = output_dir.join("screenshots");
    fs::create_dir_all(&ss_dir)?;

    let detections = parse_results_csv(&results_path)?;
    if detections.is_empty() {
        return Ok(());
    }

    // Build a lookup: match PCD stem → path
    let match_files = find_match_pcds(output_dir)?;

    for (rank, (cx, cy, cz)) in detections.iter().enumerate() {
        // Find the match PCD whose hi-centroid is closest to this detection
        let best = find_best_match(&match_files, *cx, *cy)?;
        if let Some((path, _)) = best {
            let pts = pcd::read_pcd(&path)?;
            let out = ss_dir.join(format!("{:02}.png", rank));
            render_projection(&pts, *cx, *cy, *cz, &out)?;
        }
    }

    eprintln!("  Generated {} screenshots → {}", detections.len(), ss_dir.display());
    Ok(())
}

/// Parse results.csv → Vec<(cx, cy, cz)>
fn parse_results_csv(path: &Path) -> Result<Vec<(f32, f32, f32)>> {
    let text = fs::read_to_string(path)?;
    let mut dets = Vec::new();
    for (i, line) in text.lines().enumerate() {
        if i == 0 {
            continue; // header
        }
        let cols: Vec<&str> = line.split(',').collect();
        if cols.len() < 10 {
            continue;
        }
        let cx: f32 = cols[7].parse().unwrap_or(0.0);
        let cy: f32 = cols[8].parse().unwrap_or(0.0);
        let cz: f32 = cols[9].parse().unwrap_or(0.0);
        dets.push((cx, cy, cz));
    }
    Ok(dets)
}

/// Find all match_*.pcd files in the output directory.
fn find_match_pcds(dir: &Path) -> Result<Vec<(PathBuf, (f32, f32))>> {
    let mut results = Vec::new();
    for entry in fs::read_dir(dir)? {
        let entry = entry?;
        let name = entry.file_name();
        let name_str = name.to_string_lossy();
        if name_str.starts_with("match_") && name_str.ends_with(".pcd") {
            let path = entry.path();
            // Compute hi-point centroid of this match file
            if let Ok(pts) = pcd::read_pcd(&path) {
                let (hx, hy) = hi_centroid(&pts);
                results.push((path, (hx, hy)));
            }
        }
    }
    Ok(results)
}

/// Find the match PCD whose hi-centroid is closest to (cx, cy).
fn find_best_match(
    matches: &[(PathBuf, (f32, f32))],
    cx: f32,
    cy: f32,
) -> Result<Option<(PathBuf, (f32, f32))>> {
    let mut best: Option<(PathBuf, (f32, f32))> = None;
    let mut best_dist = f32::MAX;
    for (path, (hx, hy)) in matches {
        let d = ((hx - cx).powi(2) + (hy - cy).powi(2)).sqrt();
        if d < best_dist {
            best_dist = d;
            best = Some((path.clone(), (*hx, *hy)));
        }
    }
    // Only accept if within 0.8m
    if best_dist < 0.8 {
        Ok(best)
    } else {
        Ok(None)
    }
}

/// Compute centroid of hi-intensity points (intensity >= 200).
fn hi_centroid(pts: &[PointI]) -> (f32, f32) {
    let mut sx = 0.0f32;
    let mut sy = 0.0f32;
    let mut n = 0u32;
    for p in pts {
        if p.intensity > 200.0 {
            sx += p.x;
            sy += p.y;
            n += 1;
        }
    }
    if n > 0 {
        (sx / n as f32, sy / n as f32)
    } else {
        (0.0, 0.0)
    }
}

/// Render an isometric 3D projection of the point cloud into a PNG.
/// Projects (x,y,z) at a 45° horizontal / 25° vertical angle for best visual intuition.
fn render_projection(pts: &[PointI], cx: f32, cy: f32, cz: f32, out: &Path) -> Result<()> {
    // Isometric projection angles
    let angle_h: f32 = 45.0_f32.to_radians(); // horizontal rotation
    let angle_v: f32 = 25.0_f32.to_radians(); // vertical tilt
    let ch = angle_h.cos();
    let sh = angle_h.sin();
    let cv = angle_v.cos();
    let sv = angle_v.sin();

    // Project 3D → 2D screen
    let project = |x: f32, y: f32, z: f32| -> (f32, f32) {
        let sx = (x - y) * ch;
        let sy = (x + y) * sh * cv - z * sv;
        (sx, sy)
    };

    // Gather all projected points to find bounds
    let (mut smin_x, mut smax_x) = (f32::MAX, f32::MIN);
    let (mut smin_y, mut smax_y) = (f32::MAX, f32::MIN);
    let mut proj: Vec<(f32, f32, f32)> = Vec::with_capacity(pts.len());
    for p in pts {
        let (sx, sy) = project(p.x, p.y, p.z);
        proj.push((sx, sy, p.intensity));
        if sx < smin_x { smin_x = sx; }
        if sx > smax_x { smax_x = sx; }
        if sy < smin_y { smin_y = sy; }
        if sy > smax_y { smax_y = sy; }
    }

    // Also project the detection center
    let (scx, scy) = project(cx, cy, cz);

    // Pad and scale to square image
    let pad_s = PADDING * 1.5; // slightly more padding for 3D view
    smin_x -= pad_s; smax_x += pad_s;
    smin_y -= pad_s; smax_y += pad_s;
    let xrange = smax_x - smin_x;
    let yrange = smax_y - smin_y;
    let scale_s = IMG_SIZE as f32 / xrange.max(yrange);

    // Center the content in the image
    let offset_x = (IMG_SIZE as f32 - xrange * scale_s) / 2.0;
    let offset_y = (IMG_SIZE as f32 - yrange * scale_s) / 2.0;

    let to_px = |sx: f32, sy: f32| -> (u32, u32) {
        let px = ((sx - smin_x) * scale_s + offset_x) as u32;
        let py = ((sy - smin_y) * scale_s + offset_y) as u32;
        (px.min(IMG_SIZE - 1), py.min(IMG_SIZE - 1))
    };

    // Background: dark (#0d1117)
    let bg = Rgb([13u8, 17, 23]);
    let red = Rgb([255u8, 60, 40]);
    let blue = Rgb([60u8, 80, 200]);
    let blue_dim = Rgb([40u8, 55, 140]);
    let green = Rgb([0u8, 255, 136]);
    let orange = Rgb([232u8, 160, 48]);

    let mut img: RgbImage = ImageBuffer::from_pixel(IMG_SIZE, IMG_SIZE, bg);

    // Z-order sort: draw lo (far) first, hi (near) on top
    proj.sort_by(|a, b| b.2.partial_cmp(&a.2).unwrap_or(std::cmp::Ordering::Equal));

    for &(sx, sy, intensity) in &proj {
        let (px, py) = to_px(sx, sy);
        if intensity < 100.0 {
            img.put_pixel(px, py, blue_dim);
        } else if intensity <= 200.0 {
            img.put_pixel(px, py, orange);
        } else {
            // Hi points: draw 2x2 for visibility
            for dx in 0..2 {
                for dy in 0..2 {
                    let x = (px + dx).min(IMG_SIZE - 1);
                    let y = (py + dy).min(IMG_SIZE - 1);
                    img.put_pixel(x, y, red);
                }
            }
        }
    }

    // Draw detection circle and crosshair at projected center
    let (cx_px, cy_px) = to_px(scx, scy);
    let r_px = ((BALL_R * scale_s * 0.7) as i32).max(4); // circle scales with view
    draw_circle(&mut img, cx_px, cy_px, r_px, green);
    let cross_len = 6i32;
    for dx in -cross_len..=cross_len {
        let x = (cx_px as i32 + dx).clamp(0, IMG_SIZE as i32 - 1) as u32;
        img.put_pixel(x, cy_px, green);
    }
    for dy in -cross_len..=cross_len {
        let y = (cy_px as i32 + dy).clamp(0, IMG_SIZE as i32 - 1) as u32;
        img.put_pixel(cx_px, y, green);
    }

    img.save(out)?;
    Ok(())
}

/// Bresenham circle (outline only).
fn draw_circle(img: &mut RgbImage, cx: u32, cy: u32, r: i32, color: Rgb<u8>) {
    if r <= 0 {
        return;
    }
    let mut x: i32 = 0;
    let mut y: i32 = r;
    let mut d: i32 = 3 - 2 * r;
    let w = IMG_SIZE as i32;
    while y >= x {
        for (dx, dy) in &[
            (x, y), (y, x), (-x, y), (-y, x),
            (x, -y), (y, -x), (-x, -y), (-y, -x),
        ] {
            let px = (cx as i32 + dx).clamp(0, w - 1) as u32;
            let py = (cy as i32 + dy).clamp(0, w - 1) as u32;
            img.put_pixel(px, py, color);
        }
        x += 1;
        if d > 0 {
            y -= 1;
            d = d + 4 * (x - y) + 10;
        } else {
            d = d + 4 * x + 6;
        }
    }
}
