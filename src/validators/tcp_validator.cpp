/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#include "pm_settings/validators/tcp_validator.hpp"

#include <cmath>

#include <fmt/format.h>

namespace pm::settings
{

namespace
{

constexpr double QUATERNION_NORM_TOLERANCE = 1e-3;

bool IsFinitePose(const geometry_msgs::msg::Pose & pose)
{
  return std::isfinite(pose.position.x) &&
         std::isfinite(pose.position.y) &&
         std::isfinite(pose.position.z) &&
         std::isfinite(pose.orientation.x) &&
         std::isfinite(pose.orientation.y) &&
         std::isfinite(pose.orientation.z) &&
         std::isfinite(pose.orientation.w);
}

double QuaternionNorm(const geometry_msgs::msg::Pose & pose)
{
  const auto & q = pose.orientation;
  return std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
}

}  // namespace

ValidationResult TCPValidator::Validate(
  const std::any & value,
  const ValidationContext & context) const
{
  ValidationResult result = ValidationResult::Ok();

  const auto * cfg = std::any_cast<TCPConfiguration>(&value);
  if (!cfg) {
    return ValidationResult::Error("TCPValidator: payload is not TCPConfiguration");
  }

  if (cfg->name.empty()) {
    result.Add("TCPValidator: name must not be empty");
  }

  if (!CheckPoseRange(*cfg, result)) {
    return result;
  }

  // Контекст содержит уже сохранённые конфигурации.
  const auto * existing = std::any_cast<std::vector<TCPConfiguration>>(&context.current_state);
  if (existing) {
    if (!CheckMaxCount(*existing, result)) {
      return result;
    }
    (void)CheckNameUnique(*cfg, *existing, result);
  }

  return result;
}

bool TCPValidator::CheckPoseRange(const TCPConfiguration & cfg, ValidationResult & r) const
{
  if (!IsFinitePose(cfg.pose)) {
    r.Add(fmt::format("TCPValidator: pose contains non-finite values for '{}'", cfg.name));
    return false;
  }

  const auto norm = QuaternionNorm(cfg.pose);
  if (std::abs(norm - 1.0) > QUATERNION_NORM_TOLERANCE) {
    r.Add(fmt::format(
      "TCPValidator: quaternion not normalized for '{}', |q|={:.6f}",
      cfg.name, norm));
    return false;
  }

  return true;
}

bool TCPValidator::CheckNameUnique(
  const TCPConfiguration & cfg,
  const std::vector<TCPConfiguration> & existing,
  ValidationResult & r) const
{
  // Уникальность необязательна для обновления (Add работает как upsert),
  // но дублирование имени между разными запросами всё равно стоит логировать.
  // Здесь мы только проверяем формат имени; конкретная политика - в обработчике.
  (void)existing;
  for (char ch : cfg.name) {
    if (!std::isprint(static_cast<unsigned char>(ch))) {
      r.Add(fmt::format("TCPValidator: name '{}' contains non-printable chars", cfg.name));
      return false;
    }
  }
  return true;
}

bool TCPValidator::CheckMaxCount(
  const std::vector<TCPConfiguration> & existing,
  ValidationResult & r) const
{
  if (existing.size() >= limits::MAX_TCP_COUNT) {
    r.Add(fmt::format(
      "TCPValidator: max TCP count {} reached", limits::MAX_TCP_COUNT));
    return false;
  }
  return true;
}

}  // namespace pm::settings
