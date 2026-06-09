use std::path::PathBuf;

use anyhow::Result;
use clap::Parser;

use pcd_blind::candidates;
use pcd_blind::config::Config;
use pcd_blind::features;
use pcd_blind::output;
use pcd_blind::pcd;
use pcd_blind::recenter::{self, RecenterConfig};
use pcd_blind::scoring;
use pcd_blind::types::Detection;

#[derive(Parser)]
#[command(name = "pcd-blind")]
#[command(about = "Target ball detection in tunnel point clouds")]
struct Cli {
    /// Input PCD file (scene)
    scene: PathBuf,

    /// Output directory
    out: PathBuf,

    /// Config file (TOML)
    #[arg(short, long, default_value = "config/default.toml")]
    config: PathBuf,
}

fn main() -> Result<()> {
    let cli = Cli::parse();

    // Load config
    let cfg = if cli.config.exists() {
        Config::load(&cli.config)?
    } else {
        Config::default()
    };

    // Create output directory
    std::fs::create_dir_all(&cli.out)?;

    // --- Layer 1: Load & candidate generation ---
    eprintln!("Loading {}...", cli.scene.display());
    let full_cloud = pcd::read_pcd(&cli.scene)?;
    eprintln!("  {} points", full_cloud.len());

    // Intensity threshold (Pxx)
    let mut intensities: Vec<f32> = full_cloud.iter().map(|p| p.intensity).collect();
    intensities.sort_by(|a, b| a.partial_cmp(b).unwrap_or(std::cmp::Ordering::Equal));
    let pct_idx = ((intensities.len() as f32) * cfg.detect.pct / 100.0) as usize;
    let i_threshold = intensities[pct_idx.min(intensities.len() - 1)];
    eprintln!("  I > P{} = {:.2}", cfg.detect.pct as u32, i_threshold);

    eprintln!("Generating candidates (Z-slice + XY cluster + merge)...");
    let candidates = candidates::generate_candidates(
        &full_cloud,
        i_threshold,
        cfg.candidates.z_start,
        cfg.candidates.z_step,
        cfg.candidates.z_end,
        cfg.candidates.cluster_tolerance,
        cfg.candidates.min_cluster_size,
        cfg.detect.ball_diameter,
    );
    eprintln!("  {} candidates after slice merge", candidates.len());

    // --- Layer 2: Recenter + Feature extraction ---
    let ground_cfg = recenter::GroundConfig {
        outer_ring_min: cfg.ground.outer_ring_min,
        outer_ring_max: cfg.ground.outer_ring_max,
        ransac_distance: cfg.ground.ransac_distance,
        ransac_iters: cfg.ground.ransac_iters,
        wall_nz_max: cfg.ground.wall_nz_max,
        ground_z_pct: cfg.ground.ground_z_pct,
    };
    let recenter_config = RecenterConfig {
        max_iters: cfg.recenter.max_iters,
        dxy_threshold: cfg.recenter.dxy_threshold,
    };

    let mut detections: Vec<Detection> = Vec::new();

    for (idx, cand) in candidates.iter().enumerate() {
        let result = recenter::recenter(
            &full_cloud,
            cand.cx,
            cand.cy,
            cand.z_center,
            cfg.ground.cyl_radius,
            cfg.ground.box_dz_dn,
            cfg.ground.box_dz_up,
            &ground_cfg,
            &recenter_config,
        );

        if let Some(r) = result {
            let features = features::extract_features(
                &r.box_cloud,
                &r.lo_indices,
                &r.hi_indices,
                r.ground_fraction,
                &cfg.features,
            );

            let cz = r.hi_centroid.z;

            detections.push(Detection {
                cx: r.cx,
                cy: r.cy,
                cz,
                features,
                score: 0.0,
            });

            // Save match PCD
            if cfg.output.save_matches {
                let _ = output::save_match_pcd(
                    &r.box_cloud.points,
                    &r.hi_indices,
                    &cli.out,
                    idx,
                    r.dxy,
                );
            }

            let f = &detections.last().unwrap().features;
            eprintln!(
                "  [{:3}] ({} iters, {}) ratio={:.1} tripod={:.3} compact={:.3} dxy={:.3} ground={:.2}",
                idx,
                r.iterations,
                if r.converged { "ok" } else { "max" },
                f.ratio,
                f.tripod,
                f.compact,
                f.dxy,
                f.ground_fraction,
            );
        }
    }

    // Save all candidates (pre-gate) for post-hoc filtering
    output::write_candidates_csv(&detections, &cli.out)?;

    // Gate, score, dedup, sort
    scoring::score_and_filter(&mut detections, &cfg.features, &cfg.scoring);

    // Save gated results
    output::write_results_csv(&detections, &cli.out)?;
    eprintln!(
        "\nDone. {} detections → {}",
        candidates.len(),
        detections.len()
    );
    eprintln!("Results: {}/results.csv", cli.out.display());

    Ok(())
}
