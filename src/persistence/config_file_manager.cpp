/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#include "pm_settings/persistence/config_file_manager.hpp"

#include <fstream>
#include <sstream>
#include <utility>

#include <core_utils/log.h>
#include <fmt/format.h>

namespace pm::settings
{

ConfigFileManager::ConfigFileManager(std::filesystem::path config_dir)
: m_config_dir(std::move(config_dir))
{
  std::error_code ec;
  std::filesystem::create_directories(m_config_dir, ec);
  if (ec) {
    LOG_W << fmt::format(
      "ConfigFileManager: failed to create config dir '{}': {}",
      m_config_dir.string(), ec.message());
  }
}

void ConfigFileManager::RegisterSerializer(std::shared_ptr<ISerializer> serializer)
{
  if (!serializer) {
    LOG_W << "ConfigFileManager: refused to register null serializer";
    return;
  }

  const auto ext = serializer->GetExtension();
  m_serializers.insert_or_assign(ext, std::move(serializer));
}

std::any ConfigFileManager::Load(const std::filesystem::path & relative_path) const
{
  const auto full_path = ResolvePath(relative_path);
  const auto serializer = FindSerializer(full_path);
  if (!serializer) {
    LOG_E << fmt::format(
      "ConfigFileManager: no serializer for '{}'", full_path.string());
    return std::any{};
  }

  std::ifstream ifs(full_path);
  if (!ifs.is_open()) {
    LOG_E << fmt::format("ConfigFileManager: cannot open '{}'", full_path.string());
    return std::any{};
  }

  std::stringstream buffer;
  buffer << ifs.rdbuf();
  return serializer->Deserialize(buffer.str());
}

bool ConfigFileManager::Save(
  const std::filesystem::path & relative_path,
  const std::any & data) const
{
  const auto full_path = ResolvePath(relative_path);
  const auto serializer = FindSerializer(full_path);
  if (!serializer) {
    LOG_E << fmt::format(
      "ConfigFileManager: no serializer for '{}'", full_path.string());
    return false;
  }

  // Создаём резервную копию текущего файла перед записью.
  if (std::filesystem::exists(full_path)) {
    if (!Backup(relative_path)) {
      LOG_W << fmt::format(
        "ConfigFileManager: backup failed for '{}', proceeding anyway",
        full_path.string());
    }
  }

  const auto serialized = serializer->Serialize(data);
  if (!serialized.has_value()) {
    LOG_E << fmt::format(
      "ConfigFileManager: serialization failed for '{}'", full_path.string());
    return false;
  }

  return AtomicWrite(full_path, *serialized);
}

bool ConfigFileManager::Backup(
  const std::filesystem::path & relative_path,
  std::string_view suffix) const
{
  const auto full_path = ResolvePath(relative_path);
  if (!std::filesystem::exists(full_path)) {
    LOG_W << fmt::format(
      "ConfigFileManager: source '{}' does not exist", full_path.string());
    return false;
  }

  std::filesystem::path backup_path = full_path;
  backup_path += std::string(suffix);

  std::error_code ec;
  std::filesystem::copy_file(
    full_path, backup_path,
    std::filesystem::copy_options::overwrite_existing, ec);

  if (ec) {
    LOG_E << fmt::format(
      "ConfigFileManager: backup '{}' -> '{}' failed: {}",
      full_path.string(), backup_path.string(), ec.message());
    return false;
  }

  return true;
}

bool ConfigFileManager::Restore(
  const std::filesystem::path & relative_path,
  std::string_view suffix) const
{
  const auto full_path = ResolvePath(relative_path);
  std::filesystem::path backup_path = full_path;
  backup_path += std::string(suffix);

  if (!std::filesystem::exists(backup_path)) {
    LOG_E << fmt::format(
      "ConfigFileManager: backup '{}' does not exist", backup_path.string());
    return false;
  }

  std::error_code ec;
  std::filesystem::copy_file(
    backup_path, full_path,
    std::filesystem::copy_options::overwrite_existing, ec);

  if (ec) {
    LOG_E << fmt::format(
      "ConfigFileManager: restore '{}' -> '{}' failed: {}",
      backup_path.string(), full_path.string(), ec.message());
    return false;
  }

  return true;
}

std::filesystem::path ConfigFileManager::ResolvePath(
  const std::filesystem::path & relative_path) const
{
  return m_config_dir / relative_path;
}

std::shared_ptr<ISerializer> ConfigFileManager::FindSerializer(
  const std::filesystem::path & path) const
{
  const auto ext = path.extension().string();
  const auto it = m_serializers.find(ext);
  return (it != m_serializers.end()) ? it->second : nullptr;
}

bool ConfigFileManager::AtomicWrite(
  const std::filesystem::path & target,
  std::string_view content) const
{
  // Атомарность достигается записью во временный файл с последующим
  // переименованием. На POSIX rename(2) атомарен в пределах одной FS.
  std::filesystem::path tmp = target;
  tmp += ".tmp";

  {
    std::ofstream ofs(tmp, std::ios::binary | std::ios::trunc);
    if (!ofs.is_open()) {
      LOG_E << fmt::format(
        "ConfigFileManager: cannot open temp file '{}'", tmp.string());
      return false;
    }
    ofs.write(content.data(), static_cast<std::streamsize>(content.size()));
    if (!ofs) {
      LOG_E << fmt::format(
        "ConfigFileManager: write failed to '{}'", tmp.string());
      return false;
    }
    ofs.flush();
  }

  std::error_code ec;
  std::filesystem::rename(tmp, target, ec);
  if (ec) {
    LOG_E << fmt::format(
      "ConfigFileManager: rename '{}' -> '{}' failed: {}",
      tmp.string(), target.string(), ec.message());
    std::filesystem::remove(tmp, ec);
    return false;
  }

  return true;
}

}  // namespace pm::settings
