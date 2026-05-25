/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#ifndef PM_SETTINGS__INTERFACES__I_SERIALIZER_HPP_
#define PM_SETTINGS__INTERFACES__I_SERIALIZER_HPP_

#include <any>
#include <filesystem>
#include <optional>
#include <string>

namespace pm::settings
{

/// @brief Интерфейс сериализатора конфигурационных данных.
/// @details Абстрагирует работу с конкретным форматом файла (JSON/YAML).
/// Добавление нового формата выполняется реализацией этого интерфейса
/// и регистрацией в ConfigFileManager без модификации других слоёв (НТ-6).
class ISerializer
{
public:
  virtual ~ISerializer() = default;

  /// @brief Сериализовать данные в строку.
  /// @param data Произвольные данные, структура определяется сериализатором.
  /// @return Сериализованное строковое представление либо std::nullopt при ошибке.
  [[nodiscard]] virtual std::optional<std::string> Serialize(const std::any & data) const = 0;

  /// @brief Десериализовать строку в данные.
  [[nodiscard]] virtual std::any Deserialize(const std::string & content) const = 0;

  /// @brief Расширение файлов, обслуживаемых сериализатором (например, ".json").
  [[nodiscard]] virtual std::string GetExtension() const = 0;
};

}  // namespace pm::settings

#endif  // PM_SETTINGS__INTERFACES__I_SERIALIZER_HPP_
