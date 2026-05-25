/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#include "pm_settings/handlers/tcp_handler.hpp"

#include <algorithm>
#include <utility>

#include <boost/json.hpp>
#include <core_utils/log.h>
#include <fmt/format.h>

namespace pm::settings
{

namespace
{

namespace json = boost::json;

constexpr std::string_view kConfigFile = "tcp_list.json";

std::string FormatDouble(double v)
{
  return fmt::format("{:.7f}", v);
}

json::object PoseToJson(const std::string & name, const geometry_msgs::msg::Pose & pose)
{
  json::object o;
  o["name"]          = name;
  o["position.x"]    = FormatDouble(pose.position.x);
  o["position.y"]    = FormatDouble(pose.position.y);
  o["position.z"]    = FormatDouble(pose.position.z);
  o["orientation.x"] = FormatDouble(pose.orientation.x);
  o["orientation.y"] = FormatDouble(pose.orientation.y);
  o["orientation.z"] = FormatDouble(pose.orientation.z);
  o["orientation.w"] = FormatDouble(pose.orientation.w);
  return o;
}

std::optional<TCPConfiguration> ParseConfig(const json::value & v)
{
  if (!v.is_object()) {
    return std::nullopt;
  }
  const auto & obj = v.as_object();
  if (!obj.contains("name")) {
    return std::nullopt;
  }

  TCPConfiguration cfg;
  try {
    cfg.name = obj.at("name").as_string().c_str();
    cfg.pose.position.x    = std::stod(std::string(obj.at("position.x").as_string()));
    cfg.pose.position.y    = std::stod(std::string(obj.at("position.y").as_string()));
    cfg.pose.position.z    = std::stod(std::string(obj.at("position.z").as_string()));
    cfg.pose.orientation.x = std::stod(std::string(obj.at("orientation.x").as_string()));
    cfg.pose.orientation.y = std::stod(std::string(obj.at("orientation.y").as_string()));
    cfg.pose.orientation.z = std::stod(std::string(obj.at("orientation.z").as_string()));
    cfg.pose.orientation.w = std::stod(std::string(obj.at("orientation.w").as_string()));
  } catch (const std::exception & e) {
    LOG_W << fmt::format("TCPHandler: failed to parse config: {}", e.what());
    return std::nullopt;
  }
  return cfg;
}

}  // namespace

TCPHandler::TCPHandler(
  std::shared_ptr<MoveItAdapter> moveit_adapter,
  std::shared_ptr<ConfigFileManager> config_manager)
: m_moveit_adapter(std::move(moveit_adapter)),
  m_config_manager(std::move(config_manager))
{
  m_current.name = "";
  m_current.pose.orientation.w = 1.0;
}

OperationResult TCPHandler::Apply(const std::any & value)
{
  const auto * cfg = std::any_cast<TCPConfiguration>(&value);
  if (!cfg) {
    return OperationResult::Fail(
      ResultCode::INTERNAL_ERROR, "TCPHandler: payload type mismatch");
  }

  // Сохраняем состояние для возможного отката.
  m_previous_state = m_current;

  // Применяем трансформацию через MoveItAdapter.
  const auto transform = ComputeTransform(cfg->pose);
  if (!m_moveit_adapter->UpdateTCPTransform(transform)) {
    m_previous_state.reset();
    return OperationResult::Fail(
      ResultCode::RUNTIME_UPDATE_FAILED,
      fmt::format("TCPHandler: failed to update MoveIt2 transform for '{}'", cfg->name));
  }

  // Upsert конфигурации.
  const auto it = std::find_if(m_configurations.begin(), m_configurations.end(),
    [&](const TCPConfiguration & c) { return c.name == cfg->name; });
  if (it == m_configurations.end()) {
    m_configurations.push_back(*cfg);
  } else {
    *it = *cfg;
  }
  m_current = *cfg;

  // Сохраняем в файл.
  json::object root;
  json::array tcps;
  for (const auto & c : m_configurations) {
    tcps.push_back(PoseToJson(c.name, c.pose));
  }
  root["tcps"] = std::move(tcps);

  if (!m_config_manager->Save(std::filesystem::path(kConfigFile), std::any{root})) {
    LOG_W << "TCPHandler: persisted runtime change but config write failed";
    return OperationResult{
      true, ResultCode::SUCCESS,
      "applied at runtime; config persistence failed"};
  }

  return OperationResult::Ok(fmt::format("TCP '{}' applied", cfg->name));
}

OperationResult TCPHandler::Rollback()
{
  if (!m_previous_state.has_value()) {
    return OperationResult::Fail(
      ResultCode::ROLLBACK_FAILED, "TCPHandler: no previous state stored");
  }

  const auto transform = ComputeTransform(m_previous_state->pose);
  if (!m_moveit_adapter->UpdateTCPTransform(transform)) {
    return OperationResult::Fail(
      ResultCode::ROLLBACK_FAILED, "TCPHandler: rollback transform failed");
  }
  m_current = *m_previous_state;
  m_previous_state.reset();
  return OperationResult::Ok("TCPHandler: rolled back");
}

std::any TCPHandler::GetCurrent() const
{
  return std::any{m_current};
}

bool TCPHandler::LoadFromDisk()
{
  const auto data = m_config_manager->Load(std::filesystem::path(kConfigFile));
  const auto * obj = std::any_cast<json::object>(&data);
  if (!obj || !obj->contains("tcps") || !obj->at("tcps").is_array()) {
    LOG_W << "TCPHandler: no tcp_list.json or invalid format";
    return false;
  }

  m_configurations.clear();
  for (const auto & item : obj->at("tcps").as_array()) {
    if (auto cfg = ParseConfig(item); cfg.has_value()) {
      m_configurations.push_back(std::move(*cfg));
    }
  }
  return true;
}

OperationResult TCPHandler::ApplyByName(const std::string & name)
{
  const auto it = std::find_if(m_configurations.begin(), m_configurations.end(),
    [&](const TCPConfiguration & c) { return c.name == name; });
  if (it == m_configurations.end()) {
    return OperationResult::Fail(
      ResultCode::INVALID_CONFIG,
      fmt::format("TCPHandler: '{}' not found in stored configurations", name));
  }
  return Apply(std::any{*it});
}

std::vector<TCPConfiguration> TCPHandler::GetList() const
{
  return m_configurations;
}

OperationResult TCPHandler::Delete(
  const std::string & name,
  bool reset_current,
  const std::string & apply_other)
{
  const auto it = std::find_if(m_configurations.begin(), m_configurations.end(),
    [&](const TCPConfiguration & c) { return c.name == name; });
  if (it == m_configurations.end()) {
    return OperationResult::Fail(
      ResultCode::INVALID_CONFIG,
      fmt::format("TCPHandler: '{}' not found", name));
  }

  m_configurations.erase(it);

  // Перезаписываем файл.
  json::object root;
  json::array tcps;
  for (const auto & c : m_configurations) {
    tcps.push_back(PoseToJson(c.name, c.pose));
  }
  root["tcps"] = std::move(tcps);
  (void)m_config_manager->Save(std::filesystem::path(kConfigFile), std::any{root});

  if (reset_current && m_current.name == name) {
    (void)Reset();
  }
  m_current.name.clear();

  if (!apply_other.empty()) {
    return ApplyByName(apply_other);
  }
  return OperationResult::Ok(fmt::format("TCP '{}' deleted", name));
}

OperationResult TCPHandler::Reset()
{
  TCPConfiguration defaults;
  defaults.name.clear();
  defaults.pose = {};
  defaults.pose.orientation.w = 1.0;

  const auto transform = ComputeTransform(defaults.pose);
  (void)m_moveit_adapter->UpdateTCPTransform(transform);
  m_current = defaults;
  m_previous_state.reset();
  return OperationResult::Ok("TCPHandler: reset to identity");
}

geometry_msgs::msg::TransformStamped TCPHandler::ComputeTransform(
  const geometry_msgs::msg::Pose & pose) const
{
  geometry_msgs::msg::TransformStamped t;
  t.header.frame_id = std::string(frames::FLANGE);
  t.child_frame_id  = std::string(frames::TOOL);
  t.transform.translation.x = pose.position.x;
  t.transform.translation.y = pose.position.y;
  t.transform.translation.z = pose.position.z;
  t.transform.rotation = pose.orientation;
  return t;
}

}  // namespace pm::settings
