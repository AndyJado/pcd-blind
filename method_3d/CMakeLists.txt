cmake_minimum_required(VERSION 3.5)
project(detect_spheres)

find_package(PCL 1.14 REQUIRED COMPONENTS common io filters features registration search segmentation sample_consensus)

include_directories(${PCL_INCLUDE_DIRS})
link_directories(${PCL_LIBRARY_DIRS})
add_definitions(${PCL_DEFINITIONS})

add_executable(extract_target extract_target.cpp)
target_link_libraries(extract_target ${PCL_LIBRARIES})

add_executable(match match.cpp)
target_link_libraries(match ${PCL_LIBRARIES})

add_executable(save_fpfh save_fpfh.cpp)
target_link_libraries(save_fpfh ${PCL_LIBRARIES})

add_executable(fpfh_scan fpfh_scan.cpp)
target_link_libraries(fpfh_scan ${PCL_LIBRARIES})

add_executable(pca_check pca_check.cpp)
target_link_libraries(pca_check ${PCL_LIBRARIES})

add_executable(detect_pca detect_pca.cpp)
target_link_libraries(detect_pca ${PCL_LIBRARIES})

add_executable(strip_tunnel strip_tunnel.cpp)
target_link_libraries(strip_tunnel ${PCL_LIBRARIES})

add_executable(peel_tunnel peel_tunnel.cpp)
target_link_libraries(peel_tunnel ${PCL_LIBRARIES})

add_executable(strip_ground strip_ground.cpp)
target_link_libraries(strip_ground ${PCL_LIBRARIES})

add_executable(check_pca_dir check_pca_dir.cpp)
target_link_libraries(check_pca_dir ${PCL_LIBRARIES})

add_executable(preprocess preprocess.cpp)
target_link_libraries(preprocess ${PCL_LIBRARIES})

add_executable(intensity_profile intensity_profile.cpp)
target_link_libraries(intensity_profile ${PCL_LIBRARIES})

add_executable(show_ground show_ground.cpp)
target_link_libraries(show_ground ${PCL_LIBRARIES})
add_executable(strip_ground_single strip_ground_single.cpp)
target_link_libraries(strip_ground_single ${PCL_LIBRARIES})

add_executable(analyze_2d analyze_2d.cpp)
target_link_libraries(analyze_2d ${PCL_LIBRARIES})
