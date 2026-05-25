/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#include "pm_settings/adapters/controller_adapter.hpp"

#include <array>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <thread>

#include <ament_index_cpp/get_package_share_directory.hpp>
#include <core_utils/log.h>
#include <fmt/format.h>
#include <yaml-cpp/yaml.h>

namespace pm::settings
{

namespace
{

constexpr std::array<std::string_view, 2> kTargetNodes = {"move_group", "servo_node"};

std::string FormatYaml(double value)
{
  return fmt::format("{:.2f}", value);
}

}  // namespace

ControllerAdapter::ControllerAdapter(const std::shared_ptr<rclcpp::Node> & node)
: m_node(node)
{
  m_node->get_parameter_or("pm_type", m_pm_type, std::string{"undefined"});
}

bool ControllerAdapter::UpdateJointLimits(const JointLimitsConfiguration & limits)
{
  const auto path = GetJointLimitsPath();
  if (!std::filesystem::exists(path)) {
    LOG_E << fmt::format("ControllerAdapter: joint_limits config does not exist: {}", path);
    return false;
  }

  // Создаём резервную копию основного файла перед записью.
  try {
    std::filesystem::copy_file(
      path, path + ".bak",
      std::filesystem::copy_options::overwrite_existing);
  } catch (const std::exception & e) {
    LOG_E << fmt::format("ControllerAdapter: backup failed for {}: {}", path, e.what());
    return false;
  }

  try {
    YAML::Node root = YAML::LoadFile(path);
    if (!root["joint_limits"] || !root["joint_limits"].IsMap()) {
      root["joint_limits"] = YAML::Node(YAML::NodeType::Map);
    }

    YAML::Node node = root["joint_limits"];
    for (const auto & joint : limits.limits) {
      if (!node[joint.name] || !node[joint.name].IsMap()) {
        node[joint.name] = YAML::Node(YAML::NodeType::Map);
      }
      node[joint.name]["min_position"]    = FormatYaml(joint.min_position);
      node[joint.name]["max_position"]    = FormatYaml(joint.max_position);
      node[joint.name]["max_velocity"]    = FormatYaml(joint.max_velocity);
      node[joint.name]["max_acceleration"] = FormatYaml(joint.max_acceleration);
    }

    std::ofstream ofs(path);
    ofs << root;
  } catch (const std::exception & e) {
    LOG_E << fmt::format("ControllerAdapter: write joint_limits failed: {}", e.what());
    return false;
  }

  // Обновляем runtime params-files для запущенных узлов move_group / servo_node.
  for (const auto & runtime_path : GetRuntimeParamsFiles()) {
    try {
      YAML::Node runtime = std::filesystem::exists(runtime_path)
        ? YAML::LoadFile(runtime_path)
        : YAML::Node(YAML::NodeType::Map);

      constexpr auto root_key = "/**";
      constexpr auto params_key = "ros__parameters";
      constexpr auto limits_prefix = "robot_description_planning.joint_limits";

      if (!runtime[root_key] || !runtime[root_key].IsMap()) {
        runtime[root_key] = YAML::Node(YAML::NodeType::Map);
      }
      if (!runtime[root_key][params_key] || !runtime[root_key][params_key].IsMap()) {
        runtime[root_key][params_key] = YAML::Node(YAML::NodeType::Map);
      }

      auto & params = runtime[root_key][params_key];
      for (const auto & joint : limits.limits) {
        params[fmt::format("{}.{}.min_position", limits_prefix, joint.name)] =
          FormatYaml(joint.min_position);
        params[fmt::format("{}.{}.max_position", limits_prefix, joint.name)] =
          FormatYaml(joint.max_position);
        params[fmt::format("{}.{}.max_velocity", limits_prefix, joint.name)] =
          FormatYaml(joint.max_velocity);
        params[fmt::format("{}.{}.max_acceleration", limits_prefix, joint.name)] =
          FormatYaml(joint.max_acceleration);
      }

      std::ofstream ofs(runtime_path);
      ofs << runtime;
    } catch (const std::exception & e) {
      LOG_E << fmt::format(
        "ControllerAdapter: runtime YAML update failed for {}: {}",
        runtime_path, e.what());
      return false;
    }
  }

  return RestartNodes();
}

bool ControllerAdapter::UpdateHomePosition(const HomePositionConfiguration & home)
{
  if (home.joint_names.size() != home.positions.size()) {
    LOG_E << "ControllerAdapter: joint_names/positions size mismatch";
    return false;
  }

  const auto path = std::filesystem::path(
    ament_index_cpp::get_package_share_directory("pm_description")) /
    "config" / m_pm_type / "initial_positions.yaml";

  try {
    YAML::Node root = std::filesystem::exists(path)
      ? YAML::LoadFile(path.string())
      : YAML::Node(YAML::NodeType::Map);

    if (!root["initial_positions"] || !root["initial_positions"].IsMap()) {
      root["initial_positions"] = YAML::Node(YAML::NodeType::Map);
    }

    auto positions = root["initial_positions"];
    for (std::size_t i = 0; i < home.joint_names.size(); ++i) {
      positions[home.joint_names[i]] = FormatYaml(home.positions[i]);
    }

    std::ofstream ofs(path);
    ofs << root;
    return true;
  } catch (const std::exception & e) {
    LOG_E << fmt::format("ControllerAdapter: home position write failed: {}", e.what());
    return false;
  }
}

bool ControllerAdapter::RestartNodes(std::chrono::seconds timeout)
{
  for (const auto & name : kTargetNodes) {
    if (!KillAndWait(name, timeout)) {
      LOG_E << fmt::format("ControllerAdapter: node '{}' did not restart", name);
      return false;
    }
  }
  return true;
}

std::vector<std::string> ControllerAdapter::GetRuntimeParamsFiles() const
{
  std::vector<std::string> paths;
  std::array<char, 256> buffer{};

  for (const auto & node_name : kTargetNodes) {
    const auto cmd = fmt::format("ps -o cmd= -p $(pgrep -n {}) 2>/dev/null", node_name);
    FILE * pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
      continue;
    }

    std::string output;
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
      output += buffer.data();
    }
    pclose(pipe);

