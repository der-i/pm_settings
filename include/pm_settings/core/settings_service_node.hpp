/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#ifndef PM_SETTINGS__CORE__SETTINGS_SERVICE_NODE_HPP_
#define PM_SETTINGS__CORE__SETTINGS_SERVICE_NODE_HPP_

#include <memory>

#include <rclcpp/rclcpp.hpp>
#include <std_srvs/srv/trigger.hpp>

#include <core_msgs/srv/add_object.hpp>
#include <core_msgs/srv/add_tcp.hpp>
#include <core_msgs/srv/add_transform.hpp>
#include <core_msgs/srv/add_work_frame.hpp>
#include <core_msgs/srv/apply_tcp.hpp>
#include <core_msgs/srv/apply_transform.hpp>
#include <core_msgs/srv/apply_work_frame.hpp>
#include <core_msgs/srv/delete_object.hpp>
#include <core_msgs/srv/delete_tcp.hpp>
#include <core_msgs/srv/delete_transform.hpp>
#include <core_msgs/srv/delete_work_frame.hpp>
#include <core_msgs/srv/get_object.hpp>
#include <core_msgs/srv/get_object_names.hpp>
#include <core_msgs/srv/get_work_frame.hpp>
#include <core_msgs/srv/list_work_frame.hpp>

#include "pm_settings/core/settings_manager.hpp"
#include "pm_settings/handlers/joint_limits_handler.hpp"
#include "pm_settings/handlers/mounting_handler.hpp"
#include "pm_settings/handlers/scene_handler.hpp"
#include "pm_settings/handlers/tcp_handler.hpp"

namespace pm::settings
{

/// @brief ROS2 узел, предоставляющий сервисы настройки манипулятора.
/// @details Слой интерфейсов архитектуры. Принимает запросы от внешних систем
/// (Web-интерфейс через ROSbridge, SDK через MQTT API), выполняет
/// десериализацию ROS2-сообщений во внутренние структуры модуля,
/// делегирует обработку SettingsManager и сериализует результаты обратно.
/// Не содержит бизнес-логики и не принимает решений о корректности параметров.
class SettingsServiceNode : public rclcpp::Node
{
public:
  explicit SettingsServiceNode(const rclcpp::NodeOptions & options = {});

  void Configure(
    std::shared_ptr<SettingsManager> settings_manager,
    std::shared_ptr<TCPHandler> tcp_handler,
    std::shared_ptr<MountingHandler> mounting_handler,
    std::shared_ptr<SceneHandler> scene_handler,
    std::shared_ptr<JointLimitsHandler> joint_limits_handler);

private:
  // ---- TCP services --------------------------------------------------------
  void HandleTcpAdd(
    const std::shared_ptr<core_msgs::srv::AddTCP::Request> request,
    std::shared_ptr<core_msgs::srv::AddTCP::Response> response);
  void HandleTcpApply(
    const std::shared_ptr<core_msgs::srv::ApplyTCP::Request> request,
    std::shared_ptr<core_msgs::srv::ApplyTCP::Response> response);
  void HandleTcpDelete(
    const std::shared_ptr<core_msgs::srv::DeleteTCP::Request> request,
    std::shared_ptr<core_msgs::srv::DeleteTCP::Response> response);
  void HandleTcpGetList(
    const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
    std::shared_ptr<std_srvs::srv::Trigger::Response> response);
  void HandleTcpGetCurrent(
    const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
    std::shared_ptr<std_srvs::srv::Trigger::Response> response);
  void HandleTcpReset(
    const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
    std::shared_ptr<std_srvs::srv::Trigger::Response> response);

  // ---- Base link services --------------------------------------------------
  void HandleBaseLinkAdd(
    const std::shared_ptr<core_msgs::srv::AddTransform::Request> request,
    std::shared_ptr<core_msgs::srv::AddTransform::Response> response);
  void HandleBaseLinkApply(
    const std::shared_ptr<core_msgs::srv::ApplyTransform::Request> request,
    std::shared_ptr<core_msgs::srv::ApplyTransform::Response> response);
  void HandleBaseLinkDelete(
    const std::shared_ptr<core_msgs::srv::DeleteTransform::Request> request,
    std::shared_ptr<core_msgs::srv::DeleteTransform::Response> response);
  void HandleBaseLinkGetList(
    const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
    std::shared_ptr<std_srvs::srv::Trigger::Response> response);
  void HandleBaseLinkGetCurrent(
    const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
    std::shared_ptr<std_srvs::srv::Trigger::Response> response);
  void HandleBaseLinkReset(
    const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
    std::shared_ptr<std_srvs::srv::Trigger::Response> response);

