/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#ifndef PM_SETTINGS__TYPES__COMMON_HPP_
#define PM_SETTINGS__TYPES__COMMON_HPP_

#include <cstdint>
#include <string>
#include <string_view>

#include <geometry_msgs/msg/pose.hpp>
#include <geometry_msgs/msg/transform_stamped.hpp>

namespace pm::settings
{

/// @brief Тип настраиваемого параметра. Используется SettingsManager
/// для диспетчеризации запросов между обработчиками и валидаторами.
enum class ParamType : std::uint8_t
{
  TCP = 0,
  MOUNTING = 1,
  JOINT_LIMITS = 2,
  HOME_POSITION = 3,
  SCENE_OBJECT = 4,
  WORK_FRAME = 5,
  UNKNOWN = 255
};

/// @brief Код результата операции.
/// @details Набор кодов покрывает все этапы четырёхэтапного процесса применения
/// параметра (валидация -> применение -> сохранение -> подтверждение).
enum class ResultCode : std::uint8_t
{
  SUCCESS = 0,
  WARNING_NOT_FOUND_JOINT = 1,
  CONFIG_NOT_EXIST = 2,
  INVALID_CONFIG = 3,
  BACKUP_FAILED = 4,
  RUNTIME_UPDATE_FAILED = 5,
  NODE_RESTART_FAILED = 6,
  VALIDATION_FAILED = 7,
  ROLLBACK_FAILED = 8,
  INTERNAL_ERROR = 255
};

[[nodiscard]] constexpr std::string_view ToString(const ResultCode code) noexcept
{
  switch (code) {
    case ResultCode::SUCCESS:                 return "SUCCESS";
    case ResultCode::WARNING_NOT_FOUND_JOINT: return "WARNING_NOT_FOUND_JOINT";
    case ResultCode::CONFIG_NOT_EXIST:        return "CONFIG_NOT_EXIST";
    case ResultCode::INVALID_CONFIG:          return "INVALID_CONFIG";
    case ResultCode::BACKUP_FAILED:           return "BACKUP_FAILED";
    case ResultCode::RUNTIME_UPDATE_FAILED:   return "RUNTIME_UPDATE_FAILED";
    case ResultCode::NODE_RESTART_FAILED:     return "NODE_RESTART_FAILED";
    case ResultCode::VALIDATION_FAILED:       return "VALIDATION_FAILED";
    case ResultCode::ROLLBACK_FAILED:         return "ROLLBACK_FAILED";
    case ResultCode::INTERNAL_ERROR:          return "INTERNAL_ERROR";
  }
  return "UNKNOWN";
}

/// @brief Обобщённый результат выполнения операции.
struct OperationResult
{
  bool success{false};
  ResultCode code{ResultCode::INTERNAL_ERROR};
  std::string message;

  [[nodiscard]] static OperationResult Ok(std::string msg = "success")
  {
    return OperationResult{true, ResultCode::SUCCESS, std::move(msg)};
  }

  [[nodiscard]] static OperationResult Fail(const ResultCode c, std::string msg)
  {
    return OperationResult{false, c, std::move(msg)};
  }
};

namespace frames
{

constexpr std::string_view WORLD   = "world";
constexpr std::string_view BASE    = "base_link";
constexpr std::string_view FLANGE  = "flange";
constexpr std::string_view TOOL    = "tool";
constexpr std::string_view WORK    = "work_frame";

}  // namespace frames

namespace limits
{

/// Максимальное число одновременно хранимых конфигураций ЦТИ (ТЗ, ФТ-1).
constexpr std::size_t MAX_TCP_COUNT = 5;

/// Физический предел поворота сустава в обе стороны (ТЗ, ФТ-3).
constexpr double JOINT_PHYSICAL_LIMIT_DEG = 358.0;

/// Ограничение применения одного параметра (ТЗ, НТ-1).
constexpr std::int64_t APPLY_DEADLINE_MS = 500;

}  // namespace limits

}  // namespace pm::settings

#endif  // PM_SETTINGS__TYPES__COMMON_HPP_
