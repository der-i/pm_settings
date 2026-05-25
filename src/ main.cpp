/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#include <memory>

#include <ament_index_cpp/get_package_share_directory.hpp>
#include <core_utils/log.h>
#include <rclcpp/rclcpp.hpp>

#include "pm_settings/adapters/controller_adapter.hpp"
#include "pm_settings/adapters/moveit_adapter.hpp"
#include "pm_settings/core/settings_manager.hpp"
#include "pm_settings/core/settings_service_node.hpp"
#include "pm_settings/core/validator_registry.hpp"
#include "pm_settings/handlers/joint_limits_handler.hpp"
#include "pm_settings/handlers/mounting_handler.hpp"
#include "pm_settings/handlers/scene_handler.hpp"
#include "pm_settings/handlers/tcp_handler.hpp"
#include "pm_settings/persistence/config_file_manager.hpp"
#include "pm_settings/persistence/json_serializer.hpp"
#include "pm_settings/persistence/yaml_serializer.hpp"
#include "pm_settings/validators/joint_limits_validator.hpp"
#include "pm_settings/validators/mounting_validator.hpp"
#include "pm_settings/validators/scene_object_validator.hpp"
#include "pm_settings/validators/tcp_validator.hpp"

using namespace pm::settings;

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);

  // ---- Слой персистентности ------------------------------------------
  const auto config_dir = std::filesystem::path(
    ament_index_cpp::get_package_share_directory("pm_settings")) / "config";

  auto config_manager = std::make_shared<ConfigFileManager>(config_dir);
  config_manager->RegisterSerializer(std::make_shared<JSONSerializer>());
  config_manager->RegisterSerializer(std::make_shared<YAMLSerializer>());

  // ---- Слой валидации ------------------------------------------------
  auto validator_registry = std::make_shared<ValidatorRegistry>();
  validator_registry->Register(ParamType::TCP, std::make_shared<TCPValidator>());
  validator_registry->Register(ParamType::MOUNTING, std::make_shared<MountingValidator>());
  validator_registry->Register(ParamType::JOINT_LIMITS, std::make_shared<JointLimitsValidator>());
  validator_registry->Register(ParamType::SCENE_OBJECT, std::make_shared<SceneObjectValidator>());

  // ---- ROS2 узел -----------------------------------------------------
  auto node = std::make_shared<SettingsServiceNode>();

  // ---- Слой интеграции (адаптеры) ------------------------------------
  auto moveit_adapter = std::make_shared<MoveItAdapter>(node);
  auto controller_adapter = std::make_shared<ControllerAdapter>(node);

  // ---- Слой бизнес-логики (обработчики) ------------------------------
  auto tcp_handler = std::make_shared<TCPHandler>(moveit_adapter, config_manager);
  auto mounting_handler = std::make_shared<MountingHandler>(moveit_adapter, config_manager);
  auto scene_handler = std::make_shared<SceneHandler>(moveit_adapter, config_manager);
  auto joint_limits_handler =
    std::make_shared<JointLimitsHandler>(controller_adapter, config_manager);

  // Восстановление состояния с диска при старте.
  (void)tcp_handler->LoadFromDisk();
  (void)mounting_handler->LoadFromDisk();
  (void)scene_handler->LoadFromDisk();
  (void)joint_limits_handler->LoadFromDisk();

  // ---- SettingsManager -----------------------------------------------
  auto manager = std::make_shared<SettingsManager>(validator_registry, config_manager);
  manager->RegisterHandler(tcp_handler);
  manager->RegisterHandler(mounting_handler);
  manager->RegisterHandler(scene_handler);
  manager->RegisterHandler(joint_limits_handler);

  // ---- Wiring ---------------------------------------------------------
  node->Configure(manager, tcp_handler, mounting_handler, scene_handler, joint_limits_handler);

  LOG_I << "pm_settings node started";
  rclcpp::spin(node);
  rclcpp::shutdown();
  return EXIT_SUCCESS;
}