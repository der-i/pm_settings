/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#ifndef PM_SETTINGS__HANDLERS__SCENE_HANDLER_HPP_
#define PM_SETTINGS__HANDLERS__SCENE_HANDLER_HPP_

#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "pm_settings/adapters/moveit_adapter.hpp"
#include "pm_settings/interfaces/i_settings_handler.hpp"
#include "pm_settings/persistence/config_file_manager.hpp"
#include "pm_settings/types/configurations.hpp"

namespace pm::settings
{

/// @brief Обработчик объектов сцены планирования движения.
/// @details Управляет жизненным циклом объектов сцены: добавление,
/// обновление, удаление. Применение происходит через MoveItAdapter
/// (PlanningSceneInterface), сохранение - в objects.json.
class SceneHandler final : public ISettingsHandler
{
public:
  SceneHandler(
    std::shared_ptr<MoveItAdapter> moveit_adapter,
    std::shared_ptr<ConfigFileManager> config_manager);

  [[nodiscard]] OperationResult Apply(const std::any & value) override;
  [[nodiscard]] OperationResult Rollback() override;
  [[nodiscard]] std::any GetCurrent() const override;
  [[nodiscard]] ParamType GetType() const noexcept override { return ParamType::SCENE_OBJECT; }

  [[nodiscard]] bool LoadFromDisk();

  /// @brief Удалить объект сцены по имени.
  [[nodiscard]] OperationResult Remove(const std::string & name);

  [[nodiscard]] std::vector<SceneObject> GetAll() const;
  [[nodiscard]] std::optional<SceneObject> GetByName(const std::string & name) const;
  [[nodiscard]] std::vector<std::string> GetNames() const;

  /// @brief Принудительно опубликовать всю сцену в MoveIt2.
  [[nodiscard]] OperationResult ForceUpdateScene();

private:
  [[nodiscard]] OperationResult Persist();

  std::shared_ptr<MoveItAdapter> m_moveit_adapter;
  std::shared_ptr<ConfigFileManager> m_config_manager;

  std::unordered_map<std::string, SceneObject> m_objects;
  std::optional<std::unordered_map<std::string, SceneObject>> m_previous_state;
};

}  // namespace pm::settings

#endif  // PM_SETTINGS__HANDLERS__SCENE_HANDLER_HPP_
