cmake_minimum_required(VERSION 3.16)
project(pm_settings VERSION 0.1.0 LANGUAGES CXX)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(ament_cmake REQUIRED)
find_package(core_utils REQUIRED)
find_package(core_msgs REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_srvs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(shape_msgs REQUIRED)
find_package(moveit_msgs REQUIRED)
find_package(moveit_ros_planning_interface REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)
find_package(visualization_msgs REQUIRED)
find_package(fmt REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(Boost REQUIRED COMPONENTS json)

set(PM_SETTINGS_DEPENDENCIES
  core_utils
  core_msgs
  rclcpp
  std_srvs
  geometry_msgs
  shape_msgs
  moveit_msgs
  moveit_ros_planning_interface
  tf2
  tf2_ros
  tf2_geometry_msgs
  visualization_msgs
)

# ---- Core library ------------------------------------------------------------
add_library(${PROJECT_NAME}_core SHARED
  src/persistence/json_serializer.cpp
  src/persistence/yaml_serializer.cpp
  src/persistence/config_file_manager.cpp

  src/validators/tcp_validator.cpp
  src/validators/mounting_validator.cpp
  src/validators/joint_limits_validator.cpp
  src/validators/scene_object_validator.cpp
  src/validators/validator_registry.cpp

  src/adapters/moveit_adapter.cpp
  src/adapters/controller_adapter.cpp

  src/handlers/tcp_handler.cpp
  src/handlers/mounting_handler.cpp
  src/handlers/joint_limits_handler.cpp
  src/handlers/scene_handler.cpp

  src/core/settings_manager.cpp
  src/core/settings_service_node.cpp
)

target_include_directories(${PROJECT_NAME}_core PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

ament_target_dependencies(${PROJECT_NAME}_core ${PM_SETTINGS_DEPENDENCIES})

target_link_libraries(${PROJECT_NAME}_core
  Boost::json
  fmt::fmt
  yaml-cpp
)

# ---- Executable --------------------------------------------------------------
add_executable(${PROJECT_NAME}_node src/main.cpp)

target_link_libraries(${PROJECT_NAME}_node ${PROJECT_NAME}_core)

ament_target_dependencies(${PROJECT_NAME}_node ${PM_SETTINGS_DEPENDENCIES})

# ---- Install -----------------------------------------------------------------
install(TARGETS ${PROJECT_NAME}_core
  EXPORT ${PROJECT_NAME}_coreTargets
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(TARGETS ${PROJECT_NAME}_node
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY include/
  DESTINATION include
)

install(DIRECTORY config
  DESTINATION share/${PROJECT_NAME}
)

ament_export_include_directories(include)
ament_export_libraries(${PROJECT_NAME}_core)
ament_export_dependencies(${PM_SETTINGS_DEPENDENCIES})

# ---- Tests -------------------------------------------------------------------
# if(BUILD_TESTING)
#   add_subdirectory(test)
# endif()

ament_package()
