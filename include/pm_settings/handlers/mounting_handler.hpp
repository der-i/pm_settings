/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#ifndef PM_SETTINGS__HANDLERS__MOUNTING_HANDLER_HPP_
#define PM_SETTINGS__HANDLERS__MOUNTING_HANDLER_HPP_

#include <memory>
#include <optional>
#include <vector>

#include "pm_settings/adapters/moveit_adapter.hpp"
#include "pm_settings/interfaces/i_settings_handler.hpp"
#include "pm_settings/persistence/config_file_manager.hpp"
#include "pm_settings/types/configurations.hpp"

namespace pm::settings
{

/// @brief Обработчик трансформации базового звена (монтажа манипулятора).
/// @details Вычисляет трансформацию world -> base_link по выбранной
/// конфигурации, обновляет Planning Scene через MoveItAdapter, персистирует
/// имя применённой конфигурации в last_used для восстановления при старте.
class MountingHandler final : public ISettingsHandler
{
public:
  MountingHandler(
    std::shared_ptr<MoveItAdapter> moveit_adapter,
    std::shared_ptr<ConfigFileManager> config_manager);

  [[nodiscard]] OperationResult Apply(const std::any & value) override;
  [[nodiscard]] OperationResult Rollback() override;
  [[nodiscard]] std::any GetCurrent() const override;
  [[nodiscard]] ParamType GetType() const noexcept override { return ParamType::MOUNTING; }

  /// @brief Загрузить last_used-конфигурацию при старте.
  [[nodiscard]] bool LoadFromDisk();

  [[nodiscard]] OperationResult ApplyByName(const std::string & name);
  [[nodiscard]] std::vector<MountingConfiguration> GetList() const;
  [[nodiscard]] OperationResult Delete(
    const std::string & name,
    bool reset_current,
    const std::string & apply_other);
  [[nodiscard]] OperationResult Reset();

private:
  [[nodiscard]] geometry_msgs::msg::TransformStamped ComputeTransform(
    const geometry_msgs::msg::Pose & pose) const;

  [[nodiscard]] bool PersistLastUsed(const std::string & name);

  std::shared_ptr<MoveItAdapter> m_moveit_adapter;
  std::shared_ptr<ConfigFileManager> m_config_manager;

  std::vector<MountingConfiguration> m_configurations;
  MountingConfiguration m_current;
  std::optional<MountingConfiguration> m_previous_state;
};

}  // namespace pm::settings

#endif  // PM_SETTINGS__HANDLERS__MOUNTING_HANDLER_HPP_
