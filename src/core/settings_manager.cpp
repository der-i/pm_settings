/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#include "pm_settings/core/settings_manager.hpp"

#include <utility>

#include <core_utils/log.h>
#include <fmt/format.h>

namespace pm::settings
{

SettingsManager::SettingsManager(
  std::shared_ptr<ValidatorRegistry> validator_registry,
  std::shared_ptr<ConfigFileManager> config_manager)
: m_validator_registry(std::move(validator_registry)),
  m_config_manager(std::move(config_manager))
{}

void SettingsManager::RegisterHandler(std::shared_ptr<ISettingsHandler> handler)
{
  if (!handler) {
    LOG_W << "SettingsManager: refused to register null handler";
    return;
  }
  m_handlers.insert_or_assign(handler->GetType(), std::move(handler));
}

OperationResult SettingsManager::Process(ParamType type, const std::any & value)
{
  // Этап 1: валидация.
  ValidationContext ctx;
  if (const auto handler = GetHandler(type); handler) {
    ctx.current_state = handler->GetCurrent();
  }
  const auto validation = ValidateStage(type, value, ctx);
  if (!validation.success) {
    return validation;
  }

  // Этап 2: применение через обработчик.
  const auto handler = GetHandler(type);
  if (!handler) {
    return OperationResult::Fail(
      ResultCode::INTERNAL_ERROR,
      fmt::format("SettingsManager: no handler for type {}", static_cast<int>(type)));
  }

  const auto apply_result = ApplyStage(handler, value);
  if (!apply_result.success) {
    // Этап 4 (откат): пытаемся восстановить состояние.
    const auto rollback = handler->Rollback();
    if (!rollback.success) {
      return OperationResult::Fail(
        ResultCode::ROLLBACK_FAILED,
        fmt::format("apply failed: {}; rollback failed: {}",
          apply_result.message, rollback.message));
    }
    return apply_result;
  }

  // Этап 3 (сохранение): уже выполнено внутри обработчика, поскольку
  // обработчик владеет знанием о формате и местоположении конфигурации.

  return apply_result;
}

std::any SettingsManager::GetCurrent(ParamType type) const
{
  const auto handler = GetHandler(type);
  return handler ? handler->GetCurrent() : std::any{};
}

std::shared_ptr<ISettingsHandler> SettingsManager::GetHandler(ParamType type) const
{
  const auto it = m_handlers.find(type);
  return (it != m_handlers.end()) ? it->second : nullptr;
}

OperationResult SettingsManager::ValidateStage(
  ParamType type,
  const std::any & value,
  const ValidationContext & ctx) const
{
  const auto validator = m_validator_registry->Get(type);
  if (!validator) {
    // Отсутствие валидатора - не критическая ошибка: считаем, что параметр
    // не нуждается в проверке (например, простые операции типа Reset).
    LOG_W << fmt::format(
      "SettingsManager: no validator for type {}, skipping validation",
      static_cast<int>(type));
    return OperationResult::Ok();
  }

  const auto result = validator->Validate(value, ctx);
  if (!result.valid) {
    std::string joined;
    for (const auto & err : result.errors) {
      joined += err;
      joined += "; ";
    }
    return OperationResult::Fail(ResultCode::VALIDATION_FAILED, std::move(joined));
  }

  return OperationResult::Ok();
}

OperationResult SettingsManager::ApplyStage(
  const std::shared_ptr<ISettingsHandler> & handler,
  const std::any & value)
{
  return handler->Apply(value);
}

}  // namespace pm::settings
