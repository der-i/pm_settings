/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#include "pm_settings/handlers/joint_limits_handler.hpp"

#include <utility>

#include <core_utils/log.h>
#include <fmt/format.h>

namespace pm::settings
{

JointLimitsHandler::JointLimitsHandler(
  std::shared_ptr<ControllerAdapter> controller_adapter,
  std::shared_ptr<ConfigFileManager> config_manager)
: m_controller_adapter(std::move(controller_adapter)),
  m_config_manager(std::move(config_manager))
{}

OperationResult JointLimitsHandler::Apply(const std::any & value)
{
  const auto * cfg = std::any_cast<JointLimitsConfiguration>(&value);
  if (!cfg) {
    return OperationResult::Fail(
      ResultCode::INTERNAL_ERROR, "JointLimitsHandler: payload type mismatch");
  }

  m_previous_limits = m_current_limits;

  if (!m_controller_adapter->UpdateJointLimits(*cfg)) {
    // ControllerAdapter сам выполняет восстановление основного файла из бэкапа
    // через .bak; здесь снимаем сохранённый снепшот, поскольку runtime-состояние
    // не обновилось.
    m_previous_limits.reset();
    return OperationResult::Fail(
      ResultCode::RUNTIME_UPDATE_FAILED,
      "JointLimitsHandler: ControllerAdapter::UpdateJointLimits failed");
  }

  m_current_limits = *cfg;
  return OperationResult::Ok(
    fmt::format("Applied limits for {} joint(s)", cfg->limits.size()));
}

OperationResult JointLimitsHandler::Rollback()
{
  if (!m_previous_limits.has_value()) {
    return OperationResult::Fail(
      ResultCode::ROLLBACK_FAILED, "JointLimitsHandler: no previous state");
  }

  if (!m_controller_adapter->UpdateJointLimits(*m_previous_limits)) {
    return OperationResult::Fail(
      ResultCode::ROLLBACK_FAILED, "JointLimitsHandler: rollback failed");
  }

  m_current_limits = *m_previous_limits;
  m_previous_limits.reset();
  return OperationResult::Ok("JointLimitsHandler: rolled back");
}

std::any JointLimitsHandler::GetCurrent() const
{
  return std::any{m_current_limits};
}

bool JointLimitsHandler::LoadFromDisk()
{
  // Загрузка лимитов с диска выполняется ControllerAdapter при старте узла.
  // Здесь только инициализируем пустое состояние.
  return true;
}

OperationResult JointLimitsHandler::ApplyHomePosition(const HomePositionConfiguration & home)
{
  m_previous_home = m_current_home;

  if (!m_controller_adapter->UpdateHomePosition(home)) {
    m_previous_home.reset();
    return OperationResult::Fail(
      ResultCode::RUNTIME_UPDATE_FAILED,
      "JointLimitsHandler: home position update failed");
  }

  m_current_home = home;
  return OperationResult::Ok("Home position applied");
}

JointLimitsConfiguration JointLimitsHandler::GetCurrentLimits() const
{
  return m_current_limits;
}

HomePositionConfiguration JointLimitsHandler::GetCurrentHome() const
{
  return m_current_home;
}

}  // namespace pm::settings
