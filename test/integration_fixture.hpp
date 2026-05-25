/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#ifndef PM_SETTINGS__TEST__INTEGRATION_FIXTURE_HPP_
#define PM_SETTINGS__TEST__INTEGRATION_FIXTURE_HPP_

#include <chrono>
#include <filesystem>
#include <memory>
#include <thread>

#include <gtest/gtest.h>
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

namespace pm::settings::test
{

/// @brief Базовый fixture для интеграционных тестов.
/// @details Поднимает SettingsServiceNode в отдельном executor-потоке,
/// создаёт клиентскую ноду и временную директорию конфигов,
/// чистит всё в TearDown.
class IntegrationFixture : public ::testing::Test
{
protected:
  void SetUp() override
  {
    rclcpp::init(0, nullptr);

    // Временная директория конфигов для изоляции тестов.
    m_config_dir = std::filesystem::temp_directory_path() /
      ("pm_settings_it_" + std::to_string(::testing::UnitTest::GetInstance()->random_seed()) +
       "_" + std::to_string(std::hash<const void *>{}(this)));
    std::filesystem::create_directories(m_config_dir);

    InitConfigFiles();

    // ---- Persistence ----
    m_config_manager = std::make_shared<ConfigFileManager>(m_config_dir);
    m_config_manager->RegisterSerializer(std::make_shared<JSONSerializer>());
    m_config_manager->RegisterSerializer(std::make_shared<YAMLSerializer>());

    // ---- Validation ----
    m_validator_registry = std::make_shared<ValidatorRegistry>();
    m_validator_registry->Register(ParamType::TCP, std::make_shared<TCPValidator>());
    m_validator_registry->Register(ParamType::MOUNTING, std::make_shared<MountingValidator>());
    m_validator_registry->Register(
      ParamType::JOINT_LIMITS, std::make_shared<JointLimitsValidator>());
    m_validator_registry->Register(
      ParamType::SCENE_OBJECT, std::make_shared<SceneObjectValidator>());

    // ---- Service node ----
    rclcpp::NodeOptions options;
    options.arguments({"--ros-args", "-r", "__node:=pm_settings_test"});
    m_service_node = std::make_shared<SettingsServiceNode>(options);

    // ---- Adapters (создаются, но не подключены к реальному move_group —
    // в тестах возможны предупреждения об этом, это ожидаемое поведение) ----
    m_moveit_adapter = std::make_shared<MoveItAdapter>(m_service_node);
    m_controller_adapter = std::make_shared<ControllerAdapter>(m_service_node);

    // ---- Handlers ----
    m_tcp_handler = std::make_shared<TCPHandler>(m_moveit_adapter, m_config_manager);
    m_mounting_handler =
      std::make_shared<MountingHandler>(m_moveit_adapter, m_config_manager);
    m_scene_handler =
      std::make_shared<SceneHandler>(m_moveit_adapter, m_config_manager);
    m_joint_limits_handler =
      std::make_shared<JointLimitsHandler>(m_controller_adapter, m_config_manager);

    (void)m_tcp_handler->LoadFromDisk();
    (void)m_mounting_handler->LoadFromDisk();
    (void)m_scene_handler->LoadFromDisk();

    // ---- Manager ----
    m_manager = std::make_shared<SettingsManager>(m_validator_registry, m_config_manager);
    m_manager->RegisterHandler(m_tcp_handler);
    m_manager->RegisterHandler(m_mounting_handler);
    m_manager->RegisterHandler(m_scene_handler);
    m_manager->RegisterHandler(m_joint_limits_handler);

    m_service_node->Configure(
      m_manager, m_tcp_handler, m_mounting_handler, m_scene_handler, m_joint_limits_handler);

    // ---- Client node ----
    m_client_node = std::make_shared<rclcpp::Node>("pm_settings_test_client");

    // ---- Executors in separate threads ----
    m_server_executor = std::make_unique<rclcpp::executors::SingleThreadedExecutor>();
    m_server_executor->add_node(m_service_node);
    m_server_thread = std::thread([this]() { m_server_executor->spin(); });

    m_client_executor = std::make_unique<rclcpp::executors::SingleThreadedExecutor>();
    m_client_executor->add_node(m_client_node);
    m_client_thread = std::thread([this]() { m_client_executor->spin(); });

    // Даём сервисам подняться.
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }

  void TearDown() override
  {
    if (m_server_executor) {
      m_server_executor->cancel();
    }
    if (m_client_executor) {
      m_client_executor->cancel();
    }
    if (m_server_thread.joinable()) {
      m_server_thread.join();
    }
    if (m_client_thread.joinable()) {
      m_client_thread.join();
    }

    rclcpp::shutdown();

    std::error_code ec;
    std::filesystem::remove_all(m_config_dir, ec);
  }

  /// @brief Инициализирует конфиг-файлы пустыми структурами, чтобы избежать
  /// падений при первой загрузке.
  void InitConfigFiles() const
  {
    const auto write = [&](const std::string & name, const std::string & content) {
      std::ofstream ofs(m_config_dir / name);
      ofs << content;
    };
    write("tcp_list.json", R"({"tcps": []})");
    write("base_link_transforms.json", R"({"transforms": [], "last_used": ""})");
    write("objects.json", R"({"objects": []})");
    write("work_frame_list.json", R"({"frames": [], "last_used": ""})");
  }

  /// @brief Утилита для синхронного вызова сервиса с таймаутом.
  template <typename ServiceT>
  typename ServiceT::Response::SharedPtr CallService(
    const std::string & service_name,
    typename ServiceT::Request::SharedPtr request,
    std::chrono::milliseconds timeout = std::chrono::milliseconds(2000)) const
  {
    auto client = m_client_node->create_client<ServiceT>(service_name);
    if (!client->wait_for_service(timeout)) {
      ADD_FAILURE() << "Service '" << service_name << "' not available within timeout";
      return nullptr;
    }

    auto future = client->async_send_request(request);
    const auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < timeout) {
      if (future.wait_for(std::chrono::milliseconds(10)) == std::future_status::ready) {
        return future.get();
      }
    }
    ADD_FAILURE() << "Service '" << service_name << "' response timeout";
    return nullptr;
  }

protected:
  std::filesystem::path m_config_dir;

  std::shared_ptr<ConfigFileManager> m_config_manager;
  std::shared_ptr<ValidatorRegistry> m_validator_registry;
  std::shared_ptr<SettingsManager> m_manager;

  std::shared_ptr<MoveItAdapter> m_moveit_adapter;
  std::shared_ptr<ControllerAdapter> m_controller_adapter;

  std::shared_ptr<TCPHandler> m_tcp_handler;
  std::shared_ptr<MountingHandler> m_mounting_handler;
  std::shared_ptr<SceneHandler> m_scene_handler;
  std::shared_ptr<JointLimitsHandler> m_joint_limits_handler;

  std::shared_ptr<SettingsServiceNode> m_service_node;
  std::shared_ptr<rclcpp::Node> m_client_node;

  std::unique_ptr<rclcpp::executors::SingleThreadedExecutor> m_server_executor;
  std::unique_ptr<rclcpp::executors::SingleThreadedExecutor> m_client_executor;
  std::thread m_server_thread;
  std::thread m_client_thread;
};

}  // namespace pm::settings::test

#endif  // PM_SETTINGS__TEST__INTEGRATION_FIXTURE_HPP_