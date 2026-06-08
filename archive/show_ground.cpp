#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/segmentation/sac_segmentation.h>
#include <iostream>
int main() {
    pcl::PointCloud<pcl::PointXYZI>::Ptr si(new pcl::PointCloud<pcl::PointXYZI>);
    pcl::io::loadPCDFile<pcl::PointXYZI>("/home/mz/mainframer/pcd-blind/source/庆元陈家岭隧道1.pcd",*si);
    pcl::PointCloud<pcl::PointXYZI>::Ptr ds(new pcl::PointCloud<pcl::PointXYZI>);
    pcl::VoxelGrid<pcl::PointXYZI> vg;vg.setInputCloud(si);vg.setLeafSize(0.05,0.05,0.05);vg.filter(*ds);
    pcl::PointCloud<pcl::PointXYZ>::Ptr xyz(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::copyPointCloud(*ds,*xyz);
    std::cout<<"Voxel: "<<xyz->size()<<" pts\n";
    pcl::SACSegmentation<pcl::PointXYZ> seg;seg.setOptimizeCoefficients(true);
    seg.setModelType(pcl::SACMODEL_PERPENDICULAR_PLANE);seg.setAxis(Eigen::Vector3f(0,0,1));
    seg.setEpsAngle(15.0/180.0*M_PI);seg.setMethodType(pcl::SAC_RANSAC);
    seg.setDistanceThreshold(0.15);seg.setMaxIterations(500);seg.setInputCloud(xyz);
    pcl::PointIndices::Ptr inl(new pcl::PointIndices);pcl::ModelCoefficients::Ptr cf(new pcl::ModelCoefficients);
    seg.segment(*inl,*cf);
    float gz=-(cf->values[3])/(cf->values[2]+0.0001f);if(cf->values[2]<0)gz=-gz;
    std::cout<<"Ground Z="<<gz<<" "<<inl->indices.size()<<" pts\n";
    pcl::ExtractIndices<pcl::PointXYZI> ex;ex.setInputCloud(ds);ex.setIndices(inl);
    pcl::PointCloud<pcl::PointXYZI>::Ptr g(new pcl::PointCloud<pcl::PointXYZI>);
    ex.filter(*g);pcl::io::savePCDFileBinary("/home/mz/mainframer/pcd-blind/output/ground_check.pcd",*g);
    std::cout<<"Saved ground_check.pcd\n";
    // roof
    seg.setAxis(Eigen::Vector3f(0,0,-1));
    pcl::PointIndices::Ptr ri(new pcl::PointIndices);seg.segment(*ri,*cf);
    float rz=-(cf->values[3])/(cf->values[2]+0.0001f);if(cf->values[2]>0)rz=-rz;
    std::cout<<"Roof Z="<<rz<<" "<<ri->indices.size()<<" pts\n";
    ex.setIndices(ri);pcl::PointCloud<pcl::PointXYZI>::Ptr r(new pcl::PointCloud<pcl::PointXYZI>);
    ex.filter(*r);pcl::io::savePCDFileBinary("/home/mz/mainframer/pcd-blind/output/roof_check.pcd",*r);
    std::cout<<"Saved roof_check.pcd\n";
}
