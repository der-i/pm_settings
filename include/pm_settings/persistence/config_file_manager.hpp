/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#ifndef PM_SETTINGS__PERSISTENCE__CONFIG_FILE_MANAGER_HPP_
#define PM_SETTINGS__PERSISTENCE__CONFIG_FILE_MANAGER_HPP_

#include <any>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include "pm_settings/interfaces/i_serializer.hpp"

namespace pm::settings
{

/// @brief Менеджер конфигурационных файлов.
/// @details Координирует операции чтения/записи, атомарность записи через
/// временный файл с последующим переименованием, автоматическое создание
/// резервных копий перед каждым изменением. Делегирует сериализацию
/// зарегистрированным ISerializer в зависимости от расширения файла.
class ConfigFileManager
{
public:
  /// @param config_dir Директория для хранения конфигурационных файлов.
  explicit ConfigFileManager(std::filesystem::path config_dir);

  /// @brief Зарегистрировать сериализатор для обслуживания определённого расширения.
  void RegisterSerializer(std::shared_ptr<ISerializer> serializer);

  /// @brief Прочитать и десериализовать конфигурационный файл.
  /// @param relative_path Путь относительно config_dir.
  [[nodiscard]] std::any Load(const std::filesystem::path & relative_path) const;

  /// @brief Атомарно записать данные в конфигурационный файл.
  /// @details Запись выполняется в temp-файл, затем rename. Перед записью
  /// создаётся резервная копия существующего файла.
  [[nodiscard]] bool Save(
    const std::filesystem::path & relative_path,
    const std::any & data) const;

  /// @brief Создать резервную копию файла.
  /// @param relative_path Путь относительно config_dir.
  /// @param suffix Суффикс резервной копии (".backup" по умолчанию).
  [[nodiscard]] bool Backup(
    const std::filesystem::path & relative_path,
    std::string_view suffix = ".backup") const;

  /// @brief Восстановить файл из резервной копии.
  [[nodiscard]] bool Restore(
    const std::filesystem::path & relative_path,
    std::string_view suffix = ".backup") const;

  /// @brief Полный путь к файлу конфигурации.
  [[nodiscard]] std::filesystem::path ResolvePath(
    const std::filesystem::path & relative_path) const;

private:
  [[nodiscard]] std::shared_ptr<ISerializer> FindSerializer(
    const std::filesystem::path & path) const;

  [[nodiscard]] bool AtomicWrite(
    const std::filesystem::path & target,
    std::string_view content) const;

  std::filesystem::path m_config_dir;
  std::unordered_map<std::string, std::shared_ptr<ISerializer>> m_serializers;
};

}  // namespace pm::settings

#endif  // PM_SETTINGS__PERSISTENCE__CONFIG_FILE_MANAGER_HPP_
