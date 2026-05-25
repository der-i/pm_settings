/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#ifndef PM_SETTINGS__INTERFACES__I_VALIDATOR_HPP_
#define PM_SETTINGS__INTERFACES__I_VALIDATOR_HPP_

#include <any>
#include <string>
#include <vector>

namespace pm::settings
{

/// @brief Результат валидации параметра.
struct ValidationResult
{
  bool valid{false};
  std::vector<std::string> errors;

  [[nodiscard]] static ValidationResult Ok()
  {
    return ValidationResult{true, {}};
  }

  [[nodiscard]] static ValidationResult Error(std::string msg)
  {
    return ValidationResult{false, {std::move(msg)}};
  }

  void Add(std::string msg)
  {
    valid = false;
    errors.push_back(std::move(msg));
  }
};

/// @brief Контекст текущего состояния системы для валидации.
/// @details Передаётся валидатору, чтобы тот мог проверить консистентность
/// нового значения с уже применёнными параметрами (например, домашняя
/// позиция должна лежать в пределах уже установленных лимитов суставов).
struct ValidationContext
{
  std::any current_state;
};

/// @brief Интерфейс валидатора параметра.
/// @details Каждый тип параметра имеет свой валидатор, который проверяет
/// формат, диапазон и консистентность. Регистрация валидаторов выполняется
/// через ValidatorRegistry.
class IValidator
{
public:
  virtual ~IValidator() = default;

  /// @brief Проверить корректность значения параметра.
  /// @param value Значение параметра (тип зависит от конкретного валидатора).
  /// @param context Контекст текущего состояния системы.
  /// @return Результат валидации с описанием ошибок при неуспехе.
  [[nodiscard]] virtual ValidationResult Validate(
    const std::any & value,
    const ValidationContext & context) const = 0;
};

}  // namespace pm::settings

#endif  // PM_SETTINGS__INTERFACES__I_VALIDATOR_HPP_
