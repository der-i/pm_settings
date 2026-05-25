/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#include <filesystem>

#include <boost/json.hpp>
#include <gtest/gtest.h>

#include "pm_settings/persistence/config_file_manager.hpp"
#include "pm_settings/persistence/json_serializer.hpp"

using namespace pm::settings;

namespace
{

std::filesystem::path MakeTempDir()
{
  auto dir = std::filesystem::temp_directory_path() /
             ("pm_settings_test_" + std::to_string(std::rand()));
  std::filesystem::create_directories(dir);
  return dir;
}

}  // namespace

TEST(ConfigFileManagerTest, SaveAndLoadRoundTrip)
{
  const auto tmp = MakeTempDir();
  ConfigFileManager mgr(tmp);
  mgr.RegisterSerializer(std::make_shared<JSONSerializer>());

  boost::json::object payload;
  payload["key"] = "value";
  payload["number"] = "42";

  ASSERT_TRUE(mgr.Save("test.json", std::any{payload}));

  const auto loaded = mgr.Load("test.json");
  const auto * obj = std::any_cast<boost::json::object>(&loaded);
  ASSERT_NE(obj, nullptr);
  EXPECT_EQ(obj->at("key").as_string(), "value");
  EXPECT_EQ(obj->at("number").as_string(), "42");

  std::filesystem::remove_all(tmp);
}

TEST(ConfigFileManagerTest, BackupCreatesCopy)
{
  const auto tmp = MakeTempDir();
  ConfigFileManager mgr(tmp);
  mgr.RegisterSerializer(std::make_shared<JSONSerializer>());

  boost::json::object payload;
  payload["k"] = "v";
  ASSERT_TRUE(mgr.Save("file.json", std::any{payload}));
  ASSERT_TRUE(mgr.Backup("file.json"));
  EXPECT_TRUE(std::filesystem::exists(tmp / "file.json.backup"));

  std::filesystem::remove_all(tmp);
}

TEST(ConfigFileManagerTest, RestoreFromBackup)
{
  const auto tmp = MakeTempDir();
  ConfigFileManager mgr(tmp);
  mgr.RegisterSerializer(std::make_shared<JSONSerializer>());

  boost::json::object original;
  original["v"] = "first";
  ASSERT_TRUE(mgr.Save("data.json", std::any{original}));
  ASSERT_TRUE(mgr.Backup("data.json"));

  boost::json::object updated;
  updated["v"] = "second";
  ASSERT_TRUE(mgr.Save("data.json", std::any{updated}));
  ASSERT_TRUE(mgr.Restore("data.json"));

  const auto loaded = mgr.Load("data.json");
  const auto * obj = std::any_cast<boost::json::object>(&loaded);
  ASSERT_NE(obj, nullptr);
  EXPECT_EQ(obj->at("v").as_string(), "first");

  std::filesystem::remove_all(tmp);
}

TEST(ConfigFileManagerTest, LoadMissingFileReturnsEmpty)
{
  const auto tmp = MakeTempDir();
  ConfigFileManager mgr(tmp);
  mgr.RegisterSerializer(std::make_shared<JSONSerializer>());

  const auto loaded = mgr.Load("nonexistent.json");
  EXPECT_FALSE(loaded.has_value());

  std::filesystem::remove_all(tmp);
}