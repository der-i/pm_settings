/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#include <boost/json.hpp>
#include <gtest/gtest.h>

#include "pm_settings/persistence/json_serializer.hpp"

using namespace pm::settings;

TEST(JSONSerializerTest, RoundTripSimpleObject)
{
  JSONSerializer s;
  boost::json::object src;
  src["key"] = "value";

  const auto serialized = s.Serialize(std::any{src});
  ASSERT_TRUE(serialized.has_value());

  const auto deserialized = s.Deserialize(*serialized);
  const auto * obj = std::any_cast<boost::json::object>(&deserialized);
  ASSERT_NE(obj, nullptr);
  EXPECT_EQ(obj->at("key").as_string(), "value");
}

TEST(JSONSerializerTest, RoundTripNestedArray)
{
  JSONSerializer s;
  boost::json::object root;
  boost::json::array arr;
  boost::json::object item;
  item["name"] = "test";
  arr.push_back(item);
  root["items"] = std::move(arr);

  const auto serialized = s.Serialize(std::any{root});
  ASSERT_TRUE(serialized.has_value());

  const auto deserialized = s.Deserialize(*serialized);
  const auto * obj = std::any_cast<boost::json::object>(&deserialized);
  ASSERT_NE(obj, nullptr);
  ASSERT_TRUE(obj->at("items").is_array());
  EXPECT_EQ(obj->at("items").as_array().size(), 1u);
}

TEST(JSONSerializerTest, RejectsWrongAnyType)
{
  JSONSerializer s;
  const auto result = s.Serialize(std::any{42});
  EXPECT_FALSE(result.has_value());
}

TEST(JSONSerializerTest, DeserializeInvalidStringReturnsEmpty)
{
  JSONSerializer s;
  const auto result = s.Deserialize("{not valid json}");
  EXPECT_FALSE(result.has_value());
}

TEST(JSONSerializerTest, DeserializeNonObjectRootReturnsEmpty)
{
  JSONSerializer s;
  const auto result = s.Deserialize("[1, 2, 3]");
  EXPECT_FALSE(result.has_value());
}

TEST(JSONSerializerTest, GetExtension)
{
  JSONSerializer s;
  EXPECT_EQ(s.GetExtension(), ".json");
}