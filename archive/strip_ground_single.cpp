#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/segmentation/sac_segmentation.h>
#include <iostream>
int main(int argc, char** argv) {
    if (argc < 3) return 1;
    pcl::PointCloud<pcl::PointXYZI>::Ptr c(new pcl::PointCloud<pcl::PointXYZI>);
    pcl::io::loadPCDFile<pcl::PointXYZI>(argv[1], *c);
    pcl::PointCloud<pcl::PointXYZ>::Ptr xyz(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::copyPointCloud(*c, *xyz);
    pcl::SACSegmentation<pcl::PointXYZ> seg;
    seg.setOptimizeCoefficients(true);
    seg.setModelType(pcl::SACMODEL_PERPENDICULAR_PLANE);
    seg.setAxis(Eigen::Vector3f(0,0,1));
    seg.setEpsAngle(15.0/180.0*M_PI);
    seg.setMethodType(pcl::SAC_RANSAC);
    seg.setDistanceThreshold(0.02); seg.setMaxIterations(500);
    seg.setInputCloud(xyz);
    pcl::PointIndices::Ptr inl(new pcl::PointIndices);
    pcl::ModelCoefficients::Ptr cf(new pcl::ModelCoefficients);
    seg.segment(*inl, *cf);
    std::cout << "Ground: " << inl->indices.size() << " removed\n";
    pcl::ExtractIndices<pcl::PointXYZI> ex;
    ex.setInputCloud(c); ex.setIndices(inl); ex.setNegative(true);
    pcl::PointCloud<pcl::PointXYZI>::Ptr out(new pcl::PointCloud<pcl::PointXYZI>);
    ex.filter(*out);
    pcl::io::savePCDFileBinary(argv[2], *out);
    std::cout << "Saved " << out->size() << " pts\n";
}
