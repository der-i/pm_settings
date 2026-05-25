/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#include <gtest/gtest.h>
#include <yaml-cpp/yaml.h>

#include "pm_settings/persistence/yaml_serializer.hpp"

using namespace pm::settings;

TEST(YAMLSerializerTest, RoundTripSimpleMap)
{
  YAMLSerializer s;
  YAML::Node src;
  src["key"] = "value";

  const auto serialized = s.Serialize(std::any{src});
  ASSERT_TRUE(serialized.has_value());

  const auto deserialized = s.Deserialize(*serialized);
  const auto * node = std::any_cast<YAML::Node>(&deserialized);
  ASSERT_NE(node, nullptr);
  EXPECT_EQ((*node)["key"].as<std::string>(), "value");
}

TEST(YAMLSerializerTest, RoundTripNestedJointLimits)
{
  YAMLSerializer s;
  YAML::Node src;
  src["joint_limits"]["joint_1"]["min_position"] = "-3.14";
  src["joint_limits"]["joint_1"]["max_position"] = "3.14";

  const auto serialized = s.Serialize(std::any{src});
  ASSERT_TRUE(serialized.has_value());

  const auto deserialized = s.Deserialize(*serialized);
  const auto * node = std::any_cast<YAML::Node>(&deserialized);
  ASSERT_NE(node, nullptr);
  EXPECT_EQ((*node)["joint_limits"]["joint_1"]["min_position"].as<std::string>(), "-3.14");
}

TEST(YAMLSerializerTest, RejectsWrongAnyType)
{
  YAMLSerializer s;
  const auto result = s.Serialize(std::any{42});
  EXPECT_FALSE(result.has_value());
}

TEST(YAMLSerializerTest, GetExtension)
{
  YAMLSerializer s;
  EXPECT_EQ(s.GetExtension(), ".yaml");
}