  // ---- Scene object services -----------------------------------------------
  void HandleObjectAdd(
    const std::shared_ptr<core_msgs::srv::AddObject::Request> request,
    std::shared_ptr<core_msgs::srv::AddObject::Response> response);
  void HandleObjectGet(
    const std::shared_ptr<core_msgs::srv::GetObject::Request> request,
    std::shared_ptr<core_msgs::srv::GetObject::Response> response);
  void HandleObjectDelete(
    const std::shared_ptr<core_msgs::srv::DeleteObject::Request> request,
    std::shared_ptr<core_msgs::srv::DeleteObject::Response> response);
  void HandleObjectList(
    const std::shared_ptr<core_msgs::srv::GetObjectNames::Request> request,
    std::shared_ptr<core_msgs::srv::GetObjectNames::Response> response);
  void HandleObjectUpdateScene(
    const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
    std::shared_ptr<std_srvs::srv::Trigger::Response> response);

  // ---- Work frame services -------------------------------------------------
  void HandleWorkFrameAdd(
    const std::shared_ptr<core_msgs::srv::AddWorkFrame::Request> request,
    std::shared_ptr<core_msgs::srv::AddWorkFrame::Response> response);
  void HandleWorkFrameApply(
    const std::shared_ptr<core_msgs::srv::ApplyWorkFrame::Request> request,
    std::shared_ptr<core_msgs::srv::ApplyWorkFrame::Response> response);
  void HandleWorkFrameDelete(
    const std::shared_ptr<core_msgs::srv::DeleteWorkFrame::Request> request,
    std::shared_ptr<core_msgs::srv::DeleteWorkFrame::Response> response);
  void HandleWorkFrameGetList(
    const std::shared_ptr<core_msgs::srv::ListWorkFrame::Request> request,
    std::shared_ptr<core_msgs::srv::ListWorkFrame::Response> response);
  void HandleWorkFrameGetCurrent(
    const std::shared_ptr<core_msgs::srv::GetWorkFrame::Request> request,
    std::shared_ptr<core_msgs::srv::GetWorkFrame::Response> response);
  void HandleWorkFrameReset(
    const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
    std::shared_ptr<std_srvs::srv::Trigger::Response> response);

  void CreateServices();

  std::shared_ptr<SettingsManager> m_manager;
  std::shared_ptr<TCPHandler> m_tcp_handler;
  std::shared_ptr<MountingHandler> m_mounting_handler;
  std::shared_ptr<SceneHandler> m_scene_handler;
  std::shared_ptr<JointLimitsHandler> m_joint_limits_handler;

  // ROS2 service handles. Хранятся как member-ы, чтобы не уничтожались
  // вместе со scope методов Configure/CreateServices.
  rclcpp::ServiceBase::SharedPtr m_tcp_add;
  rclcpp::ServiceBase::SharedPtr m_tcp_apply;
  rclcpp::ServiceBase::SharedPtr m_tcp_delete;
  rclcpp::ServiceBase::SharedPtr m_tcp_get_list;
  rclcpp::ServiceBase::SharedPtr m_tcp_get_current;
  rclcpp::ServiceBase::SharedPtr m_tcp_reset;

  rclcpp::ServiceBase::SharedPtr m_bl_add;
  rclcpp::ServiceBase::SharedPtr m_bl_apply;
  rclcpp::ServiceBase::SharedPtr m_bl_delete;
  rclcpp::ServiceBase::SharedPtr m_bl_get_list;
  rclcpp::ServiceBase::SharedPtr m_bl_get_current;
  rclcpp::ServiceBase::SharedPtr m_bl_reset;

  rclcpp::ServiceBase::SharedPtr m_obj_add;
  rclcpp::ServiceBase::SharedPtr m_obj_get;
  rclcpp::ServiceBase::SharedPtr m_obj_delete;
  rclcpp::ServiceBase::SharedPtr m_obj_list;
  rclcpp::ServiceBase::SharedPtr m_obj_update;

  rclcpp::ServiceBase::SharedPtr m_wf_add;
  rclcpp::ServiceBase::SharedPtr m_wf_apply;
  rclcpp::ServiceBase::SharedPtr m_wf_delete;
  rclcpp::ServiceBase::SharedPtr m_wf_get_list;
  rclcpp::ServiceBase::SharedPtr m_wf_get_current;
  rclcpp::ServiceBase::SharedPtr m_wf_reset;
};

}  // namespace pm::settings

#endif  // PM_SETTINGS__CORE__SETTINGS_SERVICE_NODE_HPP_
