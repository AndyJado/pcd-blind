// peel_tunnel_v2.cpp — RegionGrowing peel, preserve intensity
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/features/normal_3d.h>
#include <pcl/segmentation/region_growing.h>
#include <pcl/search/kdtree.h>
#include <iostream>
#include <algorithm>
#include <vector>

int main() {
    // Load with intensity
    pcl::PointCloud<pcl::PointXYZI>::Ptr si(new pcl::PointCloud<pcl::PointXYZI>);
    pcl::io::loadPCDFile<pcl::PointXYZI>(
        "/home/mz/mainframer/pcd-blind/source/绍兴白峰岭隧道.pcd", *si);

    // Voxel 5cm, keep XYZI
    pcl::PointCloud<pcl::PointXYZI>::Ptr ds(new pcl::PointCloud<pcl::PointXYZI>);
    pcl::VoxelGrid<pcl::PointXYZI> vg;
    vg.setInputCloud(si); vg.setLeafSize(0.05,0.05,0.05); vg.filter(*ds);
    std::cout << "Voxel: " << ds->size() << " pts\n";

    // Use XYZ for normal+region growing, XYZI for final output
    pcl::PointCloud<pcl::PointXYZ>::Ptr xyz(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::copyPointCloud(*ds, *xyz);

    pcl::PointCloud<pcl::Normal>::Ptr n(new pcl::PointCloud<pcl::Normal>);
    pcl::search::KdTree<pcl::PointXYZ>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZ>);
    pcl::NormalEstimation<pcl::PointXYZ, pcl::Normal> ne;
    ne.setInputCloud(xyz); ne.setSearchMethod(tree); ne.setRadiusSearch(0.15); ne.compute(*n);

    pcl::RegionGrowing<pcl::PointXYZ, pcl::Normal> rg;
    rg.setMinClusterSize(50);
    rg.setMaxClusterSize(10000000);
    rg.setSearchMethod(tree);
    rg.setNumberOfNeighbours(30);
    rg.setInputCloud(xyz);
    rg.setInputNormals(n);
    rg.setSmoothnessThreshold(4.0 / 180.0 * M_PI);
    rg.setCurvatureThreshold(1.0);

    std::vector<pcl::PointIndices> regions;
    rg.extract(regions);
    std::cout << "Regions: " << regions.size() << "\n";

    // Find largest region
    int largest_idx = 0, largest_sz = 0;
    for (size_t i = 0; i < regions.size(); i++) {
        if ((int)regions[i].indices.size() > largest_sz) {
            largest_sz = regions[i].indices.size();
            largest_idx = i;
        }
    }
    std::cout << "Largest: R" << largest_idx << " = " << largest_sz << " pts\n";
    std::cout << "Top 10 others:\n";

    std::vector<std::pair<int,int>> ranked;
    for (size_t i = 0; i < regions.size(); i++)
        ranked.push_back({(int)regions[i].indices.size(), (int)i});
    std::sort(ranked.rbegin(), ranked.rend());
    for (int i = 0; i < std::min(10, (int)ranked.size()); i++)
        if (ranked[i].second != largest_idx)
            std::cout << "  R" << ranked[i].second << ": " << ranked[i].first << " pts\n";

    // Peel only the largest region
    pcl::PointIndices::Ptr peel_inl(new pcl::PointIndices);
    *peel_inl = regions[largest_idx];

    pcl::ExtractIndices<pcl::PointXYZI> ex;
    ex.setInputCloud(ds);
    ex.setIndices(peel_inl);
    ex.setNegative(true);
    pcl::PointCloud<pcl::PointXYZI>::Ptr peeled(new pcl::PointCloud<pcl::PointXYZI>);
    ex.filter(*peeled);

    std::cout << "\nPeeled (removed " << largest_sz << " pts): " << peeled->size() << " pts remain\n";
    pcl::io::savePCDFileBinary(
        "/home/mz/mainframer/pcd-blind/output/tunnel_peeled.pcd", *peeled);
    std::cout << "Saved tunnel_peeled.pcd\n";
}
