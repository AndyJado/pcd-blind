/**
 * check_miss.cpp — 检查计量院漏检点附近为什么没检测到
 */
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/common/centroid.h>
#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    pcl::PointCloud<pcl::PointXYZI>::Ptr full(new pcl::PointCloud<pcl::PointXYZI>);
    pcl::io::loadPCDFile<pcl::PointXYZI>("source/浙江省计量院一楼实验室.pcd", *full);

    float px=6.15f, py=5.05f, pz=-0.60f;
    float R=2.0f;  // 2m radius around picked point

    // Collect region
    pcl::PointCloud<pcl::PointXYZI>::Ptr region(new pcl::PointCloud<pcl::PointXYZI>);
    for (auto& p : full->points)
        if (p.x>=px-R && p.x<=px+R && p.y>=py-R && p.y<=py+R && p.z>=pz-R && p.z<=pz+R)
            region->push_back(p);
    std::cout << "Region " << R << "m around (" << px << "," << py << "): " << region->size() << " pts\n";

    // Global I stats
    std::vector<float> Is;
    for (auto& p : full->points) Is.push_back(p.intensity);
    std::sort(Is.begin(), Is.end());
    float P80 = Is[Is.size()*80/100];
    std::cout << "Global I P80 = " << P80 << "\n";

    // Local I stats in the box
    float box_dx=0.6f, box_dy=0.6f, box_dz_up=0.3f, box_dz_dn=2.0f;
    pcl::PointCloud<pcl::PointXYZI>::Ptr box(new pcl::PointCloud<pcl::PointXYZI>);
    for (auto& p : full->points)
        if (p.x>=px-box_dx && p.x<=px+box_dx &&
            p.y>=py-box_dy && p.y<=py+box_dy &&
            p.z>=pz-box_dz_dn && p.z<=pz+box_dz_up)
            box->push_back(p);
    std::cout << "Box: " << box->size() << " pts\n";

    if (box->empty()) return 0;

    // I stats in box
    std::vector<float> box_I;
    for (auto& p : box->points) box_I.push_back(p.intensity);
    std::sort(box_I.begin(), box_I.end());
    std::cout << "Box I: min=" << box_I.front() << " P50=" << box_I[box_I.size()/2]
              << " P80=" << box_I[box_I.size()*80/100] << " max=" << box_I.back() << "\n";

    // k-means on box
    int n = box->size();
    std::vector<std::pair<float,int>> srt(n);
    for (int i=0;i<n;i++) srt[i]={box->points[i].intensity,i};
    std::sort(srt.begin(),srt.end());
    float bv=1e9; int sp=n/10;
    for (int q=n/10;q<n*9/10;q++) {
        float ls=0,hs=0;int ln=0,hn=0;
        for(int i=0;i<q;i++){ls+=srt[i].first;ln++;}
        for(int i=q;i<n;i++){hs+=srt[i].first;hn++;}
        float v=0,lm=ls/ln,hm=hs/hn;
        for(int i=0;i<q;i++)v+=(srt[i].first-lm)*(srt[i].first-lm);
        for(int i=q;i<n;i++)v+=(srt[i].first-hm)*(srt[i].first-hm);
        if(v<bv){bv=v;sp=q;}
    }
    float lo_s=0,hi_s=0;
    for(int i=0;i<sp;i++)lo_s+=srt[i].first;
    for(int i=sp;i<n;i++)hi_s+=srt[i].first;
    float lo_mean=lo_s/sp, hi_mean=hi_s/(n-sp);
    float ratio=hi_mean/(lo_mean+0.1f);
    float split_I=srt[sp].first;

    std::cout << "k-means: split=" << split_I << " lo=" << sp << "(" << lo_mean
              << ") hi=" << n-sp << "(" << hi_mean << ") ratio=" << ratio << "\n";

    // Hi points max Z
    float z_max=-1e9;
    for(int i=sp;i<n;i++){int j=srt[i].second;if(box->points[j].z>z_max)z_max=box->points[j].z;}
    std::cout << "hi max Z = " << z_max << "  crop: [" << z_max-0.24 << ", " << z_max << "]\n";

    // Hi points in Z-crop
    int crop_n=0;
    for(int i=sp;i<n;i++){int j=srt[i].second;if(box->points[j].z>=z_max-0.24f)crop_n++;}
    std::cout << "hi in crop: " << crop_n << "/" << n-sp << "\n";

    // What's the picked point's intensity and Z?
    pcl::PointXYZI picked;
    float best_d=1e9;
    for(auto&p:box->points){float d=(p.x-px)*(p.x-px)+(p.y-py)*(p.y-py)+(p.z-pz)*(p.z-pz);if(d<best_d){best_d=d;picked=p;}}
    std::cout << "Picked point: I=" << picked.intensity << " z=" << picked.z << " (nearest in box, dist=" << sqrt(best_d) << "m)\n";
}
