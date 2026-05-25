/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#include "pm_settings/handlers/scene_handler.hpp"

#include <utility>

#include <boost/json.hpp>
#include <core_utils/log.h>
#include <fmt/format.h>

namespace pm::settings
{

namespace
{

namespace json = boost::json;

constexpr std::string_view kConfigFile = "objects.json";

std::string FormatDouble(double v)
{
  return fmt::format("{:.7f}", v);
}

json::object ObjectToJson(const SceneObject & obj)
{
  json::object o;
  o["name"]          = obj.name;
  o["parent_frame"]  = obj.parent_frame;
  o["type"]          = std::to_string(static_cast<std::uint8_t>(obj.type));
  o["position.x"]    = FormatDouble(obj.pose.position.x);
  o["position.y"]    = FormatDouble(obj.pose.position.y);
  o["position.z"]    = FormatDouble(obj.pose.position.z);
  o["orientation.x"] = FormatDouble(obj.pose.orientation.x);
  o["orientation.y"] = FormatDouble(obj.pose.orientation.y);
  o["orientation.z"] = FormatDouble(obj.pose.orientation.z);
  o["orientation.w"] = FormatDouble(obj.pose.orientation.w);
  o["dimension.x"]   = FormatDouble(obj.dimension.x);
  o["dimension.y"]   = FormatDouble(obj.dimension.y);
  o["dimension.z"]   = FormatDouble(obj.dimension.z);
  return o;
}

std::optional<SceneObject> ParseObject(const json::value & v)
{
  if (!v.is_object()) {
    return std::nullopt;
  }
  const auto & obj = v.as_object();
  if (!obj.contains("name")) {
    return std::nullopt;
  }
  SceneObject so;
  try {
    so.name = obj.at("name").as_string().c_str();
    if (obj.contains("parent_frame")) {
      so.parent_frame = obj.at("parent_frame").as_string().c_str();
    }
    if (obj.contains("type")) {
      so.type = static_cast<SceneObjectType>(
        std::stoi(std::string(obj.at("type").as_string())));
    }
    so.pose.position.x    = std::stod(std::string(obj.at("position.x").as_string()));
    so.pose.position.y    = std::stod(std::string(obj.at("position.y").as_string()));
    so.pose.position.z    = std::stod(std::string(obj.at("position.z").as_string()));
    so.pose.orientation.x = std::stod(std::string(obj.at("orientation.x").as_string()));
    so.pose.orientation.y = std::stod(std::string(obj.at("orientation.y").as_string()));
    so.pose.orientation.z = std::stod(std::string(obj.at("orientation.z").as_string()));
    so.pose.orientation.w = std::stod(std::string(obj.at("orientation.w").as_string()));
    so.dimension.x = std::stod(std::string(obj.at("dimension.x").as_string()));
    so.dimension.y = std::stod(std::string(obj.at("dimension.y").as_string()));
    so.dimension.z = std::stod(std::string(obj.at("dimension.z").as_string()));
  } catch (const std::exception & e) {
    LOG_W << fmt::format("SceneHandler: parse failed: {}", e.what());
    return std::nullopt;
  }
  return so;
}

}  // namespace

SceneHandler::SceneHandler(
  std::shared_ptr<MoveItAdapter> moveit_adapter,
  std::shared_ptr<ConfigFileManager> config_manager)
: m_moveit_adapter(std::move(moveit_adapter)),
  m_config_manager(std::move(config_manager))
{}

OperationResult SceneHandler::Apply(const std::any & value)
{
  const auto * obj = std::any_cast<SceneObject>(&value);
  if (!obj) {
    return OperationResult::Fail(
      ResultCode::INTERNAL_ERROR, "SceneHandler: payload type mismatch");
  }

  m_previous_state = m_objects;

  if (!m_moveit_adapter->AddSceneObject(*obj)) {
    m_previous_state.reset();
    return OperationResult::Fail(
      ResultCode::RUNTIME_UPDATE_FAILED,
      fmt::format("SceneHandler: failed to add '{}' to PlanningScene", obj->name));
  }

  m_objects.insert_or_assign(obj->name, *obj);
  return Persist();
}

OperationResult SceneHandler::Rollback()
{
  if (!m_previous_state.has_value()) {
    return OperationResult::Fail(
      ResultCode::ROLLBACK_FAILED, "SceneHandler: no previous state");
  }
  m_objects = *m_previous_state;
  m_previous_state.reset();
  std::vector<SceneObject> all;
  all.reserve(m_objects.size());
  for (const auto & [_, v] : m_objects) {
    all.push_back(v);
  }
  (void)m_moveit_adapter->PublishFullScene(all);
  return OperationResult::Ok("SceneHandler: rolled back");
}

std::any SceneHandler::GetCurrent() const
{
  return std::any{GetAll()};
}

bool SceneHandler::LoadFromDisk()
{
  const auto data = m_config_manager->Load(std::filesystem::path(kConfigFile));
  const auto * root = std::any_cast<json::object>(&data);
  if (!root || !root->contains("objects") || !root->at("objects").is_array()) {
    return false;
  }

  m_objects.clear();
  for (const auto & item : root->at("objects").as_array()) {
    if (auto obj = ParseObject(item); obj.has_value()) {
      m_objects.insert_or_assign(obj->name, *obj);
    }
  }

  std::vector<SceneObject> all = GetAll();
  (void)m_moveit_adapter->PublishFullScene(all);
  return true;
}

OperationResult SceneHandler::Remove(const std::string & name)
{
  const auto it = m_objects.find(name);
  if (it == m_objects.end()) {
    return OperationResult::Fail(
      ResultCode::INVALID_CONFIG,
      fmt::format("SceneHandler: '{}' not found", name));
  }
  m_objects.erase(it);

  if (!m_moveit_adapter->RemoveSceneObject(name)) {
    LOG_W << fmt::format("SceneHandler: removeSceneObject failed for '{}'", name);
  }
  return Persist();
}

std::vector<SceneObject> SceneHandler::GetAll() const
{
  std::vector<SceneObject> result;
  result.reserve(m_objects.size());
  for (const auto & [_, v] : m_objects) {
    result.push_back(v);
  }
  return result;
}

std::optional<SceneObject> SceneHandler::GetByName(const std::string & name) const
{
  const auto it = m_objects.find(name);
  if (it == m_objects.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::vector<std::string> SceneHandler::GetNames() const
{
  std::vector<std::string> names;
  names.reserve(m_objects.size());
  for (const auto & [k, _] : m_objects) {
    names.push_back(k);
  }
  return names;
}

OperationResult SceneHandler::ForceUpdateScene()
{
  std::vector<SceneObject> all = GetAll();
  if (!m_moveit_adapter->PublishFullScene(all)) {
    return OperationResult::Fail(
      ResultCode::RUNTIME_UPDATE_FAILED, "SceneHandler: PublishFullScene failed");
  }
  return OperationResult::Ok("Scene updated");
}

OperationResult SceneHandler::Persist()
{
  json::object root;
  json::array arr;
  for (const auto & [_, obj] : m_objects) {
    arr.push_back(ObjectToJson(obj));
  }
  root["objects"] = std::move(arr);

  if (!m_config_manager->Save(std::filesystem::path(kConfigFile), std::any{root})) {
    return OperationResult::Fail(
      ResultCode::INVALID_CONFIG, "SceneHandler: failed to write objects.json");
  }
  return OperationResult::Ok();
}

}  // namespace pm::settings
