/**
 * detect_targets.cpp — Zslice + Cylinder + Concentric XY
 */
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/segmentation/sac_segmentation.h>
#include <pcl/segmentation/extract_clusters.h>
#include <pcl/search/kdtree.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <sys/stat.h>

struct Detection { float cx,cy,cz,ratio,compact,tripod,dxy; int hi_n,lo_n,total_n; };
struct C3 { float x,y,z; };
struct Config { float pct=80,ratio=5,voxel=0,compact_min=0.88f,cyl_r=0.5f,z_start=3.0f,z_step=0.2f,z_end=-5.0f; int min_clus=100,max_clus=50000; };
Config load_config(const std::string& s){
    Config c;std::ifstream f("config_detect.txt");if(!f.is_open())return c;
    std::string l,sec,fn=s;size_t p=fn.rfind('/');if(p!=std::string::npos)fn=fn.substr(p+1);
    while(std::getline(f,l)){while(!l.empty()&&(l.back()=='\r'||l.back()==' '))l.pop_back();
        if(l.empty()||l[0]=='#')continue;if(l[0]=='['){sec=l.substr(1,l.find(']')-1);continue;}
        bool m=(sec=="default")||(fn.find(sec)!=std::string::npos);if(!m)continue;
        size_t e=l.find('=');if(e==std::string::npos)continue;std::string k=l.substr(0,e);float v=atof(l.substr(e+1).c_str());
        while(!k.empty()&&(k.back()==' '||k.back()=='\t'))k.pop_back();
        if(k=="pct")c.pct=v;if(k=="ratio")c.ratio=v;if(k=="voxel")c.voxel=v;if(k=="min_clus")c.min_clus=(int)v;if(k=="max_clus")c.max_clus=(int)v;
        if(k=="compact_min")c.compact_min=v;if(k=="cyl_r")c.cyl_r=v;if(k=="z_start")c.z_start=v;if(k=="z_step")c.z_step=v;if(k=="z_end")c.z_end=v;}
    return c;}

