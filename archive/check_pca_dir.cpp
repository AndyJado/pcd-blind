// check_pca_dir.cpp — check principal axis direction + centroid offset
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/common/pca.h>
#include <pcl/common/common.h>
#include <pcl/common/centroid.h>
#include <iostream>
#include <cmath>
#include <iomanip>
#include <algorithm>

int main() {
    const char* files[] = {
        "/home/mz/mainframer/pcd-blind/output/targets/t1_noground.pcd",
        "/home/mz/mainframer/pcd-blind/output/targets/t2_noground.pcd",
        "/home/mz/mainframer/pcd-blind/output/targets/t3_noground.pcd",
        "/home/mz/mainframer/pcd-blind/output/targets/t4_noground.pcd",
    };
    const char* names[] = {"T1", "T2", "T3", "T4"};

    std::cout << std::fixed << std::setprecision(3);
    std::cout << "ID  n_pts  H(m)  l1/l2  l2/l3  λ1_dir(x,y,z)  Zoff/H\n";
    std::cout << "------------------------------------------------------\n";

    for (int i = 0; i < 4; i++) {
        pcl::PointCloud<pcl::PointXYZ>::Ptr c(new pcl::PointCloud<pcl::PointXYZ>);
        pcl::io::loadPCDFile<pcl::PointXYZ>(files[i], *c);
        if (c->size() < 10) continue;

        pcl::PointXYZ mn, mx;
        pcl::getMinMax3D(*c, mn, mx);
        float H = mx.z - mn.z;

        pcl::PCA<pcl::PointXYZ> pca;
        pca.setInputCloud(c);
        Eigen::Vector3f ev = pca.getEigenValues();
        Eigen::Matrix3f evc = pca.getEigenVectors();

        // find which eigenvector corresponds to largest eigenvalue
        int max_i = 0;
        if (ev[1] > ev[max_i]) max_i = 1;
        if (ev[2] > ev[max_i]) max_i = 2;
        Eigen::Vector3f dir = evc.col(max_i).normalized();
        // ensure Z-positive for consistency
        if (dir[2] < 0) dir = -dir;
        float verticality = std::abs(dir[2]);

        // sort eigenvalues
        if (ev[0] < ev[1]) std::swap(ev[0], ev[1]);
        if (ev[1] < ev[2]) std::swap(ev[1], ev[2]);
        if (ev[0] < ev[1]) std::swap(ev[0], ev[1]);
        float l1l2 = ev[0]/ev[1], l2l3 = ev[1]/ev[2];

        // centroid Z offset
        Eigen::Vector4f centroid;
        pcl::compute3DCentroid(*c, centroid);
        float zoff = (centroid[2] - mn.z) / (H + 0.001f);

        printf("%s   %5zu  %.2f  %5.1f  %5.1f  (% .2f,% .2f,% .2f) v=%.2f  %.2f\n",
               names[i], c->size(), H, l1l2, l2l3,
               dir[0], dir[1], dir[2], verticality, zoff);
    }
}
