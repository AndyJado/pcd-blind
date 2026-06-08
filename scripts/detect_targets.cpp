/**
 * detect_targets.cpp — 标靶球检测管线 vFinal
 *
 * 1. I>P80 → DBSCAN 聚类
 * 2. 每簇 centroid 取框 → SAC 去平面 → k-means 双峰比
 * 3. Z-crop 紧凑度 + PCA l2/l3 + 三脚架角分布
 * 4. 按 tripod_score × ratio 排序
 */
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/filters/voxel_grid.h>
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
#include <cstring>
#include <sys/stat.h>

struct Detection { float cx,cy,cz,ratio,compact,l2l3,tripod; int hi_n,lo_n,total_n; };
struct Config { float pct=80,ratio=5,eps=0.15f,voxel=0; int min_pts=5,min_clus=100,max_clus=50000; };

Config load_config(const std::string& scene) {
    Config c;
    std::ifstream f("config_detect.txt"); if(!f.is_open()) return c;
    std::string line, section, fname=scene;
    size_t pos=fname.rfind('/'); if(pos!=std::string::npos) fname=fname.substr(pos+1);
    while(std::getline(f,line)){
        while(!line.empty()&&(line.back()=='\r'||line.back()==' ')) line.pop_back();
        if(line.empty()||line[0]=='#') continue;
        if(line[0]=='['){section=line.substr(1,line.find(']')-1);continue;}
        bool match=(section=="default")||(fname.find(section)!=std::string::npos);
        if(!match) continue;
        size_t eq=line.find('='); if(eq==std::string::npos) continue;
        std::string k=line.substr(0,eq); float v=atof(line.substr(eq+1).c_str());
        while(!k.empty()&&(k.back()==' '||k.back()=='\t')) k.pop_back();
        if(section!="default"&&k=="pct") c.pct=v;
        if(section!="default"&&k=="ratio") c.ratio=v;
        if(section!="default"&&k=="eps") c.eps=v;
        if(section!="default"&&k=="min_pts") c.min_pts=(int)v;
        if(section!="default"&&k=="min_clus") c.min_clus=(int)v;
        if(section!="default"&&k=="max_clus") c.max_clus=(int)v;
        if(section!="default"&&k=="voxel") c.voxel=v;
        if(section=="default"&&k=="pct") c.pct=v;
        if(section=="default"&&k=="ratio") c.ratio=v;
        if(section=="default"&&k=="eps") c.eps=v;
        if(section=="default"&&k=="min_pts") c.min_pts=(int)v;
        if(section=="default"&&k=="min_clus") c.min_clus=(int)v;
        if(section=="default"&&k=="max_clus") c.max_clus=(int)v;
        if(section=="default"&&k=="voxel") c.voxel=v;
    }
    return c;
}

