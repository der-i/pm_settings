/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#ifndef PM_SETTINGS__VALIDATORS__MOUNTING_VALIDATOR_HPP_
#define PM_SETTINGS__VALIDATORS__MOUNTING_VALIDATOR_HPP_

#include "pm_settings/interfaces/i_validator.hpp"
#include "pm_settings/types/configurations.hpp"

namespace pm::settings
{

/// @brief Валидатор трансформации базового звена.
/// @details Проверяет нормализацию кватерниона ориентации и корректность
/// имени трансформации (непустое, ASCII).
class MountingValidator final : public IValidator
{
public:
  [[nodiscard]] ValidationResult Validate(
    const std::any & value,
    const ValidationContext & context) const override;

private:
  [[nodiscard]] bool CheckQuaternionNorm(
    const MountingConfiguration & cfg,
    ValidationResult & r) const;
  [[nodiscard]] bool CheckName(const MountingConfiguration & cfg, ValidationResult & r) const;
};

}  // namespace pm::settings

#endif  // PM_SETTINGS__VALIDATORS__MOUNTING_VALIDATOR_HPP_
