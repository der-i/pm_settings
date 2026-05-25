/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#ifndef PM_SETTINGS__CORE__VALIDATOR_REGISTRY_HPP_
#define PM_SETTINGS__CORE__VALIDATOR_REGISTRY_HPP_

#include <memory>
#include <unordered_map>

#include "pm_settings/interfaces/i_validator.hpp"
#include "pm_settings/types/common.hpp"

namespace pm::settings
{

/// @brief Реестр валидаторов параметров.
/// @details Хранит соответствие ParamType -> IValidator. SettingsManager
/// запрашивает валидатор по типу параметра перед делегированием обработчику.
/// Добавление нового типа параметра сводится к реализации IValidator
/// и регистрации его здесь (требование НТ-6 по расширяемости).
class ValidatorRegistry
{
public:
  /// @brief Зарегистрировать валидатор для указанного типа параметра.
  void Register(ParamType type, std::shared_ptr<IValidator> validator);

  /// @brief Получить валидатор по типу параметра.
  /// @return nullptr, если для типа не зарегистрирован валидатор.
  [[nodiscard]] std::shared_ptr<IValidator> Get(ParamType type) const;

  [[nodiscard]] bool Contains(ParamType type) const noexcept;

  [[nodiscard]] std::size_t Size() const noexcept;

private:
  std::unordered_map<ParamType, std::shared_ptr<IValidator>> m_validators;
};

}  // namespace pm::settings

#endif  // PM_SETTINGS__CORE__VALIDATOR_REGISTRY_HPP_
