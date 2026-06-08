/**
 * template_compact.cpp — 模板按 hi 最高点向下 1.2D 取窗 → 空间聚类紧凑度
 */
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/segmentation/extract_clusters.h>
#include <pcl/search/kdtree.h>
#include <pcl/common/common.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <dirent.h>
#include <cstring>

int main() {
    const char* dirs[] = {"output/targets", "output/targets_ql3"};
    float D = 0.2f;   // sphere diameter
    float crop_z = 1.2f * D;  // 0.24m

    printf("%-30s %7s %6s %7s %7s %6s %6s\n", "file", "n", "hi_n", "crop_n", "max_cl", "compact", "ratio");
    printf("%s\n", std::string(70, '-').c_str());

    for (auto dir : dirs) {
        DIR* dp = opendir(dir);
        if (!dp) continue;
        struct dirent* de;
        while ((de = readdir(dp))) {
            if (!strstr(de->d_name, ".pcd")) continue;

            std::string path = std::string(dir) + "/" + de->d_name;
            pcl::PointCloud<pcl::PointXYZI>::Ptr c(new pcl::PointCloud<pcl::PointXYZI>);
            if (pcl::io::loadPCDFile<pcl::PointXYZI>(path, *c) < 0) continue;
            if (c->empty()) continue;

            int n = c->size();

            // k-means intensity split
            std::vector<std::pair<float,int>> srt(n);
            for (int i=0; i<n; i++) srt[i] = {c->points[i].intensity, i};
            std::sort(srt.begin(), srt.end());
            float bv=1e9; int sp=n/10;
            for (int q=n/10; q<n*9/10; q++) {
                float ls=0,hs=0; int ln=0,hn=0;
                for(int i=0;i<q;i++){ls+=srt[i].first;ln++;}
                for(int i=q;i<n;i++){hs+=srt[i].first;hn++;}
                float lm=ls/ln,hm=hs/hn,v=0;
                for(int i=0;i<q;i++)v+=(srt[i].first-lm)*(srt[i].first-lm);
                for(int i=q;i<n;i++)v+=(srt[i].first-hm)*(srt[i].first-hm);
                if(v<bv){bv=v;sp=q;}
            }
            float lo_s=0,hi_s=0;
            for(int i=0;i<sp;i++)lo_s+=srt[i].first;
            for(int i=sp;i<n;i++)hi_s+=srt[i].first;
            float ratio = (hi_s/(n-sp)) / (lo_s/sp + 0.1f);
            int hi_n = n-sp;
            float split_I = srt[sp].first;

            // Find hi points & their max Z
            float z_max = -1e9;
            for (int i=sp; i<n; i++) {
                int j=srt[i].second;
                if (c->points[j].z > z_max) z_max = c->points[j].z;
            }

            // Crop: hi points within [z_max - crop_z, z_max]
            float z_lo = z_max - crop_z;
            pcl::PointCloud<pcl::PointXYZ>::Ptr crop_xyz(new pcl::PointCloud<pcl::PointXYZ>);
            for (int i=sp; i<n; i++) {
                int j=srt[i].second;
                if (c->points[j].z >= z_lo) {
                    pcl::PointXYZ pt; pt.x=c->points[j].x; pt.y=c->points[j].y; pt.z=c->points[j].z;
                    crop_xyz->push_back(pt);
                }
            }
            int crop_n = crop_xyz->size();

            // 3D cluster on cropped hi points
            pcl::search::KdTree<pcl::PointXYZ>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZ>);
            tree->setInputCloud(crop_xyz);
            std::vector<pcl::PointIndices> cl;
            pcl::EuclideanClusterExtraction<pcl::PointXYZ> ec;
            ec.setClusterTolerance(0.08);
            ec.setMinClusterSize(5);
            ec.setMaxClusterSize(50000);
            ec.setSearchMethod(tree);
            ec.setInputCloud(crop_xyz);
            ec.extract(cl);

            int max_cl = 0;
            for (auto& c : cl)
                if ((int)c.indices.size() > max_cl) max_cl = c.indices.size();
            float compact = crop_n > 0 ? (float)max_cl / crop_n : 0;

            printf("%-30s %7d %6d %7d %7d %7.3f %6.1f\n",
                   de->d_name, n, hi_n, crop_n, max_cl, compact, ratio);
        }
        closedir(dp);
    }
}
