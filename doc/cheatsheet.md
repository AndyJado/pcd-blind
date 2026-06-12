# pcd-blind 操作速查

## 跑管线
mfr scripts/run_detect.sh source/xxx.pcd output/xxx

## CloudCompare
Shift+I    按强度着色
Shfit+Z    按Z着色
F          适配视图
左键拖     旋转 | 滚轮缩放 | 中键平移

## 查结果
cat output/xxx/results.csv | head -20
# 搜坐标附近检测 (awk)
cat output/xxx/results.csv | awk -F, 'NR>1{d=sqrt(($6-X)^2+($7-Y)^2);if(d<2)print}'

## 手选点转模板
# 在 CloudCompare 点选 → 记坐标 → 
# 用 extract_tpls / tpl_stats 程式提取+分析

## 单点管线分析
# C++: bf->SAC->k-means->Zcrop->tripod
# 验证框本身能否被管线检出

## 模板特征批量
# tpl_stats.cpp: 遍历模板文件，输出 ratio/compact/tripod/dxy

## 参数调整
# 编辑 config_detect.txt，按 [数据集名] 分段设 pct/ratio/compact_min/cyl_r/z_*
