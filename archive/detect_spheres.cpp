#ifndef DETECT_SPHERES_CPP
#define DETECT_SPHERES_CPP
/**
 * detect_spheres.cpp v7 — template-based: geometric feature matching against labeled target
 * Pipeline: strip planes → cluster → per-cluster geometric features → rank by similarity to template
 */
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/segmentation/sac_segmentation.h>
#include <pcl/segmentation/extract_clusters.h>
#include <pcl/search/kdtree.h>
#include <pcl/common/centroid.h>
#include <pcl/common/pca.h>
#include <pcl/common/common.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <sys/stat.h>
#include <fstream>

struct GeoFeature {
    float height, width, depth;        // bounding box
    float aspect_hw;                   // height / max(width,depth)
    float upper_pct;                   // % points in upper 40%
    float upper_density;               // point density in upper 40% vs total
    float upper_int_mean, upper_int_max;  // intensity in upper region
    float centroid_z;                  // z of centroid
    int   n_points;
};

GeoFeature computeFeatures(pcl::PointCloud<pcl::PointXYZI>::Ptr cloud) {
    GeoFeature f{};
    f.n_points = cloud->size();
    if (cloud->empty()) return f;

    pcl::PointXYZI min_pt, max_pt;
    pcl::getMinMax3D(*cloud, min_pt, max_pt);
    f.height = max_pt.z - min_pt.z;
    f.width  = max_pt.x - min_pt.x;
    f.depth  = max_pt.y - min_pt.y;
    float wh = std::max(f.width, f.depth);
    f.aspect_hw = (wh > 0.001) ? f.height / wh : 0;

    // centroid Z
    Eigen::Vector4f centroid;
    pcl::compute3DCentroid(*cloud, centroid);
    f.centroid_z = centroid[2];

    // upper region = top 40% of bounding box
    float z_cut = min_pt.z + f.height * 0.6f;
    int upper_n = 0;
    double int_sum = 0, int_max = 0;
    for (auto& p : cloud->points) {
        if (p.z >= z_cut) {
            upper_n++;
            int_sum += p.intensity;
            int_max = std::max(int_max, (double)p.intensity);
        }
    }
    f.upper_pct = 100.0f * upper_n / cloud->size();
    float upper_vol = f.width * f.depth * (f.height * 0.4f) + 0.0001f;
    float total_vol = f.width * f.depth * f.height + 0.0001f;
    f.upper_density = (upper_n / upper_vol) / (cloud->size() / total_vol);
    f.upper_int_mean = upper_n > 0 ? int_sum / upper_n : 0;
    f.upper_int_max = int_max;

    return f;
}

float featureDistance(const GeoFeature& a, const GeoFeature& b) {
    // weighted Euclidean distance on normalized features
    float d = 0;
    float w_aspect = 0.25f, w_upper_pct = 0.20f, w_density = 0.20f, w_int = 0.15f, w_height = 0.20f;
    
    auto score = [](float va, float vb, float scale) {
        float d = (va - vb) / (scale + 0.001f);
        return d * d;
    };

    float h_scale = std::max(a.height, b.height);
    d += w_height * score(a.height, b.height, h_scale);
    d += w_aspect * score(a.aspect_hw, b.aspect_hw, 3.0f);
    d += w_upper_pct * score(a.upper_pct, b.upper_pct, 50.0f);
    d += w_density * score(a.upper_density, b.upper_density, 2.0f);
    
    float int_scale = std::max(a.upper_int_mean, b.upper_int_mean);
    d += w_int * score(a.upper_int_mean, b.upper_int_mean, int_scale);

    return d;
}

