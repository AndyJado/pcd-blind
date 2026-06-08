// save_fpfh.cpp — extract & save FPFH features
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/features/normal_3d.h>
#include <pcl/features/fpfh.h>
#include <pcl/search/kdtree.h>
#include <fstream>
#include <iomanip>
#include <sys/stat.h>

int main(int argc, char** argv) {
    if (argc < 3) { std::cerr << "Usage: " << argv[0] << " <pcd> <out_dir>\n"; return 1; }
    std::string in = argv[1], out = argv[2];
    mkdir(out.c_str(), 0755);

    pcl::PointCloud<pcl::PointXYZI>::Ptr raw(new pcl::PointCloud<pcl::PointXYZI>);
    pcl::io::loadPCDFile<pcl::PointXYZI>(in, *raw);
    pcl::PointCloud<pcl::PointXYZ>::Ptr xyz(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::copyPointCloud(*raw, *xyz);

    pcl::PointCloud<pcl::PointXYZ>::Ptr ds(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::VoxelGrid<pcl::PointXYZ> vg;
    vg.setInputCloud(xyz); vg.setLeafSize(0.05,0.05,0.05); vg.filter(*ds);

    pcl::PointCloud<pcl::Normal>::Ptr n(new pcl::PointCloud<pcl::Normal>);
    pcl::search::KdTree<pcl::PointXYZ>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZ>);
    pcl::NormalEstimation<pcl::PointXYZ, pcl::Normal> ne;
    ne.setInputCloud(ds); ne.setSearchMethod(tree); ne.setRadiusSearch(0.08); ne.compute(*n);

    pcl::PointCloud<pcl::FPFHSignature33>::Ptr f(new pcl::PointCloud<pcl::FPFHSignature33>);
    pcl::FPFHEstimation<pcl::PointXYZ, pcl::Normal, pcl::FPFHSignature33> fpfh;
    fpfh.setInputCloud(ds); fpfh.setInputNormals(n); fpfh.setSearchMethod(tree);
    fpfh.setRadiusSearch(0.15); fpfh.compute(*f);

    std::cout << "Points: " << ds->size() << "  FPFH: " << f->size() << " x 33\n";
    pcl::io::savePCDFileBinary(out + "/points_5cm.pcd", *ds);

    { std::ofstream of(out + "/fpfh.bin", std::ios::binary);
      for (size_t i=0; i<f->size(); i++) of.write((char*)f->points[i].histogram, 33*sizeof(float)); }

    { std::ofstream tf(out + "/fpfh_preview.txt");
      tf << "Total: " << f->size() << " pts x 33 dims\n";
      for (size_t i=0; i<std::min((size_t)10,f->size()); i++) {
          tf << std::fixed << std::setprecision(4) << i << " "
             << ds->points[i].x << " " << ds->points[i].y << " " << ds->points[i].z << " | ";
          for (int j=0; j<11; j++) tf << f->points[i].histogram[j] << " ";
          tf << "...\n";
      }}

    std::cout << "Saved: " << out << "/points_5cm.pcd | fpfh.bin | fpfh_preview.txt\n";
}