    if (output.empty()) {
      continue;
    }

    std::istringstream iss(output);
    std::vector<std::string> tokens(
      std::istream_iterator<std::string>{iss},
      std::istream_iterator<std::string>{});

    const auto it = std::find(tokens.begin(), tokens.end(), "--params-file");
    if (it != tokens.end() && std::next(it) != tokens.end()) {
      paths.push_back(*std::next(it));
    }
  }

  return paths;
}

std::optional<pid_t> ControllerAdapter::GetProcessPID(std::string_view name) const
{
  const auto cmd = fmt::format("pidof {}", name);
  FILE * pipe = popen(cmd.c_str(), "r");
  if (!pipe) {
    return std::nullopt;
  }

  char buffer[128]{};
  if (fgets(buffer, sizeof(buffer), pipe)) {
    pclose(pipe);
    try {
      return static_cast<pid_t>(std::stoi(buffer));
    } catch (...) {
      return std::nullopt;
    }
  }
  pclose(pipe);
  return std::nullopt;
}

bool ControllerAdapter::KillAndWait(
  std::string_view name, std::chrono::seconds timeout) const
{
  const auto old_pid = GetProcessPID(name);
  if (!old_pid.has_value()) {
    LOG_E << fmt::format("ControllerAdapter: process '{}' not found before kill", name);
    return false;
  }

  const auto cmd = fmt::format("pkill -f {}", name);
  if (std::system(cmd.c_str()) == -1) {
    LOG_E << fmt::format("ControllerAdapter: pkill failed for '{}'", name);
    return false;
  }

  // Опрашиваем процесс с интервалом 5% от таймаута.
  const auto poll = std::chrono::duration_cast<std::chrono::milliseconds>(timeout) / 20;
  const auto start = std::chrono::steady_clock::now();
  while (std::chrono::steady_clock::now() - start < timeout) {
    const auto new_pid = GetProcessPID(name);
    if (new_pid.has_value() && *new_pid != *old_pid) {
      return true;
    }
    std::this_thread::sleep_for(poll);
  }

  return false;
}

std::string ControllerAdapter::GetJointLimitsPath() const
{
  return (std::filesystem::path(
    ament_index_cpp::get_package_share_directory("pm_description")) /
    "config" / m_pm_type / "joint_limits.yaml").string();
}

}  // namespace pm::settings
