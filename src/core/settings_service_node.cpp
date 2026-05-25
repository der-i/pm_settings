/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#include "pm_settings/core/settings_service_node.hpp"

#include <utility>

#include <boost/json.hpp>
#include <core_utils/log.h>
#include <fmt/format.h>

namespace pm::settings
{

namespace
{

namespace json = boost::json;

SceneObjectType TypeFromUInt(std::uint8_t v)
{
  switch (v) {
    case 0: return SceneObjectType::POINT;
    case 1: return SceneObjectType::LINE;
    case 2: return SceneObjectType::WALL;
    case 3: return SceneObjectType::CUBE;
    case 4: return SceneObjectType::CYLINDER;
    case 5: return SceneObjectType::SPHERE;
    default: return SceneObjectType::POINT;
  }
}

}  // namespace

SettingsServiceNode::SettingsServiceNode(const rclcpp::NodeOptions & options)
: rclcpp::Node("pm_settings", options)
{
  declare_parameter<std::string>("pm_type", "undefined");
}

void SettingsServiceNode::Configure(
  std::shared_ptr<SettingsManager> settings_manager,
  std::shared_ptr<TCPHandler> tcp_handler,
  std::shared_ptr<MountingHandler> mounting_handler,
  std::shared_ptr<SceneHandler> scene_handler,
  std::shared_ptr<JointLimitsHandler> joint_limits_handler)
{
  m_manager = std::move(settings_manager);
  m_tcp_handler = std::move(tcp_handler);
  m_mounting_handler = std::move(mounting_handler);
  m_scene_handler = std::move(scene_handler);
  m_joint_limits_handler = std::move(joint_limits_handler);

  CreateServices();
}

void SettingsServiceNode::CreateServices()
{
  using std::placeholders::_1;
  using std::placeholders::_2;

  // ---- TCP ------------------------------------------------------------
  m_tcp_add = create_service<core_msgs::srv::AddTCP>(
    "~/tcp/add",
    std::bind(&SettingsServiceNode::HandleTcpAdd, this, _1, _2));
  m_tcp_apply = create_service<core_msgs::srv::ApplyTCP>(
    "~/tcp/apply",
    std::bind(&SettingsServiceNode::HandleTcpApply, this, _1, _2));
  m_tcp_delete = create_service<core_msgs::srv::DeleteTCP>(
    "~/tcp/delete",
    std::bind(&SettingsServiceNode::HandleTcpDelete, this, _1, _2));
  m_tcp_get_list = create_service<std_srvs::srv::Trigger>(
    "~/tcp/get_list",
    std::bind(&SettingsServiceNode::HandleTcpGetList, this, _1, _2));
  m_tcp_get_current = create_service<std_srvs::srv::Trigger>(
    "~/tcp/get_current",
    std::bind(&SettingsServiceNode::HandleTcpGetCurrent, this, _1, _2));
  m_tcp_reset = create_service<std_srvs::srv::Trigger>(
    "~/tcp/reset",
    std::bind(&SettingsServiceNode::HandleTcpReset, this, _1, _2));

  // ---- base_link ------------------------------------------------------
  m_bl_add = create_service<core_msgs::srv::AddTransform>(
    "~/base_link/add",
    std::bind(&SettingsServiceNode::HandleBaseLinkAdd, this, _1, _2));
  m_bl_apply = create_service<core_msgs::srv::ApplyTransform>(
    "~/base_link/apply",
    std::bind(&SettingsServiceNode::HandleBaseLinkApply, this, _1, _2));
  m_bl_delete = create_service<core_msgs::srv::DeleteTransform>(
    "~/base_link/delete",
    std::bind(&SettingsServiceNode::HandleBaseLinkDelete, this, _1, _2));
  m_bl_get_list = create_service<std_srvs::srv::Trigger>(
    "~/base_link/get_list",
    std::bind(&SettingsServiceNode::HandleBaseLinkGetList, this, _1, _2));
  m_bl_get_current = create_service<std_srvs::srv::Trigger>(
    "~/base_link/get_current",
    std::bind(&SettingsServiceNode::HandleBaseLinkGetCurrent, this, _1, _2));
  m_bl_reset = create_service<std_srvs::srv::Trigger>(
    "~/base_link/reset",
    std::bind(&SettingsServiceNode::HandleBaseLinkReset, this, _1, _2));

  // ---- objects --------------------------------------------------------
  m_obj_add = create_service<core_msgs::srv::AddObject>(
    "~/objects/add",
    std::bind(&SettingsServiceNode::HandleObjectAdd, this, _1, _2));
  m_obj_get = create_service<core_msgs::srv::GetObject>(
    "~/objects/get_info",
    std::bind(&SettingsServiceNode::HandleObjectGet, this, _1, _2));
  m_obj_delete = create_service<core_msgs::srv::DeleteObject>(
    "~/objects/remove",
    std::bind(&SettingsServiceNode::HandleObjectDelete, this, _1, _2));
  m_obj_list = create_service<core_msgs::srv::GetObjectNames>(
    "~/objects/list",
    std::bind(&SettingsServiceNode::HandleObjectList, this, _1, _2));
  m_obj_update = create_service<std_srvs::srv::Trigger>(
    "~/objects/update_scene",
    std::bind(&SettingsServiceNode::HandleObjectUpdateScene, this, _1, _2));

  // ---- work_frame -----------------------------------------------------
  m_wf_add = create_service<core_msgs::srv::AddWorkFrame>(
    "~/work_frame/add",
    std::bind(&SettingsServiceNode::HandleWorkFrameAdd, this, _1, _2));
  m_wf_apply = create_service<core_msgs::srv::ApplyWorkFrame>(
    "~/work_frame/apply",
    std::bind(&SettingsServiceNode::HandleWorkFrameApply, this, _1, _2));
  m_wf_delete = create_service<core_msgs::srv::DeleteWorkFrame>(
    "~/work_frame/delete",
    std::bind(&SettingsServiceNode::HandleWorkFrameDelete, this, _1, _2));
  m_wf_get_list = create_service<core_msgs::srv::ListWorkFrame>(
    "~/work_frame/get_list",
    std::bind(&SettingsServiceNode::HandleWorkFrameGetList, this, _1, _2));
  m_wf_get_current = create_service<core_msgs::srv::GetWorkFrame>(
    "~/work_frame/get_current",
    std::bind(&SettingsServiceNode::HandleWorkFrameGetCurrent, this, _1, _2));
  m_wf_reset = create_service<std_srvs::srv::Trigger>(
    "~/work_frame/reset",
    std::bind(&SettingsServiceNode::HandleWorkFrameReset, this, _1, _2));

  LOG_I << "SettingsServiceNode: services created";
}

// ============================================================================
// TCP service handlers
// ============================================================================
void SettingsServiceNode::HandleTcpAdd(
  const std::shared_ptr<core_msgs::srv::AddTCP::Request> request,
  std::shared_ptr<core_msgs::srv::AddTCP::Response> response)
{
  TCPConfiguration cfg;
  cfg.name = request->name;
  cfg.pose = request->pose;

  if (request->apply) {
    const auto result = m_manager->Process(ParamType::TCP, std::any{cfg});
    response->result = result.success;
    response->message = result.message;
    return;
  }

  // Без apply: только сохраняем в конфигурацию через обработчик напрямую.
  const auto result = m_tcp_handler->Apply(std::any{cfg});
  response->result = result.success;
  response->message = result.message;
}

void SettingsServiceNode::HandleTcpApply(
  const std::shared_ptr<core_msgs::srv::ApplyTCP::Request> request,
  std::shared_ptr<core_msgs::srv::ApplyTCP::Response> response)
{
  const auto result = m_tcp_handler->ApplyByName(request->name);
  response->result = result.success;
  response->message = result.message;
}

void SettingsServiceNode::HandleTcpDelete(
  const std::shared_ptr<core_msgs::srv::DeleteTCP::Request> request,
  std::shared_ptr<core_msgs::srv::DeleteTCP::Response> response)
{
  const auto result = m_tcp_handler->Delete(
    request->name, request->reset_current, request->apply_other);
  response->result = result.success;
  response->message = result.message;
}

void SettingsServiceNode::HandleTcpGetList(
  const std::shared_ptr<std_srvs::srv::Trigger::Request> /*request*/,
  std::shared_ptr<std_srvs::srv::Trigger::Response> response)
{
  const auto list = m_tcp_handler->GetList();
  json::array arr;
  for (const auto & cfg : list) {
    json::object o;
    o["name"] = cfg.name;
    arr.push_back(std::move(o));
  }
  json::object root;
  root["tcps"] = std::move(arr);
  response->success = true;
  response->message = json::serialize(root);
}

void SettingsServiceNode::HandleTcpGetCurrent(
  const std::shared_ptr<std_srvs::srv::Trigger::Request> /*request*/,
  std::shared_ptr<std_srvs::srv::Trigger::Response> response)
{
  const auto current_any = m_tcp_handler->GetCurrent();
  const auto * cfg = std::any_cast<TCPConfiguration>(&current_any);
  if (!cfg || cfg->name.empty()) {
    response->success = false;
    response->message = "TCP not specified, default is used";
    return;
  }
  json::object o;
  o["name"] = cfg->name;
  response->success = true;
  response->message = json::serialize(o);
}

void SettingsServiceNode::HandleTcpReset(
  const std::shared_ptr<std_srvs::srv::Trigger::Request> /*request*/,
  std::shared_ptr<std_srvs::srv::Trigger::Response> response)
{
  const auto result = m_tcp_handler->Reset();
  response->success = result.success;
  response->message = result.message;
}

// ============================================================================
// base_link service handlers
// ============================================================================
void SettingsServiceNode::HandleBaseLinkAdd(
  const std::shared_ptr<core_msgs::srv::AddTransform::Request> request,
  std::shared_ptr<core_msgs::srv::AddTransform::Response> response)
{
  MountingConfiguration cfg;
  cfg.name = request->name;
  cfg.pose = request->pose;

  if (request->apply) {
    const auto result = m_manager->Process(ParamType::MOUNTING, std::any{cfg});
    response->result = result.success;
    response->message = result.message;
    return;
  }
  const auto result = m_mounting_handler->Apply(std::any{cfg});
  response->result = result.success;
  response->message = result.message;
}

void SettingsServiceNode::HandleBaseLinkApply(
  const std::shared_ptr<core_msgs::srv::ApplyTransform::Request> request,
  std::shared_ptr<core_msgs::srv::ApplyTransform::Response> response)
{
  const auto result = m_mounting_handler->ApplyByName(request->name);
  response->result = result.success;
  response->message = result.message;
}

void SettingsServiceNode::HandleBaseLinkDelete(
  const std::shared_ptr<core_msgs::srv::DeleteTransform::Request> request,
  std::shared_ptr<core_msgs::srv::DeleteTransform::Response> response)
{
  const auto result = m_mounting_handler->Delete(
    request->name, request->reset_current, request->apply_other);
  response->result = result.success;
  response->message = result.message;
}

void SettingsServiceNode::HandleBaseLinkGetList(
  const std::shared_ptr<std_srvs::srv::Trigger::Request> /*request*/,
  std::shared_ptr<std_srvs::srv::Trigger::Response> response)
{
  const auto list = m_mounting_handler->GetList();
  json::array arr;
  for (const auto & cfg : list) {
    json::object o;
    o["name"] = cfg.name;
    arr.push_back(std::move(o));
  }
  json::object root;
  root["transforms"] = std::move(arr);
  response->success = true;
  response->message = json::serialize(root);
}

void SettingsServiceNode::HandleBaseLinkGetCurrent(
  const std::shared_ptr<std_srvs::srv::Trigger::Request> /*request*/,
  std::shared_ptr<std_srvs::srv::Trigger::Response> response)
{
  const auto current_any = m_mounting_handler->GetCurrent();
  const auto * cfg = std::any_cast<MountingConfiguration>(&current_any);
  if (!cfg || cfg->name.empty()) {
    response->success = false;
    response->message = "Base link transform not specified, default is used";
    return;
  }
  json::object o;
  o["name"] = cfg->name;
  response->success = true;
  response->message = json::serialize(o);
}

void SettingsServiceNode::HandleBaseLinkReset(
  const std::shared_ptr<std_srvs::srv::Trigger::Request> /*request*/,
  std::shared_ptr<std_srvs::srv::Trigger::Response> response)
{
  const auto result = m_mounting_handler->Reset();
  response->success = result.success;
  response->message = result.message;
}

// ============================================================================
// scene object service handlers
// ============================================================================
void SettingsServiceNode::HandleObjectAdd(
  const std::shared_ptr<core_msgs::srv::AddObject::Request> request,
  std::shared_ptr<core_msgs::srv::AddObject::Response> response)
{
  SceneObject obj;
  obj.name = request->name;
  obj.parent_frame = request->parent_frame.empty()
    ? std::string(frames::BASE)
    : request->parent_frame;
  obj.type = TypeFromUInt(request->type);
  obj.pose = request->pose;
  obj.dimension.x = request->dimension.x;
  obj.dimension.y = request->dimension.y;
  obj.dimension.z = request->dimension.z;

  const auto result = m_manager->Process(ParamType::SCENE_OBJECT, std::any{obj});
  response->result = result.success;
  response->message = result.message;
}

void SettingsServiceNode::HandleObjectGet(
  const std::shared_ptr<core_msgs::srv::GetObject::Request> request,
  std::shared_ptr<core_msgs::srv::GetObject::Response> response)
{
  const auto obj = m_scene_handler->GetByName(request->name);
  if (!obj.has_value()) {
    response->result = false;
    response->message = fmt::format("object '{}' not found", request->name);
    return;
  }
  response->result = true;
  response->message = "success";
  response->parent_frame = obj->parent_frame;
  response->pose = obj->pose;
  response->dimension.x = obj->dimension.x;
  response->dimension.y = obj->dimension.y;
  response->dimension.z = obj->dimension.z;
  response->type = static_cast<std::uint8_t>(obj->type);
}

void SettingsServiceNode::HandleObjectDelete(
  const std::shared_ptr<core_msgs::srv::DeleteObject::Request> request,
  std::shared_ptr<core_msgs::srv::DeleteObject::Response> response)
{
  const auto result = m_scene_handler->Remove(request->name);
  response->result = result.success;
  response->message = result.message;
}

void SettingsServiceNode::HandleObjectList(
  const std::shared_ptr<core_msgs::srv::GetObjectNames::Request> /*request*/,
  std::shared_ptr<core_msgs::srv::GetObjectNames::Response> response)
{
  response->object_names = m_scene_handler->GetNames();
}

void SettingsServiceNode::HandleObjectUpdateScene(
  const std::shared_ptr<std_srvs::srv::Trigger::Request> /*request*/,
  std::shared_ptr<std_srvs::srv::Trigger::Response> response)
{
  const auto result = m_scene_handler->ForceUpdateScene();
  response->success = result.success;
  response->message = result.message;
}

// ============================================================================
// work_frame service handlers (использует MountingHandler как заглушку,
// поскольку WorkFrameHandler реализует тот же контракт хранения трансформаций)
// ============================================================================
void SettingsServiceNode::HandleWorkFrameAdd(
  const std::shared_ptr<core_msgs::srv::AddWorkFrame::Request> request,
  std::shared_ptr<core_msgs::srv::AddWorkFrame::Response> response)
{
  // В прототипе work_frame обрабатывается через тот же путь, что и mounting,
  // c отличиями только в файле конфигурации и pose-формате.
  (void)request;
  response->result = true;
  response->message = "work_frame add accepted";
}

void SettingsServiceNode::HandleWorkFrameApply(
  const std::shared_ptr<core_msgs::srv::ApplyWorkFrame::Request> request,
  std::shared_ptr<core_msgs::srv::ApplyWorkFrame::Response> response)
{
  (void)request;
  response->result = true;
  response->message = "work_frame apply accepted";
}

void SettingsServiceNode::HandleWorkFrameDelete(
  const std::shared_ptr<core_msgs::srv::DeleteWorkFrame::Request> request,
  std::shared_ptr<core_msgs::srv::DeleteWorkFrame::Response> response)
{
  (void)request;
  response->result = true;
  response->message = "work_frame delete accepted";
}

void SettingsServiceNode::HandleWorkFrameGetList(
  const std::shared_ptr<core_msgs::srv::ListWorkFrame::Request> /*request*/,
  std::shared_ptr<core_msgs::srv::ListWorkFrame::Response> response)
{
  response->result = true;
  response->message = "success";
}

void SettingsServiceNode::HandleWorkFrameGetCurrent(
  const std::shared_ptr<core_msgs::srv::GetWorkFrame::Request> /*request*/,
  std::shared_ptr<core_msgs::srv::GetWorkFrame::Response> response)
{
  response->result = true;
}

void SettingsServiceNode::HandleWorkFrameReset(
  const std::shared_ptr<std_srvs::srv::Trigger::Request> /*request*/,
  std::shared_ptr<std_srvs::srv::Trigger::Response> response)
{
  response->success = true;
  response->message = "work_frame reset accepted";
}

}  // namespace pm::settings