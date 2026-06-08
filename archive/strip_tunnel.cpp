// strip_tunnel.cpp — aggressive plane removal, save result with intensity
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/segmentation/sac_segmentation.h>
#include <iostream>

int main() {
    pcl::PointCloud<pcl::PointXYZI>::Ptr si(new pcl::PointCloud<pcl::PointXYZI>);
    pcl::io::loadPCDFile<pcl::PointXYZI>(
        "/home/mz/mainframer/pcd-blind/source/绍兴白峰岭隧道.pcd", *si);

    // voxel 5cm, keep XYZI
    pcl::PointCloud<pcl::PointXYZI>::Ptr ds(new pcl::PointCloud<pcl::PointXYZI>);
    pcl::VoxelGrid<pcl::PointXYZI> vg;
    vg.setInputCloud(si); vg.setLeafSize(0.05,0.05,0.05); vg.filter(*ds);
    std::cout << ds->size() << " pts voxel\n";

    // SAC on XYZ shadow, apply to XYZI
    pcl::PointCloud<pcl::PointXYZ>::Ptr xyz(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::copyPointCloud(*ds, *xyz);
    pcl::PointCloud<pcl::PointXYZ>::Ptr w(new pcl::PointCloud<pcl::PointXYZ>);
    *w = *xyz;
    pcl::PointCloud<pcl::PointXYZI>::Ptr wi(new pcl::PointCloud<pcl::PointXYZI>);
    *wi = *ds;

    int removed = 0;
    while (w->size() > 1000 && removed < 100) {
        pcl::SACSegmentation<pcl::PointXYZ> seg;
        seg.setOptimizeCoefficients(true);
        seg.setModelType(pcl::SACMODEL_PLANE);
        seg.setMethodType(pcl::SAC_RANSAC);
        seg.setDistanceThreshold(0.03);
        seg.setMaxIterations(500);
        seg.setInputCloud(w);
        pcl::PointIndices::Ptr inl(new pcl::PointIndices);
        pcl::ModelCoefficients::Ptr cf(new pcl::ModelCoefficients);
        seg.segment(*inl, *cf);

        double pct = 100.0 * inl->indices.size() / w->size();
        if (removed < 5 || pct > 1.0)
            std::cout << "Plane " << removed << ": " << inl->indices.size()
                      << " pts (" << pct << "%) n=("
                      << cf->values[0] << "," << cf->values[1] << "," << cf->values[2] << ")\n";

        if (pct < 0.2) break;

        { pcl::ExtractIndices<pcl::PointXYZ> ex;
          ex.setInputCloud(w); ex.setIndices(inl); ex.setNegative(true);
          pcl::PointCloud<pcl::PointXYZ>::Ptr t(new pcl::PointCloud<pcl::PointXYZ>);
          ex.filter(*t); w.swap(t); }
        { pcl::ExtractIndices<pcl::PointXYZI> ex;
          ex.setInputCloud(wi); ex.setIndices(inl); ex.setNegative(true);
          pcl::PointCloud<pcl::PointXYZI>::Ptr t(new pcl::PointCloud<pcl::PointXYZI>);
          ex.filter(*t); wi.swap(t); }
        removed++;
    }

    std::cout << "\nRemaining: " << wi->size() << " pts\n";
    pcl::io::savePCDFileBinary(
        "/home/mz/mainframer/pcd-blind/output/tunnel_stripped.pcd", *wi);
    std::cout << "Saved tunnel_stripped.pcd\n";
}
