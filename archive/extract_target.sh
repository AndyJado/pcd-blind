#!/bin/bash
# Extract the labeled target region from original PCD
set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
OUTDIR="$PROJECT_DIR/output/绍兴白峰岭隧道_stripped"
mkdir -p "$OUTDIR"

cat > /tmp/extract_target.cpp << 'CPP'
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
int main() {
    pcl::PointCloud<pcl::PointXYZI>::Ptr in(new pcl::PointCloud<pcl::PointXYZI>);
    pcl::io::loadPCDFile<pcl::PointXYZI>(
        "/home/mz/mainframer/pcd-blind/source/绍兴白峰岭隧道.pcd", *in);

    // Bounding box from user's 3 picks with margin
    float xmin=32.5, xmax=33.5;
    float ymin=7.7,  ymax=8.5;
    float zmin=-1.0, zmax=1.0;

    pcl::PointCloud<pcl::PointXYZI>::Ptr out(new pcl::PointCloud<pcl::PointXYZI>);
    for (auto& p : in->points)
        if (p.x>=xmin && p.x<=xmax && p.y>=ymin && p.y<=ymax && p.z>=zmin && p.z<=zmax)
            out->push_back(p);

    pcl::io::savePCDFileBinary(
        "/home/mz/mainframer/pcd-blind/output/绍兴白峰岭隧道_stripped/target_labeled.pcd", *out);
    std::cout << "Extracted " << out->size() << " pts\n";
}
CPP

g++ -O2 /tmp/extract_target.cpp -o /tmp/extract_target $(pkg-config --cflags --libs pcl_io-1.14 pcl_common-1.14) 2>&1
/tmp/extract_target
echo "Done: target_labeled.pcd"
