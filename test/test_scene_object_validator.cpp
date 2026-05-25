/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#include <gtest/gtest.h>

#include "pm_settings/validators/scene_object_validator.hpp"

using namespace pm::settings;

namespace
{

SceneObject MakeCube(const std::string & name = "obj")
{
  SceneObject o;
  o.name = name;
  o.parent_frame = "base_link";
  o.type = SceneObjectType::CUBE;
  o.pose.orientation.w = 1.0;
  o.dimension.x = 0.1;
  o.dimension.y = 0.1;
  o.dimension.z = 0.1;
  return o;
}

}  // namespace

TEST(SceneObjectValidatorTest, AcceptsValidCube)
{
  SceneObjectValidator v;
  const auto result = v.Validate(std::any{MakeCube()}, ValidationContext{});
  EXPECT_TRUE(result.valid);
}

TEST(SceneObjectValidatorTest, RejectsZeroDimension)
{
  SceneObjectValidator v;
  auto obj = MakeCube();
  obj.dimension.y = 0.0;
  const auto result = v.Validate(std::any{obj}, ValidationContext{});
  EXPECT_FALSE(result.valid);
}

TEST(SceneObjectValidatorTest, RejectsEmptyName)
{
  SceneObjectValidator v;
  auto obj = MakeCube("");
  const auto result = v.Validate(std::any{obj}, ValidationContext{});
  EXPECT_FALSE(result.valid);
}

TEST(SceneObjectValidatorTest, AcceptsPointWithoutDimensions)
{
  SceneObjectValidator v;
  auto obj = MakeCube("point_obj");
  obj.type = SceneObjectType::POINT;
  obj.dimension = {};
  const auto result = v.Validate(std::any{obj}, ValidationContext{});
  EXPECT_TRUE(result.valid);
}