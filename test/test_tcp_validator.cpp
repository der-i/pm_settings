/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#include <gtest/gtest.h>

#include "pm_settings/validators/tcp_validator.hpp"

using namespace pm::settings;

namespace
{

TCPConfiguration MakeValidTCP(const std::string & name = "test")
{
  TCPConfiguration cfg;
  cfg.name = name;
  cfg.pose.position.x = 0.0;
  cfg.pose.position.y = 0.0;
  cfg.pose.position.z = 0.1;
  cfg.pose.orientation.x = 0.0;
  cfg.pose.orientation.y = 0.0;
  cfg.pose.orientation.z = 0.0;
  cfg.pose.orientation.w = 1.0;
  return cfg;
}

}  // namespace

TEST(TCPValidatorTest, AcceptsValidConfiguration)
{
  TCPValidator v;
  ValidationContext ctx;
  const auto result = v.Validate(std::any{MakeValidTCP()}, ctx);
  EXPECT_TRUE(result.valid);
  EXPECT_TRUE(result.errors.empty());
}

TEST(TCPValidatorTest, RejectsEmptyName)
{
  TCPValidator v;
  ValidationContext ctx;
  auto cfg = MakeValidTCP("");
  const auto result = v.Validate(std::any{cfg}, ctx);
  EXPECT_FALSE(result.valid);
}

TEST(TCPValidatorTest, RejectsUnnormalizedQuaternion)
{
  TCPValidator v;
  ValidationContext ctx;
  auto cfg = MakeValidTCP();
  cfg.pose.orientation.w = 2.0;
  const auto result = v.Validate(std::any{cfg}, ctx);
  EXPECT_FALSE(result.valid);
}

TEST(TCPValidatorTest, RejectsWhenMaxCountReached)
{
  TCPValidator v;
  std::vector<TCPConfiguration> existing(limits::MAX_TCP_COUNT, MakeValidTCP());
  ValidationContext ctx;
  ctx.current_state = existing;

  const auto result = v.Validate(std::any{MakeValidTCP("new")}, ctx);
  EXPECT_FALSE(result.valid);
}

TEST(TCPValidatorTest, RejectsNonTCPPayload)
{
  TCPValidator v;
  ValidationContext ctx;
  const auto result = v.Validate(std::any{42}, ctx);
  EXPECT_FALSE(result.valid);
}