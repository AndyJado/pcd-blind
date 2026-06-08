#!/bin/bash
echo "PCL version:" && dpkg -l libpcl-dev 2>/dev/null | grep libpcl-dev
echo ""
echo "Headers:" && ls /usr/include/pcl-*/pcl/ 2>/dev/null | head -5
echo ""
echo "Libs:" && ls /usr/lib/x86_64-linux-gnu/libpcl_common* 2>/dev/null | head -3
echo ""
echo "cmake:" && cmake --version 2>/dev/null | head -1
echo "g++:" && g++ --version 2>/dev/null | head -1
