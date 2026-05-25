/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#include "pm_settings/validators/joint_limits_validator.hpp"

#include <cmath>
#include <numbers>
#include <unordered_map>

#include <fmt/format.h>

namespace pm::settings
{

namespace
{

constexpr double DegToRad(double deg)
{
  return deg * std::numbers::pi_v<double> / 180.0;
}

}  // namespace

ValidationResult JointLimitsValidator::Validate(
  const std::any & value,
  const ValidationContext & context) const
{
  ValidationResult result = ValidationResult::Ok();

  const auto * cfg = std::any_cast<JointLimitsConfiguration>(&value);
  if (!cfg) {
    return ValidationResult::Error(
      "JointLimitsValidator: payload is not JointLimitsConfiguration");
  }

  if (cfg->limits.empty()) {
    return ValidationResult::Error("JointLimitsValidator: empty limits set");
  }

  for (const auto & joint : cfg->limits) {
    if (joint.name.empty()) {
      result.Add("JointLimitsValidator: joint with empty name");
      continue;
    }

    (void)CheckMinLessMax(joint, result);
    (void)CheckPhysicalLimits(joint, result);
    (void)CheckCurrentPosition(joint, context, result);
  }

  return result;
}

bool JointLimitsValidator::CheckMinLessMax(
  const JointLimit & joint, ValidationResult & r) const
{
  if (!(joint.min_position < joint.max_position)) {
    r.Add(fmt::format(
      "JointLimitsValidator: '{}' min_position {:.4f} >= max_position {:.4f}",
      joint.name, joint.min_position, joint.max_position));
    return false;
  }
  return true;
}

bool JointLimitsValidator::CheckPhysicalLimits(
  const JointLimit & joint, ValidationResult & r) const
{
  const double phys_limit = DegToRad(limits::JOINT_PHYSICAL_LIMIT_DEG);
  if (joint.min_position < -phys_limit || joint.max_position > phys_limit) {
    r.Add(fmt::format(
      "JointLimitsValidator: '{}' exceeds physical limit ±{:.0f}°",
      joint.name, limits::JOINT_PHYSICAL_LIMIT_DEG));
    return false;
  }
  return true;
}

bool JointLimitsValidator::CheckCurrentPosition(
  const JointLimit & joint,
  const ValidationContext & ctx,
  ValidationResult & r) const
{
  const auto * positions = std::any_cast<std::unordered_map<std::string, double>>(
    &ctx.current_state);
  if (!positions) {
    // Контекст не предоставлен - проверка пропускается, это допустимо.
    return true;
  }

  const auto it = positions->find(joint.name);
  if (it == positions->end()) {
    return true;
  }

  if (it->second < joint.min_position || it->second > joint.max_position) {
    r.Add(fmt::format(
      "JointLimitsValidator: '{}' current pos {:.4f} outside new limits [{:.4f}, {:.4f}]",
      joint.name, it->second, joint.min_position, joint.max_position));
    return false;
  }

  return true;
}

}  // namespace pm::settings
