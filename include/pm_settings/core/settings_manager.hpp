/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#ifndef PM_SETTINGS__CORE__SETTINGS_MANAGER_HPP_
#define PM_SETTINGS__CORE__SETTINGS_MANAGER_HPP_

#include <any>
#include <memory>
#include <unordered_map>

#include "pm_settings/core/validator_registry.hpp"
#include "pm_settings/interfaces/i_settings_handler.hpp"
#include "pm_settings/persistence/config_file_manager.hpp"
#include "pm_settings/types/common.hpp"

namespace pm::settings
{

/// @brief Координатор процесса применения настроек (центральный класс
/// бизнес-логики модуля).
/// @details Реализует общий четырёхэтапный алгоритм применения параметра:
///   1) валидация - запрос валидатора из ValidatorRegistry;
///   2) применение - делегирование обработчику ISettingsHandler;
///   3) сохранение - вызов ConfigFileManager;
///   4) подтверждение - возврат результата клиенту.
/// При ошибке на любом этапе после применения выполняется откат через
/// Rollback() обработчика.
class SettingsManager
{
public:
  SettingsManager(
    std::shared_ptr<ValidatorRegistry> validator_registry,
    std::shared_ptr<ConfigFileManager> config_manager);

  /// @brief Зарегистрировать обработчик параметра.
  void RegisterHandler(std::shared_ptr<ISettingsHandler> handler);

  /// @brief Обработать запрос на установку параметра.
  /// @param type Тип параметра.
  /// @param value Значение параметра.
  /// @return Результат операции с описанием этапа сбоя при неуспехе.
  [[nodiscard]] OperationResult Process(ParamType type, const std::any & value);

  /// @brief Получить текущее значение параметра.
  [[nodiscard]] std::any GetCurrent(ParamType type) const;

  /// @brief Получить обработчик по типу (используется SettingsServiceNode для
  /// прямого вызова специфичных методов, таких как ApplyByName).
  [[nodiscard]] std::shared_ptr<ISettingsHandler> GetHandler(ParamType type) const;

private:
  [[nodiscard]] OperationResult ValidateStage(
    ParamType type,
    const std::any & value,
    const ValidationContext & ctx) const;

  [[nodiscard]] OperationResult ApplyStage(
    const std::shared_ptr<ISettingsHandler> & handler,
    const std::any & value);

  std::shared_ptr<ValidatorRegistry> m_validator_registry;
  std::shared_ptr<ConfigFileManager> m_config_manager;
  std::unordered_map<ParamType, std::shared_ptr<ISettingsHandler>> m_handlers;
};

}  // namespace pm::settings

#endif  // PM_SETTINGS__CORE__SETTINGS_MANAGER_HPP_
