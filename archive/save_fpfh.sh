#!/bin/bash
# save_fpfh.sh — extract and save template FPFH features
set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
TPL="$PROJECT_DIR/output/绍兴白峰岭隧道_stripped/target_labeled.pcd"
FEATDIR="$PROJECT_DIR/output/fpfh_features"
mkdir -p "$FEATDIR"

cat > /tmp/save_fpfh.cpp << 'CPP'
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/features/normal_3d.h>
#include <pcl/features/fpfh.h>
#include <pcl/search/kdtree.h>
#include <fstream>
#include <iomanip>

int main(int argc, char** argv) {
    std::string in = argv[1], out_dir = argv[2];
    
    // load
    pcl::PointCloud<pcl::PointXYZI>::Ptr raw(new pcl::PointCloud<pcl::PointXYZI>);
    pcl::io::loadPCDFile<pcl::PointXYZI>(in, *raw);
    pcl::PointCloud<pcl::PointXYZ>::Ptr xyz(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::copyPointCloud(*raw, *xyz);
    
    // downsample 5cm
    pcl::PointCloud<pcl::PointXYZ>::Ptr ds(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::VoxelGrid<pcl::PointXYZ> vg;
    vg.setInputCloud(xyz); vg.setLeafSize(0.05,0.05,0.05); vg.filter(*ds);
    
    // normals
    pcl::PointCloud<pcl::Normal>::Ptr n(new pcl::PointCloud<pcl::Normal>);
    pcl::NormalEstimation<pcl::PointXYZ, pcl::Normal> ne;
    pcl::search::KdTree<pcl::PointXYZ>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZ>);
    ne.setInputCloud(ds); ne.setSearchMethod(tree); ne.setRadiusSearch(0.05); ne.compute(*n);
    
    // FPFH
    pcl::PointCloud<pcl::FPFHSignature33>::Ptr f(new pcl::PointCloud<pcl::FPFHSignature33>);
    pcl::FPFHEstimation<pcl::PointXYZ, pcl::Normal, pcl::FPFHSignature33> fpfh;
    fpfh.setInputCloud(ds); fpfh.setInputNormals(n); fpfh.setSearchMethod(tree);
    fpfh.setRadiusSearch(0.08); fpfh.compute(*f);
    
    std::cout << "Points: " << ds->size() << "  FPFH: " << f->size() << " x 33\n";
    
    // save XYZ points
    std::string pts_file = out_dir + std::string("/template_points.pcd");
    pcl::io::savePCDFileBinary(pts_file, *ds);
    
    // save FPFH as raw floats (points * 33)
    std::string feat_file = out_dir + std::string("/template_fpfh.bin");
    std::ofstream of(feat_file, std::ios::binary);
    for (size_t i = 0; i < f->size(); i++) {
        of.write(reinterpret_cast<const char*>(f->points[i].histogram), 33 * sizeof(float));
    }
    of.close();
    
    // save as text for inspection (first 10 points, first 11 bins)
    std::string txt_file = out_dir + std::string("/template_fpfh_preview.txt");
    std::ofstream tf(txt_file);
    tf << "Total: " << f->size() << " points x 33 dimensions\n";
    tf << "point x y z | fpfh[0..10]...\n";
    for (size_t i = 0; i < std::min(f->size(), (size_t)10); i++) {
        tf << std::fixed << std::setprecision(4);
        tf << i << " " << ds->points[i].x << " " << ds->points[i].y << " " << ds->points[i].z << " | ";
        for (int j = 0; j < 11; j++)
            tf << f->points[i].histogram[j] << " ";
        tf << "...\n";
    }
    tf.close();
    
    std::cout << "Saved:\n  " << pts_file << "\n  " << feat_file << " (" << f->size()*33*4 << " bytes)\n  " << txt_file << "\n";
}
CPP

g++ -O2 /tmp/save_fpfh.cpp -o /tmp/save_fpfh \
    $(pkg-config --cflags --libs pcl_common-1.14 pcl_io-1.14 pcl_features-1.14 pcl_filters-1.14 pcl_search-1.14) 2>&1
/tmp/save_fpfh "$TPL" "$FEATDIR"
