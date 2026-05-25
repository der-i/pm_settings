/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#ifndef PM_SETTINGS__HANDLERS__TCP_HANDLER_HPP_
#define PM_SETTINGS__HANDLERS__TCP_HANDLER_HPP_

#include <memory>
#include <optional>

#include "pm_settings/adapters/moveit_adapter.hpp"
#include "pm_settings/interfaces/i_settings_handler.hpp"
#include "pm_settings/persistence/config_file_manager.hpp"
#include "pm_settings/types/configurations.hpp"

namespace pm::settings
{

/// @brief Обработчик центральной точки инструмента.
/// @details Инкапсулирует логику применения конфигурации ЦТИ: вычисление
/// трансформации flange -> tool, обновление кинематической модели через
/// MoveItAdapter, сохранение в tcp_list.json через ConfigFileManager.
class TCPHandler final : public ISettingsHandler
{
public:
  TCPHandler(
    std::shared_ptr<MoveItAdapter> moveit_adapter,
    std::shared_ptr<ConfigFileManager> config_manager);

  [[nodiscard]] OperationResult Apply(const std::any & value) override;
  [[nodiscard]] OperationResult Rollback() override;
  [[nodiscard]] std::any GetCurrent() const override;
  [[nodiscard]] ParamType GetType() const noexcept override { return ParamType::TCP; }

  /// @brief Загрузить последнюю применённую конфигурацию при старте.
  [[nodiscard]] bool LoadFromDisk();

  /// @brief Применить конфигурацию по имени (используется сервисом apply).
  [[nodiscard]] OperationResult ApplyByName(const std::string & name);

  /// @brief Получить список всех сохранённых конфигураций.
  [[nodiscard]] std::vector<TCPConfiguration> GetList() const;

  /// @brief Удалить конфигурацию по имени.
  [[nodiscard]] OperationResult Delete(
    const std::string & name,
    bool reset_current,
    const std::string & apply_other);

  /// @brief Сбросить ЦТИ к значениям по умолчанию.
  [[nodiscard]] OperationResult Reset();

private:
  [[nodiscard]] geometry_msgs::msg::TransformStamped ComputeTransform(
    const geometry_msgs::msg::Pose & pose) const;

  std::shared_ptr<MoveItAdapter> m_moveit_adapter;
  std::shared_ptr<ConfigFileManager> m_config_manager;

  std::vector<TCPConfiguration> m_configurations;
  TCPConfiguration m_current;
  std::optional<TCPConfiguration> m_previous_state;
};

}  // namespace pm::settings

#endif  // PM_SETTINGS__HANDLERS__TCP_HANDLER_HPP_
