/**
 * detect_targets.cpp — 标靶球检测管线
 *
 * 1. I > P80 → 3D 聚类 → centroid
 * 2. 每簇取框(全量云) → SAC去最大平面 → k-means双峰比
 * 3. ratio > 5 → 命中, 按ratio排序
 */
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/segmentation/sac_segmentation.h>
#include <pcl/segmentation/extract_clusters.h>
#include <pcl/search/kdtree.h>
#include <pcl/common/centroid.h>
#include <Eigen/Dense>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <sys/stat.h>

struct Detection { float cx,cy,cz,ratio,compact,l2l3; int hi_n,lo_n,total_n; };

int main(int argc, char** argv) {
    if (argc < 3) { std::cerr<<"Usage: "<<argv[0]<<" <scene.pcd> <out_dir>\n"; return 1; }
    std::string scene = argv[1], out = argv[2];
    mkdir(out.c_str(), 0755);

    float cluster_tol = 0.15f;
    int   min_cluster = 100, max_cluster = 50000;
    float box_dx = 0.6f, box_dy = 0.6f, box_dz_up = 0.3f, box_dz_dn = 2.0f;
    float ratio_min = 5.0f, pct_thr = 80.0f;

    // 1. Load
    std::cout << "Loading " << scene << "..." << std::endl;
    pcl::PointCloud<pcl::PointXYZI>::Ptr full(new pcl::PointCloud<pcl::PointXYZI>);
    pcl::io::loadPCDFile<pcl::PointXYZI>(scene, *full);
    std::cout << "  " << full->size() << " pts" << std::endl;

    // 2. I > P80 → cluster cloud
    pcl::PointCloud<pcl::PointXYZI>::Ptr hi_cloud(new pcl::PointCloud<pcl::PointXYZI>);
    {
        std::vector<float> Is; Is.reserve(full->size());
        for (auto& p : full->points) Is.push_back(p.intensity);
        std::sort(Is.begin(), Is.end());
        float th = Is[Is.size() * (int)pct_thr / 100];
        std::cout << "I > P" << (int)pct_thr << " = " << th << std::endl;
        pcl::PointIndices::Ptr keep(new pcl::PointIndices);
        for (size_t i = 0; i < full->size(); i++)
            if (full->points[i].intensity >= th) keep->indices.push_back(i);
        pcl::ExtractIndices<pcl::PointXYZI> ex;
        ex.setInputCloud(full); ex.setIndices(keep); ex.setNegative(false);
        ex.filter(*hi_cloud);
        std::cout << "  kept " << hi_cloud->size() << " pts" << std::endl;
    }
    if (hi_cloud->empty()) return 0;

    // 3. 3D Euclidean cluster
    pcl::PointCloud<pcl::PointXYZ>::Ptr xyz(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::copyPointCloud(*hi_cloud, *xyz);
    pcl::search::KdTree<pcl::PointXYZ>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZ>);
    tree->setInputCloud(xyz);
    std::vector<pcl::PointIndices> clusters;
    pcl::EuclideanClusterExtraction<pcl::PointXYZ> ec;
    ec.setClusterTolerance(cluster_tol);
    ec.setMinClusterSize(min_cluster);
    ec.setMaxClusterSize(max_cluster);
    ec.setSearchMethod(tree); ec.setInputCloud(xyz);
    ec.extract(clusters);
    std::cout << "Clusters: " << clusters.size() << std::endl;

    // 4. Process each cluster
    std::vector<Detection> detections;
    int ci = 0;
    for (auto& idx : clusters) {
        ci++;
        if (idx.indices.size() < (size_t)min_cluster) continue;

        // cluster centroid
        pcl::PointCloud<pcl::PointXYZI>::Ptr cl(new pcl::PointCloud<pcl::PointXYZI>);
        for (auto i : idx.indices) cl->push_back(hi_cloud->points[i]);
        Eigen::Vector4f ct; pcl::compute3DCentroid(*cl, ct);
        float cx = ct[0], cy = ct[1], cz = ct[2];

        // Crop box from FULL original cloud
        pcl::PointCloud<pcl::PointXYZI>::Ptr box_full(new pcl::PointCloud<pcl::PointXYZI>);
        for (auto& p : full->points)
            if (p.x >= cx-box_dx && p.x <= cx+box_dx &&
                p.y >= cy-box_dy && p.y <= cy+box_dy &&
                p.z >= cz-box_dz_dn && p.z <= cz+box_dz_up)
                box_full->push_back(p);
        if (box_full->size() < 200) continue;

        // SAC plane removal (dominant plane = wall/ground)
        pcl::PointCloud<pcl::PointXYZ>::Ptr box_xyz(new pcl::PointCloud<pcl::PointXYZ>);
        pcl::copyPointCloud(*box_full, *box_xyz);
        pcl::SACSegmentation<pcl::PointXYZ> seg;
        seg.setOptimizeCoefficients(true);
        seg.setModelType(pcl::SACMODEL_PLANE);
        seg.setMethodType(pcl::SAC_RANSAC);
        seg.setDistanceThreshold(0.05);
        seg.setMaxIterations(1000);
        seg.setInputCloud(box_xyz);
        pcl::PointIndices::Ptr plane_inl(new pcl::PointIndices);
        pcl::ModelCoefficients::Ptr plane_cf(new pcl::ModelCoefficients);
        seg.segment(*plane_inl, *plane_cf);
        int n_plane = plane_inl->indices.size();
        bool has_plane = (n_plane > (int)(box_full->size() / 10));

        float nz = 0;
        if (has_plane && plane_cf->values.size() >= 4)
            nz = std::abs(plane_cf->values[2]);
        bool is_ground = has_plane && (nz > 0.7f);

        pcl::PointCloud<pcl::PointXYZI>::Ptr box(new pcl::PointCloud<pcl::PointXYZI>);
        if (is_ground) {
            pcl::ExtractIndices<pcl::PointXYZI> ex;
            ex.setInputCloud(box_full); ex.setIndices(plane_inl); ex.setNegative(true);
            ex.filter(*box);
        } else {
            *box = *box_full;
        }
        if (box->size() < 200) continue;
        if (!has_plane) continue;

        // k-means k=2 on intensity
        int n = box->size();
        std::vector<std::pair<float,int>> srt(n);
        for (int i = 0; i < n; i++) srt[i] = {box->points[i].intensity, i};
        std::sort(srt.begin(), srt.end());

        float best_var = 1e9; int sp = n / 10;
        for (int q = n/10; q < n*9/10; q++) {
            float ls = 0, hs = 0; int ln = 0, hn = 0;
            for (int i = 0; i < q; i++)  { ls += srt[i].first; ln++; }
            for (int i = q; i < n; i++)  { hs += srt[i].first; hn++; }
            float v = 0, lm = ls/ln, hm = hs/hn;
            for (int i = 0; i < q; i++)  v += (srt[i].first - lm) * (srt[i].first - lm);
            for (int i = q; i < n; i++)  v += (srt[i].first - hm) * (srt[i].first - hm);
            if (v < best_var) { best_var = v; sp = q; }
        }
        float lo_sum = 0, hi_sum = 0;
        for (int i = 0; i < sp; i++) lo_sum += srt[i].first;
        for (int i = sp; i < n; i++) hi_sum += srt[i].first;
        float ratio = (hi_sum / (n-sp)) / (lo_sum / sp + 0.1f);
        int lo_n = sp, hi_n = n - sp;

        std::cout << "  C" << ci << " box=" << box_full->size() << " plane=" << n_plane
                  << " nz=" << nz << (is_ground?"(G)":"(W)")
                  << " remain=" << n << " ratio=" << ratio << " lo=" << lo_n << " hi=" << hi_n;

        // Z-crop + compactness on hi points
        float z_max = -1e9;
        for (int i = sp; i < n; i++) {
            int j = srt[i].second;
            if (box->points[j].z > z_max) z_max = box->points[j].z;
        }
        float crop_z = 1.2f * 0.2f;  // 1.2 × sphere diameter
        float z_lo = z_max - crop_z;

        pcl::PointCloud<pcl::PointXYZ>::Ptr crop_xyz(new pcl::PointCloud<pcl::PointXYZ>);
        for (int i = sp; i < n; i++) {
            int j = srt[i].second;
            if (box->points[j].z >= z_lo) {
                pcl::PointXYZ pt; pt.x=box->points[j].x; pt.y=box->points[j].y; pt.z=box->points[j].z;
                crop_xyz->push_back(pt);
            }
        }
        int crop_n = crop_xyz->size();

        // 3D spatial cluster on cropped hi
        pcl::search::KdTree<pcl::PointXYZ>::Ptr crop_tree(new pcl::search::KdTree<pcl::PointXYZ>);
        crop_tree->setInputCloud(crop_xyz);
        std::vector<pcl::PointIndices> hi_cl;
        pcl::EuclideanClusterExtraction<pcl::PointXYZ> ec2;
        ec2.setClusterTolerance(0.10);
        ec2.setMinClusterSize(5);
        ec2.setMaxClusterSize(50000);
        ec2.setSearchMethod(crop_tree); ec2.setInputCloud(crop_xyz);
        ec2.extract(hi_cl);

        int max_cl = 0, best_hi_cl = -1;
        for (size_t k = 0; k < hi_cl.size(); k++)
            if ((int)hi_cl[k].indices.size() > max_cl)
                { max_cl = hi_cl[k].indices.size(); best_hi_cl = (int)k; }
        float compact = crop_n > 0 ? (float)max_cl / crop_n : 0;

        // PCA on largest hi spatial cluster → l2/l3 (thinness)
        float l2l3 = 1.0f;
        if (max_cl >= 10 && best_hi_cl >= 0) {
            auto& cl_idx = hi_cl[best_hi_cl];
            // Build Eigen matrix
            Eigen::MatrixXf pts(cl_idx.indices.size(), 3);
            float mx=0,my=0,mz=0;
            for (size_t k=0; k<cl_idx.indices.size(); k++) {
                int idx = cl_idx.indices[k];
                pts(k,0)=crop_xyz->points[idx].x; pts(k,1)=crop_xyz->points[idx].y; pts(k,2)=crop_xyz->points[idx].z;
                mx+=pts(k,0); my+=pts(k,1); mz+=pts(k,2);
            }
            mx/=max_cl; my/=max_cl; mz/=max_cl;
            for (int k=0; k<max_cl; k++) { pts(k,0)-=mx; pts(k,1)-=my; pts(k,2)-=mz; }
            Eigen::Matrix3f cov = pts.transpose() * pts / max_cl;
            Eigen::SelfAdjointEigenSolver<Eigen::Matrix3f> eig(cov);
            Eigen::Vector3f ev = eig.eigenvalues();
            // eigenvalues are already sorted ascending, so ev(2) ≥ ev(1) ≥ ev(0)
            float l1=ev(2), l2=ev(1), l3=ev(0);
            l2l3 = l3 > 1e-6f ? l2 / l3 : 999;
        }

        std::cout << " crop=" << crop_n << " compact=" << compact << " l2l3=" << l2l3;

        if (ratio > ratio_min && compact > 0.9f) {
            float hx = 0, hy = 0, hz = 0;
            for (int i = sp; i < n; i++) {
                int j = srt[i].second;
                hx += box->points[j].x; hy += box->points[j].y; hz += box->points[j].z;
            }
            hx /= hi_n; hy /= hi_n; hz /= hi_n;
            detections.push_back({hx, hy, hz, ratio, compact, l2l3, hi_n, lo_n, n});
            std::cout << " ✓" << std::endl;

            // Save PCD: red=hi-in-crop, orange=hi-below, blue=lo
            std::vector<bool> is_hi(n, false);
            for (int i = sp; i < n; i++) is_hi[srt[i].second] = true;
            pcl::PointCloud<pcl::PointXYZI> save;
            for (int i = 0; i < n; i++) {
                auto p = box->points[i];
                if (is_hi[i] && p.z >= z_lo)   p.intensity = 255;  // red
                else if (is_hi[i])              p.intensity = 200;  // orange
                else                            p.intensity = 50;   // blue
                save.push_back(p);
            }
            char fn[256];
            snprintf(fn, sizeof(fn), "%s/match_%02d_r%.1f.pcd",
                     out.c_str(), (int)detections.size()-1, ratio);
            pcl::io::savePCDFileBinary(fn, save);
        } else {
            std::cout << std::endl;
        }
    }

    // 5. Sort & output
    std::sort(detections.begin(), detections.end(),
              [](auto& a, auto& b) { return a.ratio > b.ratio; });
    std::ofstream csv(out + "/results.csv");
    csv << "rank,ratio,compact,l2l3,cx,cy,cz,hi_n,lo_n,total_n\n";
    std::cout << "\n=== Detections (" << detections.size() << ") ===" << std::endl;
    for (size_t i = 0; i < detections.size(); i++) {
        auto& d = detections[i];
        std::cout << "  #" << i << " ratio=" << d.ratio << " compact=" << d.compact << " l2l3=" << d.l2l3
                  << " (" << d.cx << "," << d.cy << "," << d.cz << ")"
                  << " hi=" << d.hi_n << " lo=" << d.lo_n << std::endl;
        csv << i << "," << d.ratio << "," << d.compact << "," << d.l2l3 << ","
            << d.cx << "," << d.cy << "," << d.cz << ","
            << d.hi_n << "," << d.lo_n << "," << d.total_n << "\n";
    }
    csv.close();
    std::cout << "Done. " << out << "/" << std::endl;
}
