#!/bin/bash
set -e
D="$(cd "$(dirname "$0")" && pwd)"
P="$(cd "$D/.." && pwd)"

cat > /tmp/show_ground.cpp << 'EOF'
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/segmentation/sac_segmentation.h>
#include <iostream>

int main() {
    pcl::PointCloud<pcl::PointXYZI>::Ptr si(new pcl::PointCloud<pcl::PointXYZI>);
    pcl::io::loadPCDFile<pcl::PointXYZI>("/home/mz/mainframer/pcd-blind/source/庆元陈家岭隧道1.pcd",*si);

    // voxel for speed
    pcl::PointCloud<pcl::PointXYZI>::Ptr ds(new pcl::PointCloud<pcl::PointXYZI>);
    pcl::VoxelGrid<pcl::PointXYZI> vg;
    vg.setInputCloud(si); vg.setLeafSize(0.05,0.05,0.05); vg.filter(*ds);
    pcl::PointCloud<pcl::PointXYZ>::Ptr xyz(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::copyPointCloud(*ds,*xyz);
    std::cout << "Voxel: " << xyz->size() << " pts\n";

    // Find ground plane
    pcl::SACSegmentation<pcl::PointXYZ> seg;
    seg.setOptimizeCoefficients(true);
    seg.setModelType(pcl::SACMODEL_PERPENDICULAR_PLANE);
    seg.setAxis(Eigen::Vector3f(0,0,1));
    seg.setEpsAngle(15.0/180.0*M_PI);
    seg.setMethodType(pcl::SAC_RANSAC);
    seg.setDistanceThreshold(0.05);
    seg.setMaxIterations(500);
    seg.setInputCloud(xyz);

    pcl::PointIndices::Ptr inl(new pcl::PointIndices);
    pcl::ModelCoefficients::Ptr cf(new pcl::ModelCoefficients);
    seg.segment(*inl,*cf);
    float gz = -(cf->values[3])/(cf->values[2]+0.0001f);
    if(cf->values[2]<0) gz=-gz;
    std::cout << "Ground SAC Z=" << gz << " inliers=" << inl->indices.size() << "\n";

    // Also find roof
    seg.setAxis(Eigen::Vector3f(0,0,-1));
    pcl::PointIndices::Ptr roof_inl(new pcl::PointIndices);
    seg.segment(*roof_inl,*cf);
    float rz = -(cf->values[3])/(cf->values[2]+0.0001f);
    if(cf->values[2]>0) rz=-rz;
    std::cout << "Roof SAC Z=" << rz << " inliers=" << roof_inl->indices.size() << "\n";

    // Extract ground inliers for visualization
    pcl::ExtractIndices<pcl::PointXYZI> ex;
    ex.setInputCloud(ds);
    ex.setIndices(inl);
    pcl::PointCloud<pcl::PointXYZI>::Ptr ground(new pcl::PointCloud<pcl::PointXYZI>);
    ex.filter(*ground);
    pcl::io::savePCDFileBinary("/home/mz/mainframer/pcd-blind/output/ground_check.pcd",*ground);
    std::cout << "Saved " << ground->size() << " ground pts to output/ground_check.pcd\n";

    // Roof
    ex.setIndices(roof_inl);
    pcl::PointCloud<pcl::PointXYZI>::Ptr roof(new pcl::PointCloud<pcl::PointXYZI>);
    ex.filter(*roof);
    pcl::io::savePCDFileBinary("/home/mz/mainframer/pcd-blind/output/roof_check.pcd",*roof);
    std::cout << "Saved " << roof->size() << " roof pts to output/roof_check.pcd\n";
}
EOF
g++ -O2 /tmp/show_ground.cpp -o /tmp/show_ground $(pkg-config --cflags --libs pcl_common-1.14 pcl_io-1.14 pcl_filters-1.14 pcl_segmentation-1.14 pcl_sample_consensus-1.14) 2>&1 && /tmp/show_ground
