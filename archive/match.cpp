/**
 * match.cpp — FPFH + SAC-IA. Template & scene at SAME voxel scale (5cm).
 */
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/features/normal_3d.h>
#include <pcl/features/fpfh.h>
#include <pcl/registration/ia_ransac.h>
#include <pcl/search/kdtree.h>
#include <pcl/common/centroid.h>
#include <pcl/common/common.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <sys/stat.h>
#include <fstream>

class FeatureCloud {
public:
    using PCL = pcl::PointCloud<pcl::PointXYZ>;
    using Normals = pcl::PointCloud<pcl::Normal>;
    using FPFH = pcl::PointCloud<pcl::FPFHSignature33>;
    using Search = pcl::search::KdTree<pcl::PointXYZ>;
    FeatureCloud() : s_(new Search), nr_(0.08f), fr_(0.15f) {}
    void setCloud(PCL::Ptr c) { c_ = c; comp(); }
    PCL::Ptr cloud() const { return c_; }
    FPFH::Ptr fpfh() const { return f_; }
private:
    void comp() {
        n_.reset(new Normals);
        pcl::NormalEstimation<pcl::PointXYZ, pcl::Normal> ne;
        ne.setInputCloud(c_); ne.setSearchMethod(s_);
        ne.setRadiusSearch(nr_); ne.compute(*n_);
        f_.reset(new FPFH);
        pcl::FPFHEstimation<pcl::PointXYZ, pcl::Normal, pcl::FPFHSignature33> fe;
        fe.setInputCloud(c_); fe.setInputNormals(n_);
        fe.setSearchMethod(s_); fe.setRadiusSearch(fr_); fe.compute(*f_);
    }
    PCL::Ptr c_; Normals::Ptr n_; FPFH::Ptr f_; Search::Ptr s_;
    float nr_, fr_;
};

int main(int argc, char** argv) {
    if (argc < 3) { std::cerr << "Usage: " << argv[0] << " <scene> <tpl> <out>\n"; return 1; }
    std::string sp = argv[1], tp = argv[2], out = argv[3];
    mkdir(out.c_str(), 0755);

    pcl::PointCloud<pcl::PointXYZI>::Ptr si(new pcl::PointCloud<pcl::PointXYZI>);
    pcl::io::loadPCDFile<pcl::PointXYZI>(sp, *si);
    pcl::PointCloud<pcl::PointXYZI>::Ptr ti(new pcl::PointCloud<pcl::PointXYZI>);
    pcl::io::loadPCDFile<pcl::PointXYZI>(tp, *ti);

    pcl::PointCloud<pcl::PointXYZ>::Ptr sx(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::copyPointCloud(*si, *sx);
    pcl::PointCloud<pcl::PointXYZ>::Ptr tx(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::copyPointCloud(*ti, *tx);

    // downsample BOTH to 5cm
    pcl::VoxelGrid<pcl::PointXYZ> vg;
    pcl::PointCloud<pcl::PointXYZ>::Ptr tds(new pcl::PointCloud<pcl::PointXYZ>);
    vg.setInputCloud(tx); vg.setLeafSize(0.05f,0.05f,0.05f); vg.filter(*tds);
    pcl::PointCloud<pcl::PointXYZ>::Ptr sds(new pcl::PointCloud<pcl::PointXYZ>);
    vg.setInputCloud(sx); vg.setLeafSize(0.05f,0.05f,0.05f); vg.filter(*sds);
    std::cout << "Template: " << tx->size() << "->" << tds->size() << "  Scene: " << sx->size() << "->" << sds->size() << "\n";

    FeatureCloud tfc; tfc.setCloud(tds);
    FeatureCloud sfc; sfc.setCloud(sds);

    std::ofstream csv(out + "/matches.csv");
    csv << "id,fitness,cx,cy,cz,n_region\n";
    const float FIT_THRESH = 0.015;
    const float BOX = 1.0;  // match extraction box, same scale as template
    const int MAX_DET = 20;

    pcl::PointCloud<pcl::PointXYZ>::Ptr sw(new pcl::PointCloud<pcl::PointXYZ>);
    *sw = *sds;

    for (int d = 0; d < MAX_DET && sw->size() > 100; d++) {
        pcl::SampleConsensusInitialAlignment<pcl::PointXYZ, pcl::PointXYZ, pcl::FPFHSignature33> sac;
        sac.setMinSampleDistance(0.05f);
        sac.setMaxCorrespondenceDistance(0.5f);
        sac.setMaximumIterations(500);
        sac.setInputSource(tfc.cloud()); sac.setSourceFeatures(tfc.fpfh());
        sac.setInputTarget(sfc.cloud()); sac.setTargetFeatures(sfc.fpfh());
        pcl::PointCloud<pcl::PointXYZ> al; sac.align(al);
        if (!sac.hasConverged()) break;
        float fit = sac.getFitnessScore(0.5f);
        std::cout << "#" << d << " fitness=" << fit << "\n";
        if (fit > FIT_THRESH) break;

        Eigen::Vector4f tc; pcl::compute3DCentroid(*tds, tc);
        Eigen::Vector4f mc = sac.getFinalTransformation() * tc;
        float cx=mc[0], cy=mc[1], cz=mc[2];

        pcl::PointCloud<pcl::PointXYZI>::Ptr reg(new pcl::PointCloud<pcl::PointXYZI>);
        for (auto& p : si->points)
            if (p.x>=cx-BOX&&p.x<=cx+BOX&&p.y>=cy-BOX&&p.y<=cy+BOX&&p.z>=cz-BOX&&p.z<=cz+BOX)
                reg->push_back(p);
        char fn[128]; snprintf(fn, sizeof(fn), "%s/match_%02d_f%.4f.pcd", out.c_str(), d, fit);
        pcl::io::savePCDFileBinary(fn, *reg);
        csv << d << "," << fit << "," << cx << "," << cy << "," << cz << "," << reg->size() << "\n";
        std::cout << "  -> (" << cx << "," << cy << "," << cz << ") " << reg->size() << "pts\n";

        pcl::PointIndices::Ptr rm(new pcl::PointIndices);
        for (size_t i=0; i<sw->size(); i++) {
            auto& p = sw->points[i];
            if (p.x>=cx-BOX&&p.x<=cx+BOX&&p.y>=cy-BOX&&p.y<=cy+BOX&&p.z>=cz-BOX&&p.z<=cz+BOX)
                rm->indices.push_back(i);
        }
        pcl::ExtractIndices<pcl::PointXYZ> ex;
        ex.setInputCloud(sw); ex.setIndices(rm); ex.setNegative(true);
        pcl::PointCloud<pcl::PointXYZ>::Ptr tmp(new pcl::PointCloud<pcl::PointXYZ>);
        ex.filter(*tmp); sw.swap(tmp);
        std::cout << "  scene left: " << sw->size() << "  recompute FPFH...\n";
        sfc.setCloud(sw);
    }
    csv.close();
    std::cout << "Done. " << out << "/\n";
}
