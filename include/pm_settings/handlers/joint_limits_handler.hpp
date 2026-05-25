/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#ifndef PM_SETTINGS__HANDLERS__JOINT_LIMITS_HANDLER_HPP_
#define PM_SETTINGS__HANDLERS__JOINT_LIMITS_HANDLER_HPP_

#include <memory>
#include <optional>

#include "pm_settings/adapters/controller_adapter.hpp"
#include "pm_settings/interfaces/i_settings_handler.hpp"
#include "pm_settings/persistence/config_file_manager.hpp"
#include "pm_settings/types/configurations.hpp"

namespace pm::settings
{

/// @brief Обработчик пределов хода суставов и домашнего положения.
/// @details Координирует обновление параметров одновременно в MoveIt2 и
/// ros2_control: запись в joint_limits.yaml, обновление runtime-файлов
/// узлов move_group/servo_node, перезапуск узлов через ControllerAdapter.
class JointLimitsHandler final : public ISettingsHandler
{
public:
  JointLimitsHandler(
    std::shared_ptr<ControllerAdapter> controller_adapter,
    std::shared_ptr<ConfigFileManager> config_manager);

  [[nodiscard]] OperationResult Apply(const std::any & value) override;
  [[nodiscard]] OperationResult Rollback() override;
  [[nodiscard]] std::any GetCurrent() const override;
  [[nodiscard]] ParamType GetType() const noexcept override { return ParamType::JOINT_LIMITS; }

  [[nodiscard]] bool LoadFromDisk();

  /// @brief Применить домашнее положение.
  [[nodiscard]] OperationResult ApplyHomePosition(const HomePositionConfiguration & home);

  [[nodiscard]] JointLimitsConfiguration GetCurrentLimits() const;
  [[nodiscard]] HomePositionConfiguration GetCurrentHome() const;

private:
  std::shared_ptr<ControllerAdapter> m_controller_adapter;
  std::shared_ptr<ConfigFileManager> m_config_manager;

  JointLimitsConfiguration m_current_limits;
  HomePositionConfiguration m_current_home;

  std::optional<JointLimitsConfiguration> m_previous_limits;
  std::optional<HomePositionConfiguration> m_previous_home;
};

}  // namespace pm::settings

#endif  // PM_SETTINGS__HANDLERS__JOINT_LIMITS_HANDLER_HPP_
