use anyhow::{Context, Result};
use std::fs::File;
use std::io::{BufRead, BufReader, Read, Write};
use std::path::Path;

use crate::types::PointI;

#[derive(Debug)]
struct PcdHeader {
    fields: Vec<String>,
    sizes: Vec<usize>,
    points: usize,
    data_type: String,
    width: usize,
    height: usize,
}

fn parse_header(path: &Path) -> Result<(PcdHeader, Vec<u8>)> {
    let file = File::open(path).context("open PCD")?;
    let mut reader = BufReader::new(file);

    let mut header = PcdHeader {
        fields: Vec::new(),
        sizes: Vec::new(),
        points: 0,
        data_type: String::new(),
        width: 0,
        height: 0,
    };

    let mut header_bytes = Vec::new();
    loop {
        let mut line = Vec::new();
        let n = reader.read_until(b'\n', &mut line)?;
        if n == 0 {
            anyhow::bail!("unexpected EOF in PCD header");
        }
        header_bytes.extend_from_slice(&line);
        let line_str = String::from_utf8_lossy(&line);
        let line_str = line_str.trim();

        if line_str.is_empty() || line_str.starts_with('#') {
            continue;
        }

        let parts: Vec<&str> = line_str.splitn(2, ' ').collect();
        if parts.len() < 2 {
            continue;
        }
        let key = parts[0];
        let value = parts[1];

        match key {
            "FIELDS" => header.fields = value.split_whitespace().map(|s| s.to_string()).collect(),
            "SIZE" => header.sizes = value.split_whitespace().filter_map(|s| s.parse().ok()).collect(),
            "POINTS" => header.points = value.parse().context("parse POINTS")?,
            "DATA" => {
                header.data_type = value.to_string();
                break; // header ends here
            }
            "WIDTH" => header.width = value.parse().unwrap_or(0),
            "HEIGHT" => header.height = value.parse().unwrap_or(0),
            _ => {}
        }
    }

    Ok((header, header_bytes))
}

/// Read a PCD file and return points with x, y, z, intensity.
/// Handles both 4-field (x,y,z,i) and 8-field (x,y,z,i,nx,ny,nz,curv) binary PCDs.
pub fn read_pcd(path: &Path) -> Result<Vec<PointI>> {
    let (header, header_bytes) = parse_header(path)?;

    // Find field indices for x, y, z, intensity
    let idx = |name: &str| -> Option<usize> {
        header
            .fields
            .iter()
            .position(|f| f == name)
    };

    let xi = idx("x").context("no x field")?;
    let yi = idx("y").context("no y field")?;
    let zi = idx("z").context("no z field")?;
    let ii = idx("intensity").context("no intensity field")?;

    // Compute byte stride and offsets
    let stride: usize = header.sizes.iter().sum();
    let offset = |field_idx: usize| -> usize {
        header.sizes[..field_idx].iter().sum()
    };

    let x_off = offset(xi);
    let y_off = offset(yi);
    let z_off = offset(zi);
    let i_off = offset(ii);

    if header.data_type != "binary" {
        anyhow::bail!("only binary PCD supported, got: {}", header.data_type);
    }

    let n = header.points;
    let data_size = n * stride;
    let header_len = header_bytes.len();

    // Read the entire binary data
    let mut file = File::open(path)?;
    let mut all_bytes = Vec::new();
    file.read_to_end(&mut all_bytes)?;

    if all_bytes.len() < header_len + data_size {
        anyhow::bail!(
            "truncated PCD: header={}, data needed={}, total={}",
            header_len,
            data_size,
            all_bytes.len()
        );
    }

    let data = &all_bytes[header_len..header_len + data_size];
    let mut points = Vec::with_capacity(n);

    for i in 0..n {
        let base = i * stride;
        let x = f32::from_le_bytes([
            data[base + x_off],
            data[base + x_off + 1],
            data[base + x_off + 2],
            data[base + x_off + 3],
        ]);
        let y = f32::from_le_bytes([
            data[base + y_off],
            data[base + y_off + 1],
            data[base + y_off + 2],
            data[base + y_off + 3],
        ]);
        let z = f32::from_le_bytes([
            data[base + z_off],
            data[base + z_off + 1],
            data[base + z_off + 2],
            data[base + z_off + 3],
        ]);
        let intensity = f32::from_le_bytes([
            data[base + i_off],
            data[base + i_off + 1],
            data[base + i_off + 2],
            data[base + i_off + 3],
        ]);
        points.push(PointI { x, y, z, intensity });
    }

    Ok(points)
}

/// Save points as a binary PCD with 4 fields (x, y, z, intensity).
pub fn save_pcd_binary(path: &Path, points: &[PointI]) -> Result<()> {
    let n = points.len();
    let mut f = File::create(path)?;

    // Write header
    writeln!(f, "# .PCD v0.7 - Point Cloud Data file format")?;
    writeln!(f, "VERSION 0.7")?;
    writeln!(f, "FIELDS x y z intensity")?;
    writeln!(f, "SIZE 4 4 4 4")?;
    writeln!(f, "TYPE F F F F")?;
    writeln!(f, "COUNT 1 1 1 1")?;
    writeln!(f, "WIDTH {}", n)?;
    writeln!(f, "HEIGHT 1")?;
    writeln!(f, "VIEWPOINT 0 0 0 1 0 0 0")?;
    writeln!(f, "POINTS {}", n)?;
    writeln!(f, "DATA binary")?;

    // Write binary data
    for p in points {
        f.write_all(&p.x.to_le_bytes())?;
        f.write_all(&p.y.to_le_bytes())?;
        f.write_all(&p.z.to_le_bytes())?;
        f.write_all(&p.intensity.to_le_bytes())?;
    }

    Ok(())
}