int main(int argc, char** argv) {
    if (argc < 4) { std::cerr << "Usage: " << argv[0] << " <in.pcd> <template.pcd> <out_dir>\n"; return 1; }
    std::string in = argv[1], tpl_path = argv[2], out = argv[3];
    mkdir(out.c_str(), 0755);

    // ── Load template ──
    pcl::PointCloud<pcl::PointXYZI>::Ptr tpl(new pcl::PointCloud<pcl::PointXYZI>);
    pcl::io::loadPCDFile<pcl::PointXYZI>(tpl_path, *tpl);
    GeoFeature tpl_f = computeFeatures(tpl);
    std::cout << "Template: " << tpl->size() << " pts"
              << "  H=" << tpl_f.height << " W=" << tpl_f.width << " D=" << tpl_f.depth
              << "  aspect=" << tpl_f.aspect_hw
              << "  upper%=" << tpl_f.upper_pct
              << "  upper_density=" << tpl_f.upper_density
              << "  upper_int=" << tpl_f.upper_int_mean
              << "\n";

    // ── Load & strip ──
    pcl::PointCloud<pcl::PointXYZI>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZI>);
    pcl::io::loadPCDFile<pcl::PointXYZI>(in, *cloud);
    std::cout << "Loaded " << cloud->size() << " pts\n";

    pcl::PointCloud<pcl::PointXYZI>::Ptr ds(new pcl::PointCloud<pcl::PointXYZI>);
    pcl::VoxelGrid<pcl::PointXYZI> vg;
    vg.setInputCloud(cloud); vg.setLeafSize(0.05f, 0.05f, 0.05f); vg.filter(*ds);
    std::cout << "Downsampled: " << ds->size() << " pts\n";

    pcl::PointCloud<pcl::PointXYZ>::Ptr xyz_shadow(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::copyPointCloud(*ds, *xyz_shadow);
    pcl::PointCloud<pcl::PointXYZI>::Ptr remain(new pcl::PointCloud<pcl::PointXYZI>);
    *remain = *ds;
    pcl::PointCloud<pcl::PointXYZ>::Ptr remain_xyz(new pcl::PointCloud<pcl::PointXYZ>);
    *remain_xyz = *xyz_shadow;

    // Plane removal
    for (int i = 0; i < 50 && remain_xyz->size() > 500; i++) {
        pcl::SACSegmentation<pcl::PointXYZ> seg;
        seg.setOptimizeCoefficients(true); seg.setModelType(pcl::SACMODEL_PLANE);
        seg.setMethodType(pcl::SAC_RANSAC); seg.setDistanceThreshold(0.03);
        seg.setMaxIterations(500); seg.setInputCloud(remain_xyz);
        pcl::PointIndices::Ptr inliers(new pcl::PointIndices);
        pcl::ModelCoefficients::Ptr coeff(new pcl::ModelCoefficients);
        seg.segment(*inliers, *coeff);
        if (100.0 * inliers->indices.size() / remain_xyz->size() < 0.5) break;
        { pcl::ExtractIndices<pcl::PointXYZ> e; e.setInputCloud(remain_xyz); e.setIndices(inliers); e.setNegative(true);
          pcl::PointCloud<pcl::PointXYZ>::Ptr t(new pcl::PointCloud<pcl::PointXYZ>); e.filter(*t); remain_xyz.swap(t); }
        { pcl::ExtractIndices<pcl::PointXYZI> e; e.setInputCloud(remain); e.setIndices(inliers); e.setNegative(true);
          pcl::PointCloud<pcl::PointXYZI>::Ptr t(new pcl::PointCloud<pcl::PointXYZI>); e.filter(*t); remain.swap(t); }
    }
    std::cout << "After planes: " << remain->size() << " pts\n";

    // Cluster
    pcl::search::KdTree<pcl::PointXYZI>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZI>);
    tree->setInputCloud(remain);
    std::vector<pcl::PointIndices> clusters;
    pcl::EuclideanClusterExtraction<pcl::PointXYZI> ec;
    ec.setClusterTolerance(0.2); ec.setMinClusterSize(15); ec.setMaxClusterSize(50000);
    ec.setSearchMethod(tree); ec.setInputCloud(remain);
    ec.extract(clusters);
    std::cout << "Clusters: " << clusters.size() << "\n";

    // ── Feature matching ──
    struct Match { int id; float dist; GeoFeature feat; pcl::PointCloud<pcl::PointXYZI>::Ptr cloud; };
    std::vector<Match> matches;

    for (size_t ci = 0; ci < clusters.size(); ++ci) {
        auto& idx = clusters[ci].indices;
        if (idx.size() < 20 || idx.size() > 50000) continue;
        pcl::PointCloud<pcl::PointXYZI>::Ptr cl(new pcl::PointCloud<pcl::PointXYZI>);
        for (auto i : idx) cl->push_back(remain->points[i]);
        GeoFeature f = computeFeatures(cl);
        // Pre-filter: must be roughly vertical (aspect > 1.5)
        if (f.aspect_hw < 1.2) continue;
        float dist = featureDistance(f, tpl_f);
        matches.push_back({(int)ci, dist, f, cl});
    }

    std::sort(matches.begin(), matches.end(), [](auto& a, auto& b){ return a.dist < b.dist; });

    int n_out = std::min(30, (int)matches.size());
    std::cout << "\nTop " << n_out << " matches:\n";
    std::cout << "  rank  dist    n_pts   H     W     D     aspect   up%    up_dens  up_int  centroid_z\n";
    for (int i = 0; i < n_out; ++i) {
        auto& m = matches[i];
        std::cout << "  " << i << "     " << m.dist << "  " << m.feat.n_points
                  << "  " << m.feat.height << "  " << m.feat.width << "  " << m.feat.depth
                  << "  " << m.feat.aspect_hw << "  " << m.feat.upper_pct
                  << "  " << m.feat.upper_density << "  " << m.feat.upper_int_mean
                  << "  " << m.feat.centroid_z << "\n";
    }

    // Save match cloud and top N clusters
    pcl::io::savePCDFileBinary(out + "/stripped.pcd", *remain);
    std::ofstream csv(out + "/matches.csv");
    csv << "rank,dist,n_pts,height,width,depth,aspect,upper_pct,upper_density,upper_int_mean,centroid_z\n";
    for (int i = 0; i < n_out; ++i) {
        auto& m = matches[i];
        char fname[128]; snprintf(fname, sizeof(fname), "%s/match_%02d_dist%.3f_n%d.pcd", out.c_str(), i, m.dist, m.feat.n_points);
        pcl::io::savePCDFileBinary(fname, *m.cloud);
        Eigen::Vector4f c; pcl::compute3DCentroid(*m.cloud, c);
        csv << i << "," << m.dist << "," << m.feat.n_points << ","
            << m.feat.height << "," << m.feat.width << "," << m.feat.depth << ","
            << m.feat.aspect_hw << "," << m.feat.upper_pct << ","
            << m.feat.upper_density << "," << m.feat.upper_int_mean << ","
            << m.feat.centroid_z << "\n";
    }
    csv.close();
    std::cout << "\nSaved top " << n_out << " matches to " << out << "/\n";
}
#endif
