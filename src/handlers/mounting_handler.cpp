/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#include "pm_settings/handlers/mounting_handler.hpp"

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

constexpr std::string_view kConfigFile = "base_link_transforms.json";
constexpr std::string_view kLastUsedKey = "last_used";
constexpr std::string_view kTransformsKey = "transforms";

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

std::optional<MountingConfiguration> ParseConfig(const json::value & v)
{
  if (!v.is_object()) {
    return std::nullopt;
  }
  const auto & obj = v.as_object();
  if (!obj.contains("name")) {
    return std::nullopt;
  }
  MountingConfiguration cfg;
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
    LOG_W << fmt::format("MountingHandler: failed to parse config: {}", e.what());
    return std::nullopt;
  }
  return cfg;
}

}  // namespace

MountingHandler::MountingHandler(
  std::shared_ptr<MoveItAdapter> moveit_adapter,
  std::shared_ptr<ConfigFileManager> config_manager)
: m_moveit_adapter(std::move(moveit_adapter)),
  m_config_manager(std::move(config_manager))
{
  m_current.name.clear();
  m_current.pose.orientation.w = 1.0;
}

OperationResult MountingHandler::Apply(const std::any & value)
{
  const auto * cfg = std::any_cast<MountingConfiguration>(&value);
  if (!cfg) {
    return OperationResult::Fail(
      ResultCode::INTERNAL_ERROR, "MountingHandler: payload type mismatch");
  }

  m_previous_state = m_current;

  const auto transform = ComputeTransform(cfg->pose);
  if (!m_moveit_adapter->UpdateBaseLinkTransform(transform)) {
    m_previous_state.reset();
    return OperationResult::Fail(
      ResultCode::RUNTIME_UPDATE_FAILED,
      fmt::format("MountingHandler: failed to update transform for '{}'", cfg->name));
  }

  // Upsert.
  const auto it = std::find_if(m_configurations.begin(), m_configurations.end(),
    [&](const MountingConfiguration & c) { return c.name == cfg->name; });
  if (it == m_configurations.end()) {
    m_configurations.push_back(*cfg);
  } else {
    *it = *cfg;
  }
  m_current = *cfg;

  if (!PersistLastUsed(cfg->name)) {
    LOG_W << "MountingHandler: failed to persist last_used";
  }

  return OperationResult::Ok(fmt::format("Mounting '{}' applied", cfg->name));
}

OperationResult MountingHandler::Rollback()
{
  if (!m_previous_state.has_value()) {
    return OperationResult::Fail(
      ResultCode::ROLLBACK_FAILED, "MountingHandler: no previous state");
  }
  const auto transform = ComputeTransform(m_previous_state->pose);
  if (!m_moveit_adapter->UpdateBaseLinkTransform(transform)) {
    return OperationResult::Fail(
      ResultCode::ROLLBACK_FAILED, "MountingHandler: rollback transform failed");
  }
  m_current = *m_previous_state;
  m_previous_state.reset();
  return OperationResult::Ok("MountingHandler: rolled back");
}

std::any MountingHandler::GetCurrent() const
{
  return std::any{m_current};
}

bool MountingHandler::LoadFromDisk()
{
  const auto data = m_config_manager->Load(std::filesystem::path(kConfigFile));
  const auto * obj = std::any_cast<json::object>(&data);
  if (!obj) {
    return false;
  }

  if (obj->contains(kTransformsKey) && obj->at(kTransformsKey).is_array()) {
    m_configurations.clear();
    for (const auto & item : obj->at(kTransformsKey).as_array()) {
      if (auto cfg = ParseConfig(item); cfg.has_value()) {
        m_configurations.push_back(std::move(*cfg));
      }
    }
  }

  if (obj->contains(kLastUsedKey) && obj->at(kLastUsedKey).is_string()) {
    const std::string last_used(obj->at(kLastUsedKey).as_string().c_str());
    if (!last_used.empty()) {
      const auto result = ApplyByName(last_used);
      if (!result.success) {
        LOG_W << fmt::format(
          "MountingHandler: failed to restore last_used '{}': {}",
          last_used, result.message);
      }
    }
  }
  return true;
}

OperationResult MountingHandler::ApplyByName(const std::string & name)
{
  const auto it = std::find_if(m_configurations.begin(), m_configurations.end(),
    [&](const MountingConfiguration & c) { return c.name == name; });
  if (it == m_configurations.end()) {
    return OperationResult::Fail(
      ResultCode::INVALID_CONFIG,
      fmt::format("MountingHandler: '{}' not found", name));
  }
  return Apply(std::any{*it});
}

std::vector<MountingConfiguration> MountingHandler::GetList() const
{
  return m_configurations;
}

OperationResult MountingHandler::Delete(
  const std::string & name,
  bool reset_current,
  const std::string & apply_other)
{
  const auto it = std::find_if(m_configurations.begin(), m_configurations.end(),
    [&](const MountingConfiguration & c) { return c.name == name; });
  if (it == m_configurations.end()) {
    return OperationResult::Fail(
      ResultCode::INVALID_CONFIG,
      fmt::format("MountingHandler: '{}' not found", name));
  }
  m_configurations.erase(it);

  json::object root;
  json::array transforms;
  for (const auto & c : m_configurations) {
    transforms.push_back(PoseToJson(c.name, c.pose));
  }
  root[kTransformsKey] = std::move(transforms);
  root[kLastUsedKey] = "";
  (void)m_config_manager->Save(std::filesystem::path(kConfigFile), std::any{root});

  if (reset_current && m_current.name == name) {
    (void)Reset();
  }
  m_current.name.clear();

  if (!apply_other.empty()) {
    return ApplyByName(apply_other);
  }
  return OperationResult::Ok(fmt::format("Mounting '{}' deleted", name));
}

OperationResult MountingHandler::Reset()
{
  MountingConfiguration defaults;
  defaults.pose.orientation.w = 1.0;
  const auto transform = ComputeTransform(defaults.pose);
  (void)m_moveit_adapter->UpdateBaseLinkTransform(transform);
  m_current = defaults;
  m_previous_state.reset();
  (void)PersistLastUsed("");
  return OperationResult::Ok("MountingHandler: reset to identity");
}

geometry_msgs::msg::TransformStamped MountingHandler::ComputeTransform(
  const geometry_msgs::msg::Pose & pose) const
{
  geometry_msgs::msg::TransformStamped t;
  t.header.frame_id = std::string(frames::WORLD);
  t.child_frame_id  = std::string(frames::BASE);
  t.transform.translation.x = pose.position.x;
  t.transform.translation.y = pose.position.y;
  t.transform.translation.z = pose.position.z;
  t.transform.rotation = pose.orientation;
  return t;
}

bool MountingHandler::PersistLastUsed(const std::string & name)
{
  auto data = m_config_manager->Load(std::filesystem::path(kConfigFile));
  auto * obj = std::any_cast<json::object>(&data);

  json::object root;
  if (obj) {
    root = *obj;
  }
  root[kLastUsedKey] = name;

  if (!root.contains(kTransformsKey)) {
    json::array transforms;
    for (const auto & c : m_configurations) {
      transforms.push_back(PoseToJson(c.name, c.pose));
    }
    root[kTransformsKey] = std::move(transforms);
  }

  return m_config_manager->Save(std::filesystem::path(kConfigFile), std::any{root});
}

}  // namespace pm::settings
