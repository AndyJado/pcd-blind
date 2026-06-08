#!/bin/bash
set -e
D="$(cd "$(dirname "$0")" && pwd)"
P="$(cd "$D/.." && pwd)"
# hack: replace paths in preprocess binary's hardcoded strings won't work
# just compile+run directly
cat > /tmp/pre_q.cpp << 'EOF'
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/segmentation/sac_segmentation.h>
#include <pcl/segmentation/region_growing.h>
#include <pcl/features/normal_3d.h>
#include <pcl/search/kdtree.h>
#include <iostream>
#include <vector>
#include <algorithm>
int main() {
    pcl::PointCloud<pcl::PointXYZI>::Ptr si(new pcl::PointCloud<pcl::PointXYZI>);
    pcl::io::loadPCDFile<pcl::PointXYZI>("/home/mz/mainframer/pcd-blind/source/庆元陈家岭隧道3.pcd",*si);
    pcl::PointCloud<pcl::PointXYZI>::Ptr ds(new pcl::PointCloud<pcl::PointXYZI>);
    pcl::VoxelGrid<pcl::PointXYZI> vg;
    vg.setInputCloud(si);vg.setLeafSize(0.05,0.05,0.05);vg.filter(*ds);
    pcl::PointCloud<pcl::PointXYZ>::Ptr xyz(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::copyPointCloud(*ds,*xyz);
    std::cout<<"Voxel:"<<ds->size()<<"\n";
    {pcl::SACSegmentation<pcl::PointXYZ> seg;seg.setOptimizeCoefficients(true);seg.setModelType(pcl::SACMODEL_PERPENDICULAR_PLANE);seg.setAxis(Eigen::Vector3f(0,0,1));seg.setEpsAngle(15.0/180.0*M_PI);seg.setMethodType(pcl::SAC_RANSAC);seg.setDistanceThreshold(0.05);seg.setMaxIterations(500);seg.setInputCloud(xyz);pcl::PointIndices::Ptr inl(new pcl::PointIndices);pcl::ModelCoefficients::Ptr cf(new pcl::ModelCoefficients);seg.segment(*inl,*cf);std::cout<<"Ground:"<<inl->indices.size()<<"\n";pcl::ExtractIndices<pcl::PointXYZ> ex;ex.setInputCloud(xyz);ex.setIndices(inl);ex.setNegative(true);pcl::PointCloud<pcl::PointXYZ>::Ptr t(new pcl::PointCloud<pcl::PointXYZ>);ex.filter(*t);xyz.swap(t);pcl::ExtractIndices<pcl::PointXYZI> exi;exi.setInputCloud(ds);exi.setIndices(inl);exi.setNegative(true);pcl::PointCloud<pcl::PointXYZI>::Ptr ti(new pcl::PointCloud<pcl::PointXYZI>);exi.filter(*ti);ds.swap(ti);}
    {pcl::PointCloud<pcl::Normal>::Ptr n(new pcl::PointCloud<pcl::Normal>);pcl::search::KdTree<pcl::PointXYZ>::Ptr tr(new pcl::search::KdTree<pcl::PointXYZ>);pcl::NormalEstimation<pcl::PointXYZ,pcl::Normal> ne;ne.setInputCloud(xyz);ne.setSearchMethod(tr);ne.setRadiusSearch(0.15);ne.compute(*n);pcl::RegionGrowing<pcl::PointXYZ,pcl::Normal> rg;rg.setMinClusterSize(50);rg.setMaxClusterSize(10000000);rg.setSearchMethod(tr);rg.setNumberOfNeighbours(30);rg.setInputCloud(xyz);rg.setInputNormals(n);rg.setSmoothnessThreshold(4.0/180.0*M_PI);rg.setCurvatureThreshold(1.0);std::vector<pcl::PointIndices> regions;rg.extract(regions);std::vector<std::pair<int,int>> rk;for(size_t i=0;i<regions.size();i++)rk.push_back({(int)regions[i].indices.size(),(int)i});std::sort(rk.rbegin(),rk.rend());int pn=std::min(3,(int)rk.size());std::cout<<"Peel:"<<pn;for(int k=0;k<pn;k++)std::cout<<" "<<rk[k].first;std::cout<<"\n";std::vector<bool> keep(xyz->size(),true);for(int k=0;k<pn;k++)for(auto i:regions[rk[k].second].indices)keep[i]=false;pcl::PointCloud<pcl::PointXYZ>::Ptr t(new pcl::PointCloud<pcl::PointXYZ>);pcl::PointCloud<pcl::PointXYZI>::Ptr ti(new pcl::PointCloud<pcl::PointXYZI>);for(size_t i=0;i<xyz->size();i++)if(keep[i])t->push_back(xyz->points[i]);for(size_t i=0;i<ds->size();i++)if(keep[i])ti->push_back(ds->points[i]);xyz.swap(t);ds.swap(ti);std::cout<<"Remain:"<<ds->size()<<"\n";}
    pcl::io::savePCDFileBinary("/home/mz/mainframer/pcd-blind/output/ql3_preprocessed.pcd",*ds);
    std::cout<<"Saved\n";
}
EOF
g++ -O2 /tmp/pre_q.cpp -o /tmp/pre_q $(pkg-config --cflags --libs pcl_common-1.14 pcl_io-1.14 pcl_filters-1.14 pcl_segmentation-1.14 pcl_features-1.14 pcl_search-1.14 pcl_sample_consensus-1.14) 2>&1 && /tmp/pre_q
