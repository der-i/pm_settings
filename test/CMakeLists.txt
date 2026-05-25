find_package(ament_cmake_gtest REQUIRED)
find_package(ament_lint_auto REQUIRED)

ament_lint_auto_find_test_dependencies()

# ---- Unit-тесты валидаторов -------------------------------------------
ament_add_gtest(test_tcp_validator test_tcp_validator.cpp)
target_link_libraries(test_tcp_validator ${PROJECT_NAME}_core)
ament_target_dependencies(test_tcp_validator ${PM_SETTINGS_DEPENDENCIES})

ament_add_gtest(test_mounting_validator test_mounting_validator.cpp)
target_link_libraries(test_mounting_validator ${PROJECT_NAME}_core)
ament_target_dependencies(test_mounting_validator ${PM_SETTINGS_DEPENDENCIES})

ament_add_gtest(test_joint_limits_validator test_joint_limits_validator.cpp)
target_link_libraries(test_joint_limits_validator ${PROJECT_NAME}_core)
ament_target_dependencies(test_joint_limits_validator ${PM_SETTINGS_DEPENDENCIES})

ament_add_gtest(test_scene_object_validator test_scene_object_validator.cpp)
target_link_libraries(test_scene_object_validator ${PROJECT_NAME}_core)
ament_target_dependencies(test_scene_object_validator ${PM_SETTINGS_DEPENDENCIES})

# ---- Unit-тесты persistence -------------------------------------------
ament_add_gtest(test_json_serializer test_json_serializer.cpp)
target_link_libraries(test_json_serializer ${PROJECT_NAME}_core)
ament_target_dependencies(test_json_serializer ${PM_SETTINGS_DEPENDENCIES})

ament_add_gtest(test_yaml_serializer test_yaml_serializer.cpp)
target_link_libraries(test_yaml_serializer ${PROJECT_NAME}_core)
ament_target_dependencies(test_yaml_serializer ${PM_SETTINGS_DEPENDENCIES})

ament_add_gtest(test_config_file_manager test_config_file_manager.cpp)
target_link_libraries(test_config_file_manager ${PROJECT_NAME}_core)
ament_target_dependencies(test_config_file_manager ${PM_SETTINGS_DEPENDENCIES})

# ---- Unit-тесты core --------------------------------------------------
ament_add_gtest(test_validator_registry test_validator_registry.cpp)
target_link_libraries(test_validator_registry ${PROJECT_NAME}_core)
ament_target_dependencies(test_validator_registry ${PM_SETTINGS_DEPENDENCIES})

ament_add_gtest(test_settings_manager test_settings_manager.cpp)
target_link_libraries(test_settings_manager ${PROJECT_NAME}_core)
ament_target_dependencies(test_settings_manager ${PM_SETTINGS_DEPENDENCIES})

# ---- Интеграционные тесты ---------------------------------------------
ament_add_gtest(test_tcp_services_integration test_tcp_services_integration.cpp)
target_include_directories(test_tcp_services_integration PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_tcp_services_integration ${PROJECT_NAME}_core)
ament_target_dependencies(test_tcp_services_integration ${PM_SETTINGS_DEPENDENCIES})

ament_add_gtest(test_base_link_services_integration
  test_base_link_services_integration.cpp)
target_include_directories(test_base_link_services_integration
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_base_link_services_integration ${PROJECT_NAME}_core)
ament_target_dependencies(test_base_link_services_integration ${PM_SETTINGS_DEPENDENCIES})

ament_add_gtest(test_objects_services_integration
  test_objects_services_integration.cpp)
target_include_directories(test_objects_services_integration
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_objects_services_integration ${PROJECT_NAME}_core)
ament_target_dependencies(test_objects_services_integration ${PM_SETTINGS_DEPENDENCIES})

ament_add_gtest(test_work_frame_services_integration
  test_work_frame_services_integration.cpp)
target_include_directories(test_work_frame_services_integration
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_work_frame_services_integration ${PROJECT_NAME}_core)
ament_target_dependencies(test_work_frame_services_integration ${PM_SETTINGS_DEPENDENCIES})