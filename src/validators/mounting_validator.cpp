/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#include "pm_settings/validators/mounting_validator.hpp"

#include <cmath>

#include <fmt/format.h>

namespace pm::settings
{

namespace
{
constexpr double QUATERNION_NORM_TOLERANCE = 1e-3;
}  // namespace

ValidationResult MountingValidator::Validate(
  const std::any & value,
  const ValidationContext & /*context*/) const
{
  ValidationResult result = ValidationResult::Ok();

  const auto * cfg = std::any_cast<MountingConfiguration>(&value);
  if (!cfg) {
    return ValidationResult::Error("MountingValidator: payload is not MountingConfiguration");
  }

  (void)CheckName(*cfg, result);
  (void)CheckQuaternionNorm(*cfg, result);
  return result;
}

bool MountingValidator::CheckQuaternionNorm(
  const MountingConfiguration & cfg, ValidationResult & r) const
{
  const auto & q = cfg.pose.orientation;
  const double norm = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
  if (std::abs(norm - 1.0) > QUATERNION_NORM_TOLERANCE) {
    r.Add(fmt::format(
      "MountingValidator: quaternion not normalized for '{}', |q|={:.6f}",
      cfg.name, norm));
    return false;
  }
  return true;
}

bool MountingValidator::CheckName(
  const MountingConfiguration & cfg, ValidationResult & r) const
{
  if (cfg.name.empty()) {
    r.Add("MountingValidator: name must not be empty");
    return false;
  }
  return true;
}

}  // namespace pm::settings
