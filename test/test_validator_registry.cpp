/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#include <gtest/gtest.h>

#include "pm_settings/core/validator_registry.hpp"
#include "pm_settings/validators/tcp_validator.hpp"

using namespace pm::settings;

TEST(ValidatorRegistryTest, RegisterAndGet)
{
  ValidatorRegistry r;
  auto v = std::make_shared<TCPValidator>();
  r.Register(ParamType::TCP, v);

  EXPECT_TRUE(r.Contains(ParamType::TCP));
  EXPECT_EQ(r.Size(), 1u);
  EXPECT_EQ(r.Get(ParamType::TCP).get(), v.get());
}

TEST(ValidatorRegistryTest, GetMissingReturnsNull)
{
  ValidatorRegistry r;
  EXPECT_EQ(r.Get(ParamType::SCENE_OBJECT), nullptr);
  EXPECT_FALSE(r.Contains(ParamType::SCENE_OBJECT));
}

TEST(ValidatorRegistryTest, NullValidatorIgnored)
{
  ValidatorRegistry r;
  r.Register(ParamType::TCP, nullptr);
  EXPECT_EQ(r.Size(), 0u);
}

TEST(ValidatorRegistryTest, OverwriteExisting)
{
  ValidatorRegistry r;
  auto v1 = std::make_shared<TCPValidator>();
  auto v2 = std::make_shared<TCPValidator>();
  r.Register(ParamType::TCP, v1);
  r.Register(ParamType::TCP, v2);
  EXPECT_EQ(r.Size(), 1u);
  EXPECT_EQ(r.Get(ParamType::TCP).get(), v2.get());
}