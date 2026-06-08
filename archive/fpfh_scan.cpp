// fpfh_scan.cpp — sweep FPFH parameters, report validity stats
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/features/normal_3d.h>
#include <pcl/features/fpfh.h>
#include <pcl/search/kdtree.h>
#include <iostream>
#include <vector>
#include <iomanip>

int main(int argc, char** argv) {
    std::string in = argv[1];

    pcl::PointCloud<pcl::PointXYZI>::Ptr raw(new pcl::PointCloud<pcl::PointXYZI>);
    pcl::io::loadPCDFile<pcl::PointXYZI>(in, *raw);
    pcl::PointCloud<pcl::PointXYZ>::Ptr xyz(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::copyPointCloud(*raw, *xyz);

    std::vector<float> voxels = {0.03f, 0.05f, 0.08f, 0.10f};
    std::vector<float> nradii = {0.08f, 0.12f, 0.15f, 0.20f, 0.25f, 0.30f};
    std::vector<float> fradii = {0.15f, 0.20f, 0.25f, 0.30f, 0.40f, 0.50f};

    std::cout << std::fixed << std::setprecision(3);
    std::cout << "voxel  n_pts  n_rad  f_rad  valid%  mean_nonzero_bins  max_bin0\n";
    std::cout << "---------------------------------------------------------------\n";

    for (float v : voxels) {
        // downsample
        pcl::PointCloud<pcl::PointXYZ>::Ptr ds(new pcl::PointCloud<pcl::PointXYZ>);
        pcl::VoxelGrid<pcl::PointXYZ> vg;
        vg.setInputCloud(xyz); vg.setLeafSize(v,v,v); vg.filter(*ds);
        if (ds->size() < 30) continue;

        for (float nr : nradii) {
            // normals
            pcl::PointCloud<pcl::Normal>::Ptr n(new pcl::PointCloud<pcl::Normal>);
            pcl::search::KdTree<pcl::PointXYZ>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZ>);
            pcl::NormalEstimation<pcl::PointXYZ, pcl::Normal> ne;
            ne.setInputCloud(ds); ne.setSearchMethod(tree); ne.setRadiusSearch(nr); ne.compute(*n);

            for (float fr : fradii) {
                if (fr < nr * 1.2f) continue;
                // FPFH
                pcl::PointCloud<pcl::FPFHSignature33>::Ptr f(new pcl::PointCloud<pcl::FPFHSignature33>);
                pcl::FPFHEstimation<pcl::PointXYZ, pcl::Normal, pcl::FPFHSignature33> fpfh;
                fpfh.setInputCloud(ds); fpfh.setInputNormals(n); fpfh.setSearchMethod(tree);
                fpfh.setRadiusSearch(fr); fpfh.compute(*f);

                int valid = 0, total = f->size();
                float sum_nz = 0, max_b0 = 0;
                for (size_t i = 0; i < f->size(); i++) {
                    int nz = 0;
                    for (int j = 1; j < 33; j++) if (f->points[i].histogram[j] > 0.01f) nz++;
                    if (nz > 0) valid++;
                    else max_b0 = std::max(max_b0, f->points[i].histogram[0]);
                    sum_nz += nz;
                }
                float valid_pct = 100.0f * valid / total;
                float mean_nz = sum_nz / total;

                printf("%.2f   %5d  %.2f   %.2f   %5.1f%%  %6.1f          %6.1f\n",
                       v, (int)ds->size(), nr, fr, valid_pct, mean_nz, max_b0);
            }
        }
        std::cout << "\n";
    }
}