int main(int argc,char**argv){
    if(argc<3){std::cerr<<"Usage: "<<argv[0]<<" <scene> <out>\n";return 1;}
    std::string scene=argv[1],out=argv[2];mkdir(out.c_str(),0755);
    Config cfg=load_config(scene);
    float pct_thr=cfg.pct,ratio_min=cfg.ratio,voxel_sz=cfg.voxel,compact_min=cfg.compact_min;
    int min_cluster=cfg.min_clus,max_cluster=cfg.max_clus;
    float box_dz_up=0.3f,box_dz_dn=2.0f,z_start=cfg.z_start,z_step=cfg.z_step,z_end=cfg.z_end,cyl_r=cfg.cyl_r;

    // Load
    pcl::PointCloud<pcl::PointXYZI>::Ptr full(new pcl::PointCloud<pcl::PointXYZI>);
    pcl::io::loadPCDFile<pcl::PointXYZI>(scene,*full);
    std::cout<<"Loaded "<<full->size()<<" pts"<<std::endl;
    if(voxel_sz>0.001f){/*voxel*/}

    std::vector<float>Is;for(auto&p:full->points)Is.push_back(p.intensity);std::sort(Is.begin(),Is.end());
    float Ith=Is[Is.size()*(int)pct_thr/100];
    std::cout<<"I>P"<<(int)pct_thr<<"="<<Ith<<std::endl;

    std::vector<C3> used;
    std::vector<Detection> dets;

    for(float z=z_start;z>z_end;z-=z_step){
        pcl::PointCloud<pcl::PointXYZ>::Ptr sl(new pcl::PointCloud<pcl::PointXYZ>);
        for(auto&p:full->points)if(p.z>=z&&p.z<z+z_step&&p.intensity>=Ith){pcl::PointXYZ pt;pt.x=p.x;pt.y=p.y;pt.z=0;sl->push_back(pt);}
        if(sl->size()<50)continue;
        pcl::search::KdTree<pcl::PointXYZ>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZ>);tree->setInputCloud(sl);
        std::vector<pcl::PointIndices> cl; pcl::EuclideanClusterExtraction<pcl::PointXYZ>ec;
        ec.setClusterTolerance(0.15);ec.setMinClusterSize(20);ec.setMaxClusterSize(5000);ec.setSearchMethod(tree);ec.setInputCloud(sl);ec.extract(cl);
        for(auto&idx:cl){if((int)idx.indices.size()<20)continue;
            float cx=0,cy=0;for(auto i:idx.indices){cx+=sl->points[i].x;cy+=sl->points[i].y;}cx/=idx.indices.size();cy/=idx.indices.size();
            bool dup=false;for(auto&u:used)if(fabs(cx-u.x)<0.3f&&fabs(cy-u.y)<0.3f&&fabs(z-u.z)<z_step*1.5f){dup=true;break;}
            if(dup)continue;used.push_back({cx,cy,z});

            float cz_center=z;
            for(int pass=0;pass<2;pass++){
                pcl::PointCloud<pcl::PointXYZI>::Ptr bf(new pcl::PointCloud<pcl::PointXYZI>);
                for(auto&p:full->points){float rx=p.x-cx,ry=p.y-cy;if(rx*rx+ry*ry<=cyl_r*cyl_r&&p.z>=cz_center-box_dz_dn&&p.z<=cz_center+box_dz_up)bf->push_back(p);}
                if(bf->size()<200)break;
                pcl::PointCloud<pcl::PointXYZ>::Ptr bx(new pcl::PointCloud<pcl::PointXYZ>);pcl::copyPointCloud(*bf,*bx);
                pcl::SACSegmentation<pcl::PointXYZ>seg;seg.setOptimizeCoefficients(true);seg.setModelType(pcl::SACMODEL_PLANE);seg.setMethodType(pcl::SAC_RANSAC);seg.setDistanceThreshold(0.05);seg.setMaxIterations(1000);seg.setInputCloud(bx);
                pcl::PointIndices::Ptr pl(new pcl::PointIndices);pcl::ModelCoefficients::Ptr pcf(new pcl::ModelCoefficients);seg.segment(*pl,*pcf);
                bool hp=pl->indices.size()>(int)(bf->size()/10);float nz=hp&&pcf->values.size()>=4?std::abs(pcf->values[2]):0;bool is_ground=hp&&nz>0.7f;
                pcl::PointCloud<pcl::PointXYZI>::Ptr box(new pcl::PointCloud<pcl::PointXYZI>);
                if(is_ground){pcl::ExtractIndices<pcl::PointXYZI>ex;ex.setInputCloud(bf);ex.setIndices(pl);ex.setNegative(true);ex.filter(*box);}else *box=*bf;
                if(box->size()<200||!hp)break;
                int n=box->size();std::vector<std::pair<float,int>>srt(n);
                for(int i=0;i<n;i++)srt[i]={box->points[i].intensity,i};std::sort(srt.begin(),srt.end());
                float bv=1e9;int sp=n/10;
                for(int q=n/10;q<n*9/10;q++){float ls=0,hs=0;int ln=0,hn=0;
                    for(int i=0;i<q;i++){ls+=srt[i].first;ln++;}for(int i=q;i<n;i++){hs+=srt[i].first;hn++;}
                    float lm=ls/ln,hm=hs/hn,v=0;for(int i=0;i<q;i++)v+=(srt[i].first-lm)*(srt[i].first-lm);
                    for(int i=q;i<n;i++)v+=(srt[i].first-hm)*(srt[i].first-hm);if(v<bv){bv=v;sp=q;}}
                float ls_=0,hs_=0;for(int i=0;i<sp;i++)ls_+=srt[i].first;for(int i=sp;i<n;i++)hs_+=srt[i].first;
                float ratio=(hs_/(n-sp))/(ls_/sp+0.1f);int lo_n=sp,hi_n=n-sp;
                if(ratio<=ratio_min){if(pass==0)break;continue;}
                float hx=0,hy=0,hz_=0;for(int i=sp;i<n;i++){int j=srt[i].second;hx+=box->points[j].x;hy+=box->points[j].y;hz_+=box->points[j].z;}hx/=hi_n;hy/=hi_n;hz_/=hi_n;
                float lx=0,ly=0;for(int i=0;i<sp;i++){int j=srt[i].second;lx+=box->points[j].x;ly+=box->points[j].y;}lx/=sp;ly/=sp;
                float dxy=sqrt((lx-hx)*(lx-hx)+(ly-hy)*(ly-hy));
                if(pass==0 && dxy>0.15f){cx=(lx+hx)/2;cy=(ly+hy)/2;continue;}
                // Z-crop compact
                float z_max=-1e9;for(int i=sp;i<n;i++){int j=srt[i].second;if(box->points[j].z>z_max)z_max=box->points[j].z;}
                pcl::PointCloud<pcl::PointXYZ>::Ptr crop(new pcl::PointCloud<pcl::PointXYZ>);
                for(int i=sp;i<n;i++){int j=srt[i].second;if(box->points[j].z>=z_max-0.24f){pcl::PointXYZ pt;pt.x=box->points[j].x;pt.y=box->points[j].y;pt.z=box->points[j].z;crop->push_back(pt);}}
                int crop_n=crop->size();float cmx=0,cmy=0,cmz=0;
                for(auto&p:crop->points){cmx+=p.x;cmy+=p.y;cmz+=p.z;}cmx/=crop_n;cmy/=crop_n;cmz/=crop_n;
                float cvar=0;for(auto&p:crop->points){float dx=p.x-cmx,dy=p.y-cmy,dz=p.z-cmz;cvar+=dx*dx+dy*dy+dz*dz;}
                float compact=crop_n>0?1.0f/(1.0f+sqrt(cvar/crop_n)):0;
                if(compact<compact_min)break;
                // Tripod (disabled for now)
                float tripod=0;
                dets.push_back({hx,hy,hz_,ratio,compact,tripod,dxy,hi_n,lo_n,n});
                pcl::PointCloud<pcl::PointXYZI>sv;std::vector<bool>ih(n,false);for(int i=sp;i<n;i++)ih[srt[i].second]=true;
                for(int i=0;i<n;i++){auto p=box->points[i];p.intensity=ih[i]?255:50;sv.push_back(p);}
                char fn[256];snprintf(fn,sizeof(fn),"%s/match_%02d_r%.1f.pcd",out.c_str(),(int)dets.size()-1,ratio);pcl::io::savePCDFileBinary(fn,sv);
                break;
            }
        }
    }
    std::sort(dets.begin(),dets.end(),[](auto&a,auto&b){if(a.compact!=b.compact)return a.compact>b.compact;return a.dxy<b.dxy;});
    std::ofstream csv(out+"/results.csv");csv<<"rank,ratio,compact,tripod,dxy,cx,cy,cz,hi_n,lo_n,total_n\n";
    for(size_t i=0;i<dets.size();i++){auto&d=dets[i];csv<<i<<","<<d.ratio<<","<<d.compact<<","<<d.tripod<<","<<d.dxy<<","<<d.cx<<","<<d.cy<<","<<d.cz<<","<<d.hi_n<<","<<d.lo_n<<","<<d.total_n<<"\n";}
    csv.close();std::cout<<"Candidates: "<<used.size()<<" Detections: "<<dets.size()<<std::endl;
}
