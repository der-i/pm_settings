/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#ifndef PM_SETTINGS__VALIDATORS__JOINT_LIMITS_VALIDATOR_HPP_
#define PM_SETTINGS__VALIDATORS__JOINT_LIMITS_VALIDATOR_HPP_

#include "pm_settings/interfaces/i_validator.hpp"
#include "pm_settings/types/configurations.hpp"

namespace pm::settings
{

/// @brief Валидатор пределов хода суставов и домашнего положения.
/// @details Проверяет: min < max, диапазон в пределах физических ограничений
/// (±358°, ТЗ ФТ-3), валидность имён суставов, попадание текущей позиции
/// в новые ограничения.
class JointLimitsValidator final : public IValidator
{
public:
  [[nodiscard]] ValidationResult Validate(
    const std::any & value,
    const ValidationContext & context) const override;

private:
  [[nodiscard]] bool CheckMinLessMax(const JointLimit & joint, ValidationResult & r) const;
  [[nodiscard]] bool CheckPhysicalLimits(const JointLimit & joint, ValidationResult & r) const;
  [[nodiscard]] bool CheckCurrentPosition(
    const JointLimit & joint,
    const ValidationContext & ctx,
    ValidationResult & r) const;
};

}  // namespace pm::settings

#endif  // PM_SETTINGS__VALIDATORS__JOINT_LIMITS_VALIDATOR_HPP_
