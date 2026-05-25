/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#include "pm_settings/persistence/yaml_serializer.hpp"

#include <sstream>

#include <core_utils/log.h>
#include <fmt/format.h>

namespace pm::settings
{

std::optional<std::string> YAMLSerializer::Serialize(const std::any & data) const
{
  try {
    const auto * node = std::any_cast<YAML::Node>(&data);
    if (!node) {
      LOG_E << "YAMLSerializer: unsupported std::any payload, expected YAML::Node";
      return std::nullopt;
    }

    std::ostringstream oss;
    oss << *node;
    return oss.str();
  } catch (const std::exception & e) {
    LOG_E << fmt::format("YAMLSerializer: serialize failed: {}", e.what());
    return std::nullopt;
  }
}

std::any YAMLSerializer::Deserialize(const std::string & content) const
{
  try {
    return std::any{YAML::Load(content)};
  } catch (const YAML::Exception & e) {
    LOG_E << fmt::format("YAMLSerializer: parse failed: {}", e.what());
    return std::any{};
  }
}

std::string YAMLSerializer::GetExtension() const
{
  return ".yaml";
}

}  // namespace pm::settings
