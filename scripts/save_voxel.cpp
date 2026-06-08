#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/filters/voxel_grid.h>
int main() {
    pcl::PointCloud<pcl::PointXYZI>::Ptr c(new pcl::PointCloud<pcl::PointXYZI>);
    pcl::io::loadPCDFile<pcl::PointXYZI>("source/浙江省计量院一楼实验室.pcd", *c);
    pcl::VoxelGrid<pcl::PointXYZI> vg;
    vg.setInputCloud(c); vg.setLeafSize(0.04,0.04,0.04);
    pcl::PointCloud<pcl::PointXYZI>::Ptr out(new pcl::PointCloud<pcl::PointXYZI>);
    vg.filter(*out);
    pcl::io::savePCDFileBinary("output/计量院_voxel_004.pcd", *out);
    std::cout << c->size() << " -> " << out->size() << std::endl;
}
