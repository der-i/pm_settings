/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#include <gtest/gtest.h>

#include "pm_settings/validators/mounting_validator.hpp"

using namespace pm::settings;

namespace
{

MountingConfiguration MakeValid(const std::string & name = "wall_mount")
{
  MountingConfiguration cfg;
  cfg.name = name;
  cfg.pose.orientation.w = 1.0;
  return cfg;
}

}  // namespace

TEST(MountingValidatorTest, AcceptsValidConfiguration)
{
  MountingValidator v;
  const auto result = v.Validate(std::any{MakeValid()}, ValidationContext{});
  EXPECT_TRUE(result.valid);
}

TEST(MountingValidatorTest, RejectsEmptyName)
{
  MountingValidator v;
  auto cfg = MakeValid("");
  const auto result = v.Validate(std::any{cfg}, ValidationContext{});
  EXPECT_FALSE(result.valid);
}

TEST(MountingValidatorTest, RejectsUnnormalizedQuaternion)
{
  MountingValidator v;
  auto cfg = MakeValid();
  cfg.pose.orientation.w = 3.0;
  const auto result = v.Validate(std::any{cfg}, ValidationContext{});
  EXPECT_FALSE(result.valid);
}

TEST(MountingValidatorTest, RejectsNonMountingPayload)
{
  MountingValidator v;
  const auto result = v.Validate(std::any{std::string{"garbage"}}, ValidationContext{});
  EXPECT_FALSE(result.valid);
}

TEST(MountingValidatorTest, AcceptsAxisAlignedQuaternion)
{
  MountingValidator v;
  auto cfg = MakeValid();
  // Поворот на 90° вокруг Y.
  cfg.pose.orientation.x = 0.0;
  cfg.pose.orientation.y = 0.707107;
  cfg.pose.orientation.z = 0.0;
  cfg.pose.orientation.w = 0.707107;
  const auto result = v.Validate(std::any{cfg}, ValidationContext{});
  EXPECT_TRUE(result.valid);
}