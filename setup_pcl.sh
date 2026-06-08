#!/bin/bash
echo "=== Already installed? ==="
dpkg -l 2>/dev/null | grep libpcl | head -10
echo ""
echo "=== pcl_config? ==="
which pcl_config 2>/dev/null && pcl_config --version || echo "not found"
echo ""
echo "=== apt list available ==="
apt list --installed 2>/dev/null | grep pcl | head -10 || echo "n/a"
echo ""
echo "=== Can sudo without password? ==="
sudo -n true 2>&1 && echo "YES passwordless sudo" || echo "NO - needs password"
