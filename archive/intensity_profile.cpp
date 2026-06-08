// intensity_profile.cpp
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/common/common.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>

int main() {
    const char* files[] = {
        "/home/mz/mainframer/pcd-blind/output/targets/t1_noground.pcd",
        "/home/mz/mainframer/pcd-blind/output/targets/t2_noground.pcd",
        "/home/mz/mainframer/pcd-blind/output/targets/t3_noground.pcd",
        "/home/mz/mainframer/pcd-blind/output/targets/t4_noground.pcd",
    };
    const char* names[] = {"T1","T2","T3","T4"};

    std::cout << std::fixed << std::setprecision(1);
    std::cout << "ID   n    H(m)  I_top_mean  I_bot_mean  I_ratio  I_top_P90  I_top_max\n";
    std::cout << "------------------------------------------------------------------------\n";

    for (int i=0;i<4;i++) {
        pcl::PointCloud<pcl::PointXYZI>::Ptr c(new pcl::PointCloud<pcl::PointXYZI>);
        pcl::io::loadPCDFile<pcl::PointXYZI>(files[i], *c);
        if (c->empty()) continue;

        pcl::PointXYZI mn,mx;
        pcl::getMinMax3D(*c,mn,mx);
        float H=mx.z-mn.z, zcut=mn.z+H*0.55f; // split at 55% from bottom

        std::vector<float> itop, ibot;
        for (auto&p:c->points) {
            if (p.z>=zcut) itop.push_back(p.intensity);
            else ibot.push_back(p.intensity);
        }
        if (itop.empty()||ibot.empty()) continue;

        float top_mean=0,bot_mean=0;
        for (auto v:itop) top_mean+=v; top_mean/=itop.size();
        for (auto v:ibot) bot_mean+=v; bot_mean/=ibot.size();

        std::sort(itop.begin(),itop.end());
        float top_p90=itop[itop.size()*90/100];
        float top_max=itop.back();

        printf("%s  %4zu  %.2f  %8.1f  %8.1f  %7.2f  %8.1f  %8.0f\n",
               names[i], c->size(), H, top_mean, bot_mean, top_mean/(bot_mean+0.1f),
               top_p90, top_max);
    }
}
