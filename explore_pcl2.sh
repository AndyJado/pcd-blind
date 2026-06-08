#!/bin/bash
echo "=== SACModelSphere 完整接口 ==="
cat /usr/include/pcl-1.14/pcl/sample_consensus/sac_model_sphere.h
echo ""
echo "=== SAC model radius limits ==="
grep -A5 "setRadiusLimits\|radius_limits\|min_radius\|max_radius" /usr/include/pcl-1.14/pcl/sample_consensus/sac_model.h | head -20
echo ""
echo "=== PCL examples ==="
find /usr/share/doc/libpcl-dev* -name "*.cpp" 2>/dev/null | head -10
echo ""
echo "=== Sphere example search ==="
apt-file search "sphere" 2>/dev/null | grep pcl | head -10 || echo "no apt-file"
dpkg -L libpcl-doc 2>/dev/null | grep sphere | head -10 || echo "no libpcl-doc"
