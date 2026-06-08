/**
 * strip_ground.cpp — remove ground plane from templates
 * Ground: Z ≈ min_z, normal ≈ (0,0,1)
 */
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/segmentation/sac_segmentation.h>
#include <pcl/common/common.h>
#include <iostream>
#include <cmath>

void remove_ground(const char* in_path, const char* out_path) {
    pcl::PointCloud<pcl::PointXYZI>::Ptr c(new pcl::PointCloud<pcl::PointXYZI>);
    pcl::io::loadPCDFile<pcl::PointXYZI>(in_path, *c);
    if (c->empty()) return;

    pcl::PointCloud<pcl::PointXYZ>::Ptr xyz(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::copyPointCloud(*c, *xyz);

    // Find min Z → ground is near bottom
    pcl::PointXYZI mn, mx;
    pcl::getMinMax3D(*c, mn, mx);
    float ground_z = mn.z;
    std::cout << "  range Z: " << mn.z << " to " << mx.z << "  ground_z≈" << ground_z << "\n";

    // SAC for HORIZONTAL plane only (ground)
    pcl::SACSegmentation<pcl::PointXYZ> seg;
    seg.setOptimizeCoefficients(true);
    seg.setModelType(pcl::SACMODEL_PERPENDICULAR_PLANE);
    seg.setAxis(Eigen::Vector3f(0, 0, 1));  // force horizontal
    seg.setEpsAngle(15.0 / 180.0 * M_PI);    // ±15° from horizontal
    seg.setMethodType(pcl::SAC_RANSAC);
    seg.setDistanceThreshold(0.02);
    seg.setMaxIterations(1000);
    seg.setInputCloud(xyz);

    pcl::PointIndices::Ptr inl(new pcl::PointIndices);
    pcl::ModelCoefficients::Ptr cf(new pcl::ModelCoefficients);
    seg.segment(*inl, *cf);

    float nx = cf->values[0], ny = cf->values[1], nz = cf->values[2];
    float d  = cf->values[3];
    std::cout << "  SAC plane: n=(" << nx << "," << ny << "," << nz << ") d=" << d << "\n";

    // Check if it's ground: normal near vertical AND plane near bottom
    float verticality = std::abs(nz);
    if (verticality < 0.8) {
        std::cout << "  WARNING: not vertical enough, skip\n";
        pcl::io::savePCDFileBinary(out_path, *c);
        return;
    }

    // Only remove points near this plane
    pcl::ExtractIndices<pcl::PointXYZI> ex;
    ex.setInputCloud(c);
    ex.setIndices(inl);
    ex.setNegative(true);
    pcl::PointCloud<pcl::PointXYZI>::Ptr noground(new pcl::PointCloud<pcl::PointXYZI>);
    ex.filter(*noground);

    std::cout << "  removed " << inl->indices.size() << " ground pts, " << noground->size() << " remain\n";
    pcl::io::savePCDFileBinary(out_path, *noground);
}

int main() {
    const char* dir = "/home/mz/mainframer/pcd-blind/output/targets";
    const char* dir2 = "/home/mz/mainframer/pcd-blind/output/targets";
    remove_ground((std::string(dir)+"/t1_33_8.pcd").c_str(), (std::string(dir2)+"/t1_noground.pcd").c_str());
    remove_ground((std::string(dir)+"/t2_19_-4.pcd").c_str(), (std::string(dir2)+"/t2_noground.pcd").c_str());
    remove_ground((std::string(dir)+"/t3_33_-4.5.pcd").c_str(), (std::string(dir2)+"/t3_noground.pcd").c_str());
    remove_ground((std::string(dir)+"/t4_22_8.pcd").c_str(), (std::string(dir2)+"/t4_noground.pcd").c_str());
    std::cout << "Done.\n";
}
