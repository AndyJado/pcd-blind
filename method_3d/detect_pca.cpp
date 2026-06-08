/**
 * detect.cpp — clean: config file + no downsampling by default
 */
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/segmentation/sac_segmentation.h>
#include <pcl/segmentation/extract_clusters.h>
#include <pcl/segmentation/region_growing.h>
#include <pcl/features/normal_3d.h>
#include <pcl/search/kdtree.h>
#include <pcl/common/centroid.h>
#include <pcl/common/common.h>
#include <pcl/registration/icp.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <sys/stat.h>

struct Candidate { float cx,cy,cz,icp_fitness; int n; float score; };

int main(int argc, char** argv) {
    if (argc < 3) { std::cerr<<"Usage: "<<argv[0]<<" <scene> <tpl1..> <out> [--voxel X]\n"; return 1; }
    std::string scene_path=argv[1], out=argv[argc-1];
    mkdir(out.c_str(),0755);

    // Read config
    float voxel_sz=0, cluster_tol=0.10, gm=0.1, rh=4.0;
    { std::ifstream cfg("config.txt"); std::string l;
      if(cfg.is_open()) while(std::getline(cfg,l)){
        if(l.rfind("voxel=",0)==0) voxel_sz=std::stof(l.substr(6));
        if(l.rfind("cluster_tol=",0)==0) cluster_tol=std::stof(l.substr(13));
        if(l.rfind("ground_pct=",0)==0) gp=std::stoi(l.substr(11));
        if(l.rfind("ground_margin=",0)==0) gm=std::stof(l.substr(14));
        if(l.rfind("roof_height=",0)==0) rh=std::stof(l.substr(12));
    }}
    // CLI override
    for(int i=1;i<argc;i++){std::string a=argv[i];
        if(a=="--voxel"&&i+1<argc) voxel_sz=std::stof(argv[++i]);
        if(a=="--cluster"&&i+1<argc) cluster_tol=std::stof(argv[++i]);
    }
    std::cout<<"vox="<<voxel_sz<<" gm="<<gm<<" rh="<<rh<<" ct="<<cluster_tol<<"\n";

    // Load
    pcl::PointCloud<pcl::PointXYZI>::Ptr si(new pcl::PointCloud<pcl::PointXYZI>);
    pcl::io::loadPCDFile<pcl::PointXYZI>(scene_path,*si);
    std::cout<<"Scene: "<<si->size()<<" pts\n";

    // Templates
    std::vector<pcl::PointCloud<pcl::PointXYZ>::Ptr> tpls;
    for(int i=2;i<argc-1;i++){if(argv[i][0]=='-')continue;
        auto t=std::make_shared<pcl::PointCloud<pcl::PointXYZ>>();
        pcl::io::loadPCDFile<pcl::PointXYZ>(argv[i],*t); tpls.push_back(t);
    }

    // Voxel (skip if 0)
    pcl::PointCloud<pcl::PointXYZI>::Ptr ds(new pcl::PointCloud<pcl::PointXYZI>);
    if(voxel_sz>0.001){pcl::VoxelGrid<pcl::PointXYZI> vg;vg.setInputCloud(si);vg.setLeafSize(voxel_sz,voxel_sz,voxel_sz);vg.filter(*ds);}
    else *ds=*si;
    pcl::PointCloud<pcl::PointXYZ>::Ptr xyz(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::copyPointCloud(*ds,*xyz);
    std::cout<<"Vox: "<<xyz->size()<<" pts\n";

    // Ground: P1 + margin. Roof: ground + rh.
    {std::vector<float> zs;zs.reserve(xyz->size());
     for(auto&p:xyz->points)zs.push_back(p.z);
     std::sort(zs.begin(),zs.end());
     float gz=zs[zs.size()*gp/100];
     float zlo=gz+gm,zhi=gz+rh;
     std::cout<<"Ground="<<gz<<" cut Z<"<<zlo<<" Z>"<<zhi<<"\n";
     pcl::PointIndices::Ptr rm(new pcl::PointIndices);
     for(size_t i=0;i<xyz->size();i++){float z=xyz->points[i].z;if(z<zlo||z>zhi)rm->indices.push_back(i);}
     std::cout<<"  removed "<<rm->indices.size()<<" pts\n";
     pcl::ExtractIndices<pcl::PointXYZ> ex;ex.setInputCloud(xyz);ex.setIndices(rm);ex.setNegative(true);
     pcl::PointCloud<pcl::PointXYZ>::Ptr t(new pcl::PointCloud<pcl::PointXYZ>);ex.filter(*t);xyz.swap(t);
     pcl::ExtractIndices<pcl::PointXYZI> exi;exi.setInputCloud(ds);exi.setIndices(rm);exi.setNegative(true);
     pcl::PointCloud<pcl::PointXYZI>::Ptr ti(new pcl::PointCloud<pcl::PointXYZI>);exi.filter(*ti);ds.swap(ti);}
    // Peel
    {pcl::PointCloud<pcl::Normal>::Ptr n(new pcl::PointCloud<pcl::Normal>);
     pcl::search::KdTree<pcl::PointXYZ>::Ptr tr(new pcl::search::KdTree<pcl::PointXYZ>);
     pcl::NormalEstimation<pcl::PointXYZ,pcl::Normal> ne;ne.setInputCloud(xyz);ne.setSearchMethod(tr);ne.setRadiusSearch(0.15);ne.compute(*n);
     pcl::RegionGrowing<pcl::PointXYZ,pcl::Normal> rg;rg.setMinClusterSize(50);rg.setMaxClusterSize(10000000);
     rg.setSearchMethod(tr);rg.setNumberOfNeighbours(30);rg.setInputCloud(xyz);rg.setInputNormals(n);
     rg.setSmoothnessThreshold(4.0/180.0*M_PI);rg.setCurvatureThreshold(1.0);
     std::vector<pcl::PointIndices> regs;rg.extract(regs);
     std::vector<std::pair<int,int>> rk;for(size_t i=0;i<regs.size();i++)rk.push_back({(int)regs[i].indices.size(),(int)i});
     std::sort(rk.rbegin(),rk.rend());int pn=std::min(3,(int)rk.size());
     std::cout<<"Peel "<<pn<<":";for(int k=0;k<pn;k++)std::cout<<" "<<rk[k].first;std::cout<<"\n";
     std::vector<bool> keep(xyz->size(),true);
     for(int k=0;k<pn;k++)for(auto i:regs[rk[k].second].indices)keep[i]=false;
     pcl::PointCloud<pcl::PointXYZ>::Ptr t(new pcl::PointCloud<pcl::PointXYZ>);
     pcl::PointCloud<pcl::PointXYZI>::Ptr ti(new pcl::PointCloud<pcl::PointXYZI>);
     for(size_t i=0;i<xyz->size();i++)if(keep[i]){t->push_back(xyz->points[i]);ti->push_back(ds->points[i]);}
     xyz.swap(t);ds.swap(ti);
     std::cout<<"Remain: "<<ds->size()<<" pts\n";
     pcl::io::savePCDFileBinary(out+"/preprocessed.pcd",*ds);}

    // 2D cluster
    pcl::PointCloud<pcl::PointXYZ>::Ptr xy2d(new pcl::PointCloud<pcl::PointXYZ>);
    for(auto&p:xyz->points){pcl::PointXYZ pt;pt.x=p.x;pt.y=p.y;pt.z=0;xy2d->push_back(pt);}
    pcl::search::KdTree<pcl::PointXYZ>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZ>);tree->setInputCloud(xy2d);
    std::vector<pcl::PointIndices> clusters;
    pcl::EuclideanClusterExtraction<pcl::PointXYZ> ec;ec.setClusterTolerance(cluster_tol);ec.setMinClusterSize(30);ec.setMaxClusterSize(50000);
    ec.setSearchMethod(tree);ec.setInputCloud(xy2d);ec.extract(clusters);
    std::cout<<"Clusters: "<<clusters.size()<<"\n";

    // Score: 2D features
    auto sf=[](float v,float lo,float hi,float w){if(v>=lo&&v<=hi)return w;float d=std::min(std::abs(v-lo),std::abs(v-hi));float s=(hi-lo)*0.5f+0.1f;return w*std::max(0.0f,1.0f-d/s);};
    struct Scored{float score;Candidate c;};
    std::vector<Scored> scored;
    for(auto&idx:clusters){
        pcl::PointCloud<pcl::PointXYZI>::Ptr cl(new pcl::PointCloud<pcl::PointXYZI>);
        for(auto i:idx.indices)cl->push_back(ds->points[i]);
        int n=cl->size();if(n<50)continue;
        std::vector<float> xs,ys,is;for(auto&p:cl->points){xs.push_back(p.x);ys.push_back(p.y);is.push_back(p.intensity);}
        float xmin=*std::min_element(xs.begin(),xs.end()),xmax=*std::max_element(xs.begin(),xs.end());
        float ymin=*std::min_element(ys.begin(),ys.end()),ymax=*std::max_element(ys.begin(),ys.end());
        float area=(xmax-xmin)*(ymax-ymin);if(area<0.2||area>3.0)continue;
        float density=n/(area+0.001f);
        float imean=0,istd=0;for(auto v:is)imean+=v;imean/=n;for(auto v:is)istd+=(v-imean)*(v-imean);istd=sqrt(istd/n);
        float s=sf(area,0.3f,1.5f,4.0f);s+=sf(density,500.0f,5000.0f,3.0f);s+=sf(imean,5.0f,50.0f,3.0f);s+=sf(istd,15.0f,60.0f,3.0f);
        Eigen::Vector4f ct;pcl::compute3DCentroid(*cl,ct);
        Candidate c={ct[0],ct[1],ct[2],0.0f,n,s};
        scored.push_back({s,c});
    }
    std::sort(scored.begin(),scored.end(),[](auto&a,auto&b){return a.score>b.score;});
    std::vector<Candidate> cands;
    std::cout<<"Top 10:\n";
    for(int i=0;i<std::min(30,(int)scored.size());i++){auto&sc=scored[i];cands.push_back(sc.c);
        if(i<10)std::cout<<"  #"<<i<<" s="<<sc.score<<" ("<<sc.c.cx<<","<<sc.c.cy<<","<<sc.c.cz<<") n="<<sc.c.n<<"\n";}

    // ICP
    for(auto&c:cands){float best=999;float R=1.2f;
        pcl::PointCloud<pcl::PointXYZ>::Ptr reg(new pcl::PointCloud<pcl::PointXYZ>);
        for(auto&p:xyz->points)if(p.x>=c.cx-R&&p.x<=c.cx+R&&p.y>=c.cy-R&&p.y<=c.cy+R&&p.z>=c.cz-R&&p.z<=c.cz+R)reg->push_back(p);
        if(reg->size()<50){c.icp_fitness=999;continue;}
        for(auto&t:tpls){pcl::IterativeClosestPoint<pcl::PointXYZ,pcl::PointXYZ>icp;icp.setInputSource(t);icp.setInputTarget(reg);icp.setMaximumIterations(30);icp.setMaxCorrespondenceDistance(1.0);pcl::PointCloud<pcl::PointXYZ>al;icp.align(al);if(icp.hasConverged())best=std::min(best,(float)icp.getFitnessScore());}
        c.icp_fitness=best;
    }
    std::sort(cands.begin(),cands.end(),[](auto&a,auto&b){return a.icp_fitness<b.icp_fitness;});
    std::ofstream csv(out+"/results.csv");csv<<"rank,icp_fitness,cx,cy,cz,score,n\n";
    for(int i=0;i<std::min(20,(int)cands.size());i++){auto&c=cands[i];
        if(i<10)std::cout<<"  #"<<i<<" fit="<<c.icp_fitness<<" ("<<c.cx<<","<<c.cy<<","<<c.cz<<")\n";
        csv<<i<<","<<c.icp_fitness<<","<<c.cx<<","<<c.cy<<","<<c.cz<<","<<c.score<<","<<c.n<<"\n";
        pcl::PointCloud<pcl::PointXYZI>::Ptr r(new pcl::PointCloud<pcl::PointXYZI>);
        float R=1.5f;for(auto&p:ds->points)if(p.x>=c.cx-R&&p.x<=c.cx+R&&p.y>=c.cy-R&&p.y<=c.cy+R&&p.z>=c.cz-R&&p.z<=c.cz+R)r->push_back(p);
        char fn[128];snprintf(fn,sizeof(fn),"%s/match_%02d_f%.4f.pcd",out.c_str(),i,c.icp_fitness);pcl::io::savePCDFileBinary(fn,*r);
    }
    csv.close();
    // Save all scored
    std::ofstream ac(out+"/all_clusters.csv");ac<<"cx,cy,cz,score,n\n";for(auto&sc:scored)ac<<sc.c.cx<<","<<sc.c.cy<<","<<sc.c.cz<<","<<sc.score<<","<<sc.c.n<<"\n";ac.close();
    std::cout<<"Done. "<<out<<"/\n";
}
