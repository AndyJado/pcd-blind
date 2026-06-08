#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/segmentation/sac_segmentation.h>
#include <pcl/search/kdtree.h>
#include <pcl/segmentation/extract_clusters.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
int main() {
    float px=69.92f, py=3.82f, pz=-2.40f;
    float dx=0.6f,dy=0.6f,dz_up=0.3f,dz_dn=2.0f;
    
    pcl::PointCloud<pcl::PointXYZI>::Ptr full(new pcl::PointCloud<pcl::PointXYZI>);
    pcl::io::loadPCDFile<pcl::PointXYZI>("source/庆元陈家岭隧道1.pcd",*full);
    
    // Box
    pcl::PointCloud<pcl::PointXYZI>::Ptr bf(new pcl::PointCloud<pcl::PointXYZI>);
    for(auto&p:full->points)if(p.x>=px-dx&&p.x<=px+dx&&p.y>=py-dy&&p.y<=py+dy&&p.z>=pz-dz_dn&&p.z<=pz+dz_up)bf->push_back(p);
    std::cout<<"Box: "<<bf->size()<<" pts\n";
    
    // SAC plane
    pcl::PointCloud<pcl::PointXYZ>::Ptr bx(new pcl::PointCloud<pcl::PointXYZ>);pcl::copyPointCloud(*bf,*bx);
    pcl::SACSegmentation<pcl::PointXYZ>seg;seg.setOptimizeCoefficients(true);seg.setModelType(pcl::SACMODEL_PLANE);seg.setMethodType(pcl::SAC_RANSAC);seg.setDistanceThreshold(0.05);seg.setMaxIterations(1000);seg.setInputCloud(bx);
    pcl::PointIndices::Ptr pl(new pcl::PointIndices);pcl::ModelCoefficients::Ptr pcf(new pcl::ModelCoefficients);seg.segment(*pl,*pcf);
    bool hp=(pl->indices.size()>(int)(bf->size()/10));
    float nz=0;if(hp&&pcf->values.size()>=4)nz=std::abs(pcf->values[2]);
    pcl::PointCloud<pcl::PointXYZI>::Ptr box(new pcl::PointCloud<pcl::PointXYZI>);
    if(hp){pcl::ExtractIndices<pcl::PointXYZI>ex;ex.setInputCloud(bf);ex.setIndices(pl);ex.setNegative(true);ex.filter(*box);}else *box=*bf;
    std::cout<<"SAC: plane="<<pl->indices.size()<<" nz="<<nz<<" remain="<<box->size()<<std::endl;
    
    // k-means
    int n=box->size();std::vector<std::pair<float,int>>srt(n);
    for(int i=0;i<n;i++)srt[i]={box->points[i].intensity,i};std::sort(srt.begin(),srt.end());
    float bv=1e9;int sp=n/10;
    for(int q=n/10;q<n*9/10;q++){float ls=0,hs=0;int ln=0,hn=0;
        for(int i=0;i<q;i++){ls+=srt[i].first;ln++;}for(int i=q;i<n;i++){hs+=srt[i].first;hn++;}
        float v=0,lm=ls/ln,hm=hs/hn;
        for(int i=0;i<q;i++)v+=(srt[i].first-lm)*(srt[i].first-lm);
        for(int i=q;i<n;i++)v+=(srt[i].first-hm)*(srt[i].first-hm);if(v<bv){bv=v;sp=q;}}
    float lo_s=0,hi_s=0;for(int i=0;i<sp;i++)lo_s+=srt[i].first;for(int i=sp;i<n;i++)hi_s+=srt[i].first;
    float ratio=(hi_s/(n-sp))/(lo_s/sp+0.1f);
    std::cout<<"k-means: split_I="<<srt[sp].first<<" lo="<<sp<<"("<<lo_s/sp<<") hi="<<n-sp<<"("<<hi_s/(n-sp)<<") ratio="<<ratio<<std::endl;
    
    // Z-crop compact
    float hx=0,hy=0,hz=0;for(int i=sp;i<n;i++){int j=srt[i].second;hx+=box->points[j].x;hy+=box->points[j].y;hz+=box->points[j].z;}hx/=n-sp;hy/=n-sp;hz/=n-sp;
    float z_max=-1e9;for(int i=sp;i<n;i++){int j=srt[i].second;if(box->points[j].z>z_max)z_max=box->points[j].z;}
    float z_lo=z_max-0.24f;
    pcl::PointCloud<pcl::PointXYZ>::Ptr crop(new pcl::PointCloud<pcl::PointXYZ>);
    for(int i=sp;i<n;i++){int j=srt[i].second;if(box->points[j].z>=z_lo){pcl::PointXYZ pt;pt.x=box->points[j].x;pt.y=box->points[j].y;pt.z=box->points[j].z;crop->push_back(pt);}}
    pcl::search::KdTree<pcl::PointXYZ>::Ptr ct(new pcl::search::KdTree<pcl::PointXYZ>);ct->setInputCloud(crop);
    std::vector<pcl::PointIndices>hc;pcl::EuclideanClusterExtraction<pcl::PointXYZ>ec2;ec2.setClusterTolerance(0.10);ec2.setMinClusterSize(5);ec2.setMaxClusterSize(50000);ec2.setSearchMethod(ct);ec2.setInputCloud(crop);ec2.extract(hc);
    int max_cl=0;for(auto&c:hc)if((int)c.indices.size()>max_cl)max_cl=c.indices.size();
    float compact=crop->size()>0?(float)max_cl/crop->size():0;
    std::cout<<"Z-crop: n="<<crop->size()<<" max_cl="<<max_cl<<" compact="<<compact<<std::endl;
    
    // Tripod
    float tripod=0;
    if(sp>=2*(n-sp)){
        int nbins=36;std::vector<int>ah(nbins,0);
        for(int i=0;i<sp;i++){int j=srt[i].second;float dx=box->points[j].x-hx,dy=box->points[j].y-hy;
            float a=atan2(dy,dx)*180/M_PI;if(a<0)a+=360;int b=a*nbins/360;if(b>=0&&b<nbins)ah[b]++;}
        float am=0;for(int v:ah)am+=v;am/=nbins;
        std::vector<int>peaks;
        for(int k=0;k<nbins;k++){int pr=k==0?ah[nbins-1]:ah[k-1];int nx=k==nbins-1?ah[0]:ah[k+1];
            if(ah[k]>=pr&&ah[k]>=nx&&ah[k]>am*1.2f&&ah[k]>=3)peaks.push_back(k);}
        std::vector<int>mg;for(size_t p=0;p<peaks.size();p++){if(mg.empty()||peaks[p]-mg.back()>2)mg.push_back(peaks[p]);else if(ah[peaks[p]]>ah[mg.back()])mg.back()=peaks[p];}
        int np=mg.size();
        if(np>=3){std::vector<std::pair<int,int>>pr2;for(auto pk:mg)pr2.push_back({ah[pk],pk});std::sort(pr2.rbegin(),pr2.rend());
            if(pr2.size()>=3){int p1=pr2[0].second,p2=pr2[1].second,p3=pr2[2].second;
                auto cd=[](int a,int b,int n){int d=abs(a-b);return std::min(d,n-d);};
                float avg=(cd(p1,p2,nbins)+cd(p2,p3,nbins)+cd(p3,p1,nbins))/3.0f;
                tripod=1.0f-std::min(1.0f,std::abs(avg-nbins/3.0f)/(nbins/3.0f));}}}
    std::cout<<"Tripod: "<<tripod<<" (lo="<<sp<<" hi="<<n-sp<<", need lo>=2*hi="<<(sp>=2*(n-sp)?"Y":"N")<<")"<<std::endl;
    
    // Save colored
    std::vector<bool>ih(n,false);for(int i=sp;i<n;i++)ih[srt[i].second]=true;
    pcl::PointCloud<pcl::PointXYZI>sv;
    for(int i=0;i<n;i++){auto p=box->points[i];p.intensity=ih[i]?255:50;sv.push_back(p);}
    pcl::io::savePCDFileBinary("output/qy1_miss_point.pcd",sv);
    std::cout<<"Saved output/qy1_miss_point.pcd"<<std::endl;
}
