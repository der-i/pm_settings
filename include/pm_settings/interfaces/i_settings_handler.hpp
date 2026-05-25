/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#ifndef PM_SETTINGS__INTERFACES__I_SETTINGS_HANDLER_HPP_
#define PM_SETTINGS__INTERFACES__I_SETTINGS_HANDLER_HPP_

#include <any>

#include "pm_settings/types/common.hpp"

namespace pm::settings
{

/// @brief Интерфейс обработчика параметра.
/// @details Каждый обработчик инкапсулирует логику применения конкретного
/// типа параметра к компонентам MControl. Контракт: перед вызовом Apply()
/// обработчик обязан сохранить текущее состояние, чтобы Rollback() мог
/// корректно восстановить его при сбое на любом последующем этапе.
class ISettingsHandler
{
public:
  virtual ~ISettingsHandler() = default;

  /// @brief Применить параметр к runtime-компонентам MControl.
  /// @param value Значение параметра (тип зависит от конкретного обработчика).
  /// @return Результат операции с кодом ошибки при неуспехе.
  [[nodiscard]] virtual OperationResult Apply(const std::any & value) = 0;

  /// @brief Откатить последнее изменение к предыдущему состоянию.
  /// @return Результат операции. В случае неуспеха отката клиенту
  /// возвращается код ROLLBACK_FAILED, а система остаётся в
  /// неопределённом состоянии.
  [[nodiscard]] virtual OperationResult Rollback() = 0;

  /// @brief Получить текущее значение параметра.
  /// @return Значение параметра (тип зависит от обработчика).
  [[nodiscard]] virtual std::any GetCurrent() const = 0;

  /// @brief Тип параметра, обрабатываемого данным обработчиком.
  [[nodiscard]] virtual ParamType GetType() const noexcept = 0;
};

}  // namespace pm::settings

#endif  // PM_SETTINGS__INTERFACES__I_SETTINGS_HANDLER_HPP_
