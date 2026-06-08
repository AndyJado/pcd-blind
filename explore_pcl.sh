#!/bin/bash
echo "=== PCL CLI tools ==="
dpkg -L libpcl-dev 2>/dev/null | grep bin/ | head -20
echo ""
echo "=== PCL headers (SAC/RANSAC相关) ==="
find /usr/include/pcl-* -name "sac*.h" -o -name "sphere*.h" -o -name "ransac*.h" 2>/dev/null | head -20
echo ""
echo "=== PCL apps 工具 ==="
dpkg -L libpcl-apps1.14 2>/dev/null | grep bin/ | head -20
echo ""
echo "=== pcl_sac 头文件 ==="
ls /usr/include/pcl-*/pcl/sample_consensus/ 2>/dev/null | head -20
echo ""
echo "=== 球相关方法 ==="
grep -r "SACMODEL_SPHERE\|setRadius\|setModelType.*SPHERE" /usr/include/pcl-*/pcl/sample_consensus/ 2>/dev/null | head -10
