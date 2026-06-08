#!/bin/bash
# strip_tunnel.sh — remove tunnel envelope, save result for inspection
set -e
D="$(cd "$(dirname "$0")" && pwd)"
P="$(cd "$D/.." && pwd)"

cat > /tmp/strip.cpp << 'CPP'
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/segmentation/sac_segmentation.h>
#include <iostream>
using namespace std;

int main() {
    pcl::PointCloud<pcl::PointXYZI>::Ptr si(new pcl::PointCloud<pcl::PointXYZI>);
    pcl::io::loadPCDFile<pcl::PointXYZI>(
        "/home/mz/mainframer/pcd-blind/source/绍兴白峰岭隧道.pcd", *si);
    pcl::PointCloud<pcl::PointXYZ>::Ptr s(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::copyPointCloud(*si, *s);
    
    // voxel 5cm
    pcl::PointCloud<pcl::PointXYZ>::Ptr ds(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::VoxelGrid<pcl::PointXYZ> vg;
    vg.setInputCloud(s); vg.setLeafSize(0.05,0.05,0.05); vg.filter(*ds);
    cout << ds->size() << " pts after voxel\n";
    
    pcl::PointCloud<pcl::PointXYZ>::Ptr w(new pcl::PointCloud<pcl::PointXYZ>);
    *w = *ds;
    
    int removed = 0;
    while (w->size() > 1000 && removed < 100) {
        pcl::SACSegmentation<pcl::PointXYZ> seg;
        seg.setOptimizeCoefficients(true);
        seg.setModelType(pcl::SACMODEL_PLANE);
        seg.setMethodType(pcl::SAC_RANSAC);
        seg.setDistanceThreshold(0.03);
        seg.setMaxIterations(500);
        seg.setInputCloud(w);
        pcl::PointIndices::Ptr inl(new pcl::PointIndices);
        pcl::ModelCoefficients::Ptr cf(new pcl::ModelCoefficients);
        seg.segment(*inl, *cf);
        
        double pct = 100.0 * inl->indices.size() / w->size();
        cout << "Plane " << removed << ": " << inl->indices.size()
             << " pts (" << pct << "%) n=("
             << cf->values[0] << "," << cf->values[1] << "," << cf->values[2] << ")\n";
        
        if (pct < 0.2) break;
        
        pcl::ExtractIndices<pcl::PointXYZ> ex;
        ex.setInputCloud(w); ex.setIndices(inl); ex.setNegative(true);
        pcl::PointCloud<pcl::PointXYZ>::Ptr t(new pcl::PointCloud<pcl::PointXYZ>);
        ex.filter(*t); w.swap(t);
        removed++;
    }
    
    cout << "\nRemaining: " << w->size() << " pts\n";
    
    // save with XYZI (intensity=0 for stripped points)
    pcl::PointCloud<pcl::PointXYZI>::Ptr out(new pcl::PointCloud<pcl::PointXYZI>);
    for (auto& p : w->points) {
        pcl::PointXYZI pt; pt.x=p.x; pt.y=p.y; pt.z=p.z; pt.intensity=0;
        out->push_back(pt);
    }
    pcl::io::savePCDFileBinary(
        "/home/mz/mainframer/pcd-blind/output/tunnel_stripped.pcd", *out);
    cout << "Saved tunnel_stripped.pcd\n";
}
CPP

g++ -O2 /tmp/strip.cpp -o /tmp/strip \
    $(pkg-config --cflags --libs pcl_common-1.14 pcl_io-1.14 pcl_filters-1.14 pcl_segmentation-1.14 pcl_sample_consensus-1.14) 2>&1
/tmp/strip
