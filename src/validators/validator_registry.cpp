/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#include "pm_settings/core/validator_registry.hpp"

#include <utility>

#include <core_utils/log.h>
#include <fmt/format.h>

namespace pm::settings
{

void ValidatorRegistry::Register(ParamType type, std::shared_ptr<IValidator> validator)
{
  if (!validator) {
    LOG_W << fmt::format(
      "ValidatorRegistry: refused to register null validator for type {}",
      static_cast<int>(type));
    return;
  }
  m_validators.insert_or_assign(type, std::move(validator));
}

std::shared_ptr<IValidator> ValidatorRegistry::Get(ParamType type) const
{
  const auto it = m_validators.find(type);
  return (it != m_validators.end()) ? it->second : nullptr;
}

bool ValidatorRegistry::Contains(ParamType type) const noexcept
{
  return m_validators.find(type) != m_validators.end();
}

std::size_t ValidatorRegistry::Size() const noexcept
{
  return m_validators.size();
}

}  // namespace pm::settings
