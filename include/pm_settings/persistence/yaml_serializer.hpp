/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#ifndef PM_SETTINGS__PERSISTENCE__YAML_SERIALIZER_HPP_
#define PM_SETTINGS__PERSISTENCE__YAML_SERIALIZER_HPP_

#include <yaml-cpp/yaml.h>

#include "pm_settings/interfaces/i_serializer.hpp"

namespace pm::settings
{

/// @brief Сериализатор YAML на базе yaml-cpp.
/// @details Принимает и отдаёт std::any с внутренним типом YAML::Node.
/// Используется для joint_limits.yaml и initial_positions.yaml, поскольку
/// данные файлы потребляются подсистемой MoveIt2 напрямую.
class YAMLSerializer final : public ISerializer
{
public:
  [[nodiscard]] std::optional<std::string> Serialize(const std::any & data) const override;
  [[nodiscard]] std::any Deserialize(const std::string & content) const override;
  [[nodiscard]] std::string GetExtension() const override;
};

}  // namespace pm::settings

#endif  // PM_SETTINGS__PERSISTENCE__YAML_SERIALIZER_HPP_
