/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#ifndef PM_SETTINGS__PERSISTENCE__JSON_SERIALIZER_HPP_
#define PM_SETTINGS__PERSISTENCE__JSON_SERIALIZER_HPP_

#include <boost/json.hpp>

#include "pm_settings/interfaces/i_serializer.hpp"

namespace pm::settings
{

namespace json = boost::json;

/// @brief Сериализатор JSON на базе boost::json.
/// @details Принимает и отдаёт std::any с внутренним типом json::object.
/// Используется для tcp_list.json, base_link_transforms.json, objects.json,
/// work_frame_list.json. Числа хранятся в виде строк с фиксированной
/// точностью для исключения потерь при round-trip.
class JSONSerializer final : public ISerializer
{
public:
  [[nodiscard]] std::optional<std::string> Serialize(const std::any & data) const override;
  [[nodiscard]] std::any Deserialize(const std::string & content) const override;
  [[nodiscard]] std::string GetExtension() const override;
};

}  // namespace pm::settings

#endif  // PM_SETTINGS__PERSISTENCE__JSON_SERIALIZER_HPP_
