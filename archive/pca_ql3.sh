#!/bin/bash
set -e
D="$(cd "$(dirname "$0")" && pwd)"
P="$(cd "$D/.." && pwd)"
cat > /tmp/pca_q.cpp << 'CPP'
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/common/pca.h>
#include <pcl/common/common.h>
#include <iostream>
#include <iomanip>
#include <algorithm>
int main() {
    const char* fs[]={
        "/home/mz/mainframer/pcd-blind/output/targets_ql3/t1_noground.pcd",
        "/home/mz/mainframer/pcd-blind/output/targets_ql3/t2_noground.pcd"};
    const char* ns[]={"QL3_T1","QL3_T2"};
    std::cout<<std::fixed<<std::setprecision(3);
    std::cout<<"ID      N     H     W     D    lam1  lam2  lam3  l1/l2 l2/l3  v   zoff\n";
    for(int i=0;i<2;i++){
        pcl::PointCloud<pcl::PointXYZ>::Ptr c(new pcl::PointCloud<pcl::PointXYZ>);
        pcl::io::loadPCDFile<pcl::PointXYZ>(fs[i],*c);
        pcl::PointXYZ mn,mx; pcl::getMinMax3D(*c,mn,mx);
        float H=mx.z-mn.z,W=mx.x-mn.x,D=mx.y-mn.y;
        pcl::PCA<pcl::PointXYZ> pca; pca.setInputCloud(c);
        Eigen::Vector3f ev=pca.getEigenValues();
        Eigen::Matrix3f evc=pca.getEigenVectors();
        int mi=0;if(ev[1]>ev[mi])mi=1;if(ev[2]>ev[mi])mi=2;
        Eigen::Vector3f dir=evc.col(mi);if(dir[2]<0)dir=-dir;dir.normalize();
        if(ev[0]<ev[1])std::swap(ev[0],ev[1]);if(ev[1]<ev[2])std::swap(ev[1],ev[2]);if(ev[0]<ev[1])std::swap(ev[0],ev[1]);
        Eigen::Vector4f ct; pcl::compute3DCentroid(*c,ct);
        float zoff=(ct[2]-mn.z)/(H+0.001f);
        printf("%s %5zu %5.2f %5.2f %5.2f %5.3f %5.3f %5.3f %5.1f %5.1f %.2f %.2f\n",
               ns[i],c->size(),H,W,D,ev[0],ev[1],ev[2],ev[0]/ev[1],ev[1]/ev[2],std::abs(dir[2]),zoff);
    }
}
CPP
g++ -O2 /tmp/pca_q.cpp -o /tmp/pca_q $(pkg-config --cflags --libs pcl_common-1.14 pcl_io-1.14) 2>&1 && /tmp/pca_q
