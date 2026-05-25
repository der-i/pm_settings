/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#include "pm_settings/persistence/json_serializer.hpp"

#include <core_utils/log.h>
#include <fmt/format.h>

namespace pm::settings
{

std::optional<std::string> JSONSerializer::Serialize(const std::any & data) const
{
  try {
    if (const auto * obj = std::any_cast<json::object>(&data)) {
      return json::serialize(*obj);
    }
    if (const auto * val = std::any_cast<json::value>(&data)) {
      return json::serialize(*val);
    }
    LOG_E << "JSONSerializer: unsupported std::any payload";
    return std::nullopt;
  } catch (const std::exception & e) {
    LOG_E << fmt::format("JSONSerializer: serialize failed: {}", e.what());
    return std::nullopt;
  }
}

std::any JSONSerializer::Deserialize(const std::string & content) const
{
  json::error_code ec;
  const auto value = json::parse(content, ec);
  if (ec) {
    LOG_E << fmt::format("JSONSerializer: parse failed: {}", ec.message());
    return std::any{};
  }

  if (!value.is_object()) {
    LOG_E << "JSONSerializer: root is not an object";
    return std::any{};
  }

  return std::any{value.as_object()};
}

std::string JSONSerializer::GetExtension() const
{
  return ".json";
}

}  // namespace pm::settings
