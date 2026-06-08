// extract_target.cpp — standalone: extract a box region from a PCD
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <iostream>
int main(int argc, char** argv) {
    if (argc < 8) { std::cerr << "Usage: extract <in.pcd> <out.pcd> <xmin> <xmax> <ymin> <ymax> <zmin> <zmax>\n"; return 1; }
    pcl::PointCloud<pcl::PointXYZI>::Ptr in(new pcl::PointCloud<pcl::PointXYZI>);
    pcl::io::loadPCDFile<pcl::PointXYZI>(argv[1], *in);
    float xmin=atof(argv[3]),xmax=atof(argv[4]),ymin=atof(argv[5]),ymax=atof(argv[6]),zmin=atof(argv[7]),zmax=atof(argv[8]);
    pcl::PointCloud<pcl::PointXYZI>::Ptr out(new pcl::PointCloud<pcl::PointXYZI>);
    for (auto& p : in->points)
        if (p.x>=xmin&&p.x<=xmax&&p.y>=ymin&&p.y<=ymax&&p.z>=zmin&&p.z<=zmax)
            out->push_back(p);
    pcl::io::savePCDFileBinary(argv[2], *out);
    std::cout << "Extracted " << out->size() << " pts\n";
}
