/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#ifndef PM_SETTINGS__ADAPTERS__CONTROLLER_ADAPTER_HPP_
#define PM_SETTINGS__ADAPTERS__CONTROLLER_ADAPTER_HPP_

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <rclcpp/rclcpp.hpp>
#include <sys/types.h>

#include "pm_settings/types/configurations.hpp"

namespace pm::settings
{

/// @brief Адаптер для взаимодействия с контроллерами ros2_control и узлами MoveIt2.
/// @details Обновляет параметры лимитов суставов в YAML-файлах, используемых
/// запущенными узлами move_group и servo_node (через анализ аргумента
/// --params-file в командной строке процесса), и выполняет их перезапуск.
class ControllerAdapter
{
public:
  explicit ControllerAdapter(const std::shared_ptr<rclcpp::Node> & node);

  /// @brief Обновить лимиты суставов в MoveIt2 и ros2_control.
  /// @details Выполняет запись в основной joint_limits.yaml, анализ
  /// --params-file для move_group/servo_node, обновление runtime-файлов,
  /// перезапуск узлов с ожиданием до 15 секунд.
  [[nodiscard]] bool UpdateJointLimits(const JointLimitsConfiguration & limits);

  /// @brief Обновить домашнее положение (initial_positions.yaml).
  [[nodiscard]] bool UpdateHomePosition(const HomePositionConfiguration & home);

  /// @brief Перезапустить узлы (move_group, servo_node) после изменения
  /// параметров, требующих re-read конфигурации.
  [[nodiscard]] bool RestartNodes(
    std::chrono::seconds timeout = std::chrono::seconds{15});

private:
  /// @brief Получить список runtime params-файлов для узлов move_group/servo_node
  /// через анализ аргумента --params-file в командной строке процесса.
  [[nodiscard]] std::vector<std::string> GetRuntimeParamsFiles() const;

  /// @brief Получить PID процесса по имени.
  [[nodiscard]] std::optional<pid_t> GetProcessPID(std::string_view name) const;

  /// @brief Убить процесс и дождаться его повторного запуска.
  [[nodiscard]] bool KillAndWait(std::string_view name, std::chrono::seconds timeout) const;

  [[nodiscard]] std::string GetJointLimitsPath() const;

  std::shared_ptr<rclcpp::Node> m_node;
  std::string m_pm_type;
};

}  // namespace pm::settings

#endif  // PM_SETTINGS__ADAPTERS__CONTROLLER_ADAPTER_HPP_
