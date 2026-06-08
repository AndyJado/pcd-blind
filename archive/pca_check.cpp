#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/common/pca.h>
#include <pcl/common/common.h>
#include <iostream>
#include <iomanip>
#include <algorithm>

int main() {
    struct { const char* path; const char* name; } files[] = {
        {"/home/mz/mainframer/pcd-blind/output/targets/t1_33_8.pcd", "BF_T1"},
        {"/home/mz/mainframer/pcd-blind/output/targets/t2_19_-4.pcd", "BF_T2"},
        {"/home/mz/mainframer/pcd-blind/output/targets/t3_33_-4.5.pcd", "BF_T3"},
        {"/home/mz/mainframer/pcd-blind/output/targets/t4_22_8.pcd", "BF_T4"},
        {"/home/mz/mainframer/pcd-blind/output/targets_ql3/t1_10_5_v2.pcd", "QL3_T1"},
        {"/home/mz/mainframer/pcd-blind/output/targets_ql3/t2_53_14.pcd", "QL3_T2"},
    };

    std::cout << std::fixed << std::setprecision(3);
    std::cout << "ID       N_pts   H(m)   W(m)   D(m)   lam1  lam2  lam3  l1/l2  l2/l3\n";
    std::cout << "------------------------------------------------------------------------\n";

    for (auto& f : files) {
        pcl::PointCloud<pcl::PointXYZ>::Ptr c(new pcl::PointCloud<pcl::PointXYZ>);
        pcl::io::loadPCDFile<pcl::PointXYZ>(f.path, *c);
        if (c->size() < 10) continue;

        pcl::PointXYZ mn, mx;
        pcl::getMinMax3D(*c, mn, mx);
        float H = mx.z - mn.z, W = mx.x - mn.x, D = mx.y - mn.y;

        pcl::PCA<pcl::PointXYZ> pca;
        pca.setInputCloud(c);
        Eigen::Vector3f ev = pca.getEigenValues();
        if (ev[0] < ev[1]) std::swap(ev[0], ev[1]);
        if (ev[1] < ev[2]) std::swap(ev[1], ev[2]);
        if (ev[0] < ev[1]) std::swap(ev[0], ev[1]);

        printf("%-8s %5zu  %5.2f %5.2f %5.2f  %5.3f %5.3f %5.3f  %5.1f  %5.1f\n",
               f.name, c->size(), H, W, D, ev[0], ev[1], ev[2],
               ev[0]/ev[1], ev[1]/ev[2]);
    }
}
