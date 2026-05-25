/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#include <gtest/gtest.h>

#include "pm_settings/validators/joint_limits_validator.hpp"

using namespace pm::settings;

namespace
{

JointLimit MakeJoint(
  const std::string & name = "joint_1",
  double mn = -3.14,
  double mx = 3.14)
{
  JointLimit j;
  j.name = name;
  j.min_position = mn;
  j.max_position = mx;
  j.max_velocity = 1.0;
  j.max_acceleration = 1.0;
  return j;
}

}  // namespace

TEST(JointLimitsValidatorTest, AcceptsValidLimits)
{
  JointLimitsConfiguration cfg;
  cfg.limits.push_back(MakeJoint());
  cfg.limits.push_back(MakeJoint("joint_2"));

  JointLimitsValidator v;
  const auto result = v.Validate(std::any{cfg}, ValidationContext{});
  EXPECT_TRUE(result.valid);
}

TEST(JointLimitsValidatorTest, RejectsMinGreaterThanMax)
{
  JointLimitsConfiguration cfg;
  cfg.limits.push_back(MakeJoint("bad", 1.0, -1.0));

  JointLimitsValidator v;
  const auto result = v.Validate(std::any{cfg}, ValidationContext{});
  EXPECT_FALSE(result.valid);
}

TEST(JointLimitsValidatorTest, RejectsBeyondPhysicalLimit)
{
  JointLimitsConfiguration cfg;
  cfg.limits.push_back(MakeJoint("over", -100.0, 100.0));

  JointLimitsValidator v;
  const auto result = v.Validate(std::any{cfg}, ValidationContext{});
  EXPECT_FALSE(result.valid);
}

TEST(JointLimitsValidatorTest, RejectsEmptyConfig)
{
  JointLimitsConfiguration cfg;
  JointLimitsValidator v;
  const auto result = v.Validate(std::any{cfg}, ValidationContext{});
  EXPECT_FALSE(result.valid);
}