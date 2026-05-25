/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#ifndef PM_SETTINGS__VALIDATORS__TCP_VALIDATOR_HPP_
#define PM_SETTINGS__VALIDATORS__TCP_VALIDATOR_HPP_

#include <vector>

#include "pm_settings/interfaces/i_validator.hpp"
#include "pm_settings/types/configurations.hpp"

namespace pm::settings
{

/// @brief Валидатор параметров центральной точки инструмента.
/// @details Проверяет: корректность позиции (конечные значения), нормализацию
/// кватерниона ориентации, уникальность имени конфигурации и ограничение
/// количества ЦТИ (не более MAX_TCP_COUNT).
class TCPValidator final : public IValidator
{
public:
  [[nodiscard]] ValidationResult Validate(
    const std::any & value,
    const ValidationContext & context) const override;

private:
  [[nodiscard]] bool CheckPoseRange(const TCPConfiguration & cfg, ValidationResult & r) const;
  [[nodiscard]] bool CheckNameUnique(
    const TCPConfiguration & cfg,
    const std::vector<TCPConfiguration> & existing,
    ValidationResult & r) const;
  [[nodiscard]] bool CheckMaxCount(
    const std::vector<TCPConfiguration> & existing,
    ValidationResult & r) const;
};

}  // namespace pm::settings

#endif  // PM_SETTINGS__VALIDATORS__TCP_VALIDATOR_HPP_