int main(int argc, char** argv) {
    if(argc<3){std::cerr<<"Usage: "<<argv[0]<<" <scene> <out> [--pct X] [--ratio X]\n";return 1;}
    std::string scene=argv[1],out=argv[2]; mkdir(out.c_str(),0755);

    Config cfg=load_config(scene);
    float pct_thr=cfg.pct, ratio_min=cfg.ratio, eps=cfg.eps, voxel_sz=cfg.voxel;
    int min_pts=cfg.min_pts, min_cluster=cfg.min_clus, max_cluster=cfg.max_clus;
    float box_dx=0.6f,box_dy=0.6f,box_dz_up=0.3f,box_dz_dn=2.0f;
    float compact_min=0.9f;

    for(int i=2;i<argc;i++){std::string a=argv[i];
        if(a=="--pct"&&i+1<argc) pct_thr=atof(argv[++i]);
        if(a=="--ratio"&&i+1<argc) ratio_min=atof(argv[++i]);
    }

    std::cout<<"Config: pct="<<pct_thr<<" ratio="<<ratio_min<<" eps="<<eps<<" min_pts="<<min_pts<<" voxel="<<voxel_sz<<std::endl;

    // Load
    std::cout<<"Loading "<<scene<<"..."<<std::endl;
    pcl::PointCloud<pcl::PointXYZI>::Ptr full(new pcl::PointCloud<pcl::PointXYZI>);
    pcl::io::loadPCDFile<pcl::PointXYZI>(scene,*full);
    std::cout<<"  "<<full->size()<<" pts"<<std::endl;

    if(voxel_sz>0.001f){
        pcl::VoxelGrid<pcl::PointXYZI> vg; vg.setInputCloud(full); vg.setLeafSize(voxel_sz,voxel_sz,voxel_sz);
        auto tmp=std::make_shared<pcl::PointCloud<pcl::PointXYZI>>(); vg.filter(*tmp); full.swap(tmp);
        std::cout<<"Voxel "<<voxel_sz<<"m: "<<full->size()<<" pts"<<std::endl;
    }

    // I>Pct
    pcl::PointCloud<pcl::PointXYZI>::Ptr hi(new pcl::PointCloud<pcl::PointXYZI>);
    {std::vector<float>Is;Is.reserve(full->size());for(auto&p:full->points)Is.push_back(p.intensity);
     std::sort(Is.begin(),Is.end()); float th=Is[Is.size()*(int)pct_thr/100];
     std::cout<<"I>P"<<(int)pct_thr<<"="<<th<<std::endl;
     pcl::PointIndices::Ptr k(new pcl::PointIndices);
     for(size_t i=0;i<full->size();i++)if(full->points[i].intensity>=th)k->indices.push_back(i);
     pcl::ExtractIndices<pcl::PointXYZI>ex;ex.setInputCloud(full);ex.setIndices(k);ex.setNegative(false);ex.filter(*hi);
     std::cout<<"  kept "<<hi->size()<<" pts"<<std::endl;}
    if(hi->empty())return 0;

    // DBSCAN
    pcl::PointCloud<pcl::PointXYZ>::Ptr xyz(new pcl::PointCloud<pcl::PointXYZ>); pcl::copyPointCloud(*hi,*xyz);
    pcl::search::KdTree<pcl::PointXYZ>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZ>); tree->setInputCloud(xyz);
    std::vector<int> labels(xyz->size(),-1); std::vector<int> ni; std::vector<float> nd;
    int cid=0;
    for(size_t i=0;i<xyz->size();i++){
        if(labels[i]!=-1)continue;
        tree->radiusSearch(xyz->points[i],eps,ni,nd);
        if((int)ni.size()<min_pts){labels[i]=-2;continue;}
        labels[i]=cid; std::vector<int> seeds(ni.begin(),ni.end());
        for(size_t s=0;s<seeds.size();s++){
            int idx=seeds[s]; if(labels[idx]==-2)labels[idx]=cid;
            if(labels[idx]!=-1)continue; labels[idx]=cid;
            tree->radiusSearch(xyz->points[idx],eps,ni,nd);
            if((int)ni.size()>=min_pts) seeds.insert(seeds.end(),ni.begin(),ni.end());
        }
        cid++;
    }
    std::vector<pcl::PointIndices> clusters(cid);
    int noise=0; for(size_t i=0;i<labels.size();i++){if(labels[i]>=0)clusters[labels[i]].indices.push_back(i);else if(labels[i]==-2)noise++;}
    std::cout<<"DBSCAN: "<<cid<<" clusters "<<noise<<" noise"<<std::endl;

    std::vector<Detection> dets; int ci=0;
    for(auto&idx:clusters){
        ci++; if((int)idx.indices.size()<min_cluster||(int)idx.indices.size()>max_cluster) continue;

        pcl::PointCloud<pcl::PointXYZI>::Ptr cl(new pcl::PointCloud<pcl::PointXYZI>);
        for(auto i:idx.indices)cl->push_back(hi->points[i]);
        Eigen::Vector4f centroid; pcl::compute3DCentroid(*cl, centroid);
        float cx=centroid[0],cy=centroid[1],cz=centroid[2];

        // Box from full
        pcl::PointCloud<pcl::PointXYZI>::Ptr bf(new pcl::PointCloud<pcl::PointXYZI>);
        for(auto&p:full->points)if(p.x>=cx-box_dx&&p.x<=cx+box_dx&&p.y>=cy-box_dy&&p.y<=cy+box_dy&&p.z>=cz-box_dz_dn&&p.z<=cz+box_dz_up)bf->push_back(p);
        if(bf->size()<200)continue;

        // SAC plane
        pcl::PointCloud<pcl::PointXYZ>::Ptr bx(new pcl::PointCloud<pcl::PointXYZ>);pcl::copyPointCloud(*bf,*bx);
        pcl::SACSegmentation<pcl::PointXYZ>seg;seg.setOptimizeCoefficients(true);seg.setModelType(pcl::SACMODEL_PLANE);
        seg.setMethodType(pcl::SAC_RANSAC);seg.setDistanceThreshold(0.05);seg.setMaxIterations(1000);seg.setInputCloud(bx);
        pcl::PointIndices::Ptr pl(new pcl::PointIndices);pcl::ModelCoefficients::Ptr pcf(new pcl::ModelCoefficients);
        seg.segment(*pl,*pcf);
        bool has_plane=(pl->indices.size()>(int)(bf->size()/10));
        float nz=0;if(has_plane&&pcf->values.size()>=4)nz=std::abs(pcf->values[2]);

        pcl::PointCloud<pcl::PointXYZI>::Ptr box(new pcl::PointCloud<pcl::PointXYZI>);
        if(has_plane){pcl::ExtractIndices<pcl::PointXYZI>ex;ex.setInputCloud(bf);ex.setIndices(pl);ex.setNegative(true);ex.filter(*box);}
        else *box=*bf;
        if(box->size()<200||!has_plane)continue;

        // k-means
        int n=box->size();std::vector<std::pair<float,int>>srt(n);
        for(int i=0;i<n;i++)srt[i]={box->points[i].intensity,i};std::sort(srt.begin(),srt.end());
        float bv=1e9;int sp=n/10;
        for(int q=n/10;q<n*9/10;q++){float ls=0,hs=0;int ln=0,hn=0;
            for(int i=0;i<q;i++){ls+=srt[i].first;ln++;}for(int i=q;i<n;i++){hs+=srt[i].first;hn++;}
            float v=0,lm=ls/ln,hm=hs/hn;
            for(int i=0;i<q;i++)v+=(srt[i].first-lm)*(srt[i].first-lm);
            for(int i=q;i<n;i++)v+=(srt[i].first-hm)*(srt[i].first-hm); if(v<bv){bv=v;sp=q;}}
        float lo_sum=0,hi_sum=0;for(int i=0;i<sp;i++)lo_sum+=srt[i].first;for(int i=sp;i<n;i++)hi_sum+=srt[i].first;
        float ratio=(hi_sum/(n-sp))/(lo_sum/sp+0.1f);int lo_n=sp,hi_n=n-sp;

        // Hi centroid
        float hx=0,hy=0,hz=0;for(int i=sp;i<n;i++){int j=srt[i].second;hx+=box->points[j].x;hy+=box->points[j].y;hz+=box->points[j].z;}hx/=hi_n;hy/=hi_n;hz/=hi_n;

        // Z-crop + compact
        float z_max=-1e9;for(int i=sp;i<n;i++){int j=srt[i].second;if(box->points[j].z>z_max)z_max=box->points[j].z;}
        float z_lo=z_max-0.24f;
        pcl::PointCloud<pcl::PointXYZ>::Ptr crop(new pcl::PointCloud<pcl::PointXYZ>);
        for(int i=sp;i<n;i++){int j=srt[i].second;if(box->points[j].z>=z_lo){pcl::PointXYZ pt;pt.x=box->points[j].x;pt.y=box->points[j].y;pt.z=box->points[j].z;crop->push_back(pt);}}
        int crop_n=crop->size();
        pcl::search::KdTree<pcl::PointXYZ>::Ptr ct(new pcl::search::KdTree<pcl::PointXYZ>);ct->setInputCloud(crop);
        std::vector<pcl::PointIndices>hc;pcl::EuclideanClusterExtraction<pcl::PointXYZ>ec2;
        ec2.setClusterTolerance(0.10);ec2.setMinClusterSize(5);ec2.setMaxClusterSize(50000);ec2.setSearchMethod(ct);ec2.setInputCloud(crop);ec2.extract(hc);
        int max_cl=0;for(auto&c:hc)if((int)c.indices.size()>max_cl)max_cl=c.indices.size();
        float compact=crop_n>0?(float)max_cl/crop_n:0;

        // PCA l2/l3
        float l2l3=1;if(max_cl>=10){/* simplified: use crop_n and assume thin */float dz=z_max-z_lo;l2l3=(dz>0.001f)?0.15f/dz:1;}

        // Tripod: lo_n >= 2*hi_n + lo angular 3 peaks
        float tripod=0;
        if(lo_n>=2*hi_n){
            int nbins=36;std::vector<int>ah(nbins,0);
            for(int i=0;i<sp;i++){int j=srt[i].second;float dx=box->points[j].x-hx,dy=box->points[j].y-hy;
                float a=atan2(dy,dx)*180/M_PI;if(a<0)a+=360;int b=a*nbins/360;if(b>=0&&b<nbins)ah[b]++;}
            float am=0;for(int v:ah)am+=v;am/=nbins;
            std::vector<int>peaks;
            for(int k=0;k<nbins;k++){int pr=k==0?ah[nbins-1]:ah[k-1];int nx=k==nbins-1?ah[0]:ah[k+1];
                if(ah[k]>=pr&&ah[k]>=nx&&ah[k]>am*1.2f&&ah[k]>=3)peaks.push_back(k);}
            std::vector<int>mg;for(size_t p=0;p<peaks.size();p++)
                {if(mg.empty()||peaks[p]-mg.back()>2)mg.push_back(peaks[p]);else if(ah[peaks[p]]>ah[mg.back()])mg.back()=peaks[p];}
            int np=mg.size();
            if(np>=3){std::vector<std::pair<int,int>>pr2;for(auto pk:mg)pr2.push_back({ah[pk],pk});std::sort(pr2.rbegin(),pr2.rend());
                if(pr2.size()>=3){int p1=pr2[0].second,p2=pr2[1].second,p3=pr2[2].second;
                    auto cd=[](int a,int b,int n){int d=abs(a-b);return std::min(d,n-d);};
                    float avg=(cd(p1,p2,nbins)+cd(p2,p3,nbins)+cd(p3,p1,nbins))/3.0f;
                    tripod=1.0f-std::min(1.0f,std::abs(avg-nbins/3.0f)/(nbins/3.0f));}}
            else if(np==2) tripod=0.3f;
        }

        std::cout<<"  C"<<ci<<" box="<<bf->size()<<" plane="<<pl->indices.size()<<" nz="<<nz
                  <<" ratio="<<ratio<<" lo="<<lo_n<<" hi="<<hi_n<<" compact="<<compact<<" tripod="<<tripod;
        if(ratio>ratio_min&&compact>compact_min&&tripod>0.5f){
            dets.push_back({hx,hy,hz,ratio,compact,l2l3,tripod,hi_n,lo_n,n});std::cout<<" ✓"<<std::endl;
            pcl::PointCloud<pcl::PointXYZI>sv;std::vector<bool>ih(n,false);for(int i=sp;i<n;i++)ih[srt[i].second]=true;
            for(int i=0;i<n;i++){auto p=box->points[i];p.intensity=ih[i]?255:50;sv.push_back(p);}
            char fn[256];snprintf(fn,sizeof(fn),"%s/match_%02d_r%.1f.pcd",out.c_str(),(int)dets.size()-1,ratio);pcl::io::savePCDFileBinary(fn,sv);}
        else std::cout<<std::endl;
    }

    std::sort(dets.begin(),dets.end(),[](auto&a,auto&b){return a.tripod*a.ratio>b.tripod*b.ratio;});
    std::ofstream csv(out+"/results.csv");csv<<"rank,ratio,compact,l2l3,tripod,cx,cy,cz,hi_n,lo_n,total_n\n";
    std::cout<<"\n=== Detections ("<<dets.size()<<") ==="<<std::endl;
    for(size_t i=0;i<dets.size();i++){auto&d=dets[i];
        std::cout<<"  #"<<i<<" r="<<d.ratio<<" c="<<d.compact<<" tripod="<<d.tripod<<" ("<<d.cx<<","<<d.cy<<","<<d.cz<<")"<<std::endl;
        csv<<i<<","<<d.ratio<<","<<d.compact<<","<<d.l2l3<<","<<d.tripod<<","<<d.cx<<","<<d.cy<<","<<d.cz<<","<<d.hi_n<<","<<d.lo_n<<","<<d.total_n<<"\n";}
    csv.close();std::cout<<"Done. "<<out<<"/"<<std::endl;
}
