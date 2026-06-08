// analyze_2d.cpp — project templates to XY, compute 2D features
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/common/common.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <iomanip>

float convex_area(std::vector<float>& xs, std::vector<float>& ys) {
    // Simple: bounding box area
    float xmin=*std::min_element(xs.begin(),xs.end());
    float xmax=*std::max_element(xs.begin(),xs.end());
    float ymin=*std::min_element(ys.begin(),ys.end());
    float ymax=*std::max_element(ys.begin(),ys.end());
    return (xmax-xmin)*(ymax-ymin);
}

int main() {
    struct { const char* path; const char* name; } files[] = {
        {"/home/mz/mainframer/pcd-blind/output/targets/t1_noground.pcd","BF_T1"},
        {"/home/mz/mainframer/pcd-blind/output/targets/t2_noground.pcd","BF_T2"},
        {"/home/mz/mainframer/pcd-blind/output/targets/t3_noground.pcd","BF_T3"},
        {"/home/mz/mainframer/pcd-blind/output/targets/t4_noground.pcd","BF_T4"},
        {"/home/mz/mainframer/pcd-blind/output/targets_ql3/t1_noground.pcd","QL3_T1"},
        {"/home/mz/mainframer/pcd-blind/output/targets_ql3/t2_noground.pcd","QL3_T2"},
    };
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "ID      N    bbox_area  convex_area  density  I_mean  I_std\n";
    for (auto& f : files) {
        pcl::PointCloud<pcl::PointXYZI>::Ptr c(new pcl::PointCloud<pcl::PointXYZI>);
        pcl::io::loadPCDFile<pcl::PointXYZI>(f.path, *c);
        std::vector<float> xs, ys, is;
        for (auto& p : c->points) { xs.push_back(p.x); ys.push_back(p.y); is.push_back(p.intensity); }
        float bb_area = ( *std::max_element(xs.begin(),xs.end()) - *std::min_element(xs.begin(),xs.end()) )
                      * ( *std::max_element(ys.begin(),ys.end()) - *std::min_element(ys.begin(),ys.end()) );
        float density = c->size() / (bb_area + 0.001f);
        float imean=0; for(auto v:is) imean+=v; imean/=is.size();
        float istd=0; for(auto v:is) istd+=(v-imean)*(v-imean); istd=sqrt(istd/is.size());
        float cvx = convex_area(xs, ys);
        printf("%s %5zu  %8.2f  %8.2f  %8.1f  %6.1f  %6.1f\n",
               f.name, c->size(), bb_area, cvx, density, imean, istd);
    }
}
