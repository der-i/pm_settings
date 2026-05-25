/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#include <core_msgs/srv/add_transform.hpp>
#include <core_msgs/srv/apply_transform.hpp>
#include <core_msgs/srv/delete_transform.hpp>
#include <std_srvs/srv/trigger.hpp>

#include "integration_fixture.hpp"

namespace pm::settings::test
{

namespace
{

core_msgs::srv::AddTransform::Request::SharedPtr MakeAddTransformRequest(
  const std::string & name, bool apply = true)
{
  auto req = std::make_shared<core_msgs::srv::AddTransform::Request>();
  req->name = name;
  req->pose.orientation.w = 1.0;
  req->apply = apply;
  return req;
}

}  // namespace

using BaseLinkIntegrationTest = IntegrationFixture;

TEST_F(BaseLinkIntegrationTest, AddTransform)
{
  const auto resp = CallService<core_msgs::srv::AddTransform>(
    "/pm_settings_test/base_link/add", MakeAddTransformRequest("floor_mount"));
  ASSERT_NE(resp, nullptr);
  EXPECT_TRUE(resp->result);
}

TEST_F(BaseLinkIntegrationTest, AddTransformAppearsInList)
{
  ASSERT_NE(CallService<core_msgs::srv::AddTransform>(
    "/pm_settings_test/base_link/add", MakeAddTransformRequest("wall_mount")), nullptr);

  auto req = std::make_shared<std_srvs::srv::Trigger::Request>();
  const auto resp = CallService<std_srvs::srv::Trigger>(
    "/pm_settings_test/base_link/get_list", req);
  ASSERT_NE(resp, nullptr);
  EXPECT_TRUE(resp->success);
  EXPECT_NE(resp->message.find("wall_mount"), std::string::npos);
}

TEST_F(BaseLinkIntegrationTest, ApplyTransformByName)
{
  ASSERT_NE(CallService<core_msgs::srv::AddTransform>(
    "/pm_settings_test/base_link/add",
    MakeAddTransformRequest("wall", false)), nullptr);

  auto apply_req = std::make_shared<core_msgs::srv::ApplyTransform::Request>();
  apply_req->name = "wall";
  const auto resp = CallService<core_msgs::srv::ApplyTransform>(
    "/pm_settings_test/base_link/apply", apply_req);
  ASSERT_NE(resp, nullptr);
  EXPECT_TRUE(resp->result);
}

TEST_F(BaseLinkIntegrationTest, ApplyUnknownTransformFails)
{
  auto req = std::make_shared<core_msgs::srv::ApplyTransform::Request>();
  req->name = "ghost";
  const auto resp = CallService<core_msgs::srv::ApplyTransform>(
    "/pm_settings_test/base_link/apply", req);
  ASSERT_NE(resp, nullptr);
  EXPECT_FALSE(resp->result);
}

TEST_F(BaseLinkIntegrationTest, DeleteTransform)
{
  ASSERT_NE(CallService<core_msgs::srv::AddTransform>(
    "/pm_settings_test/base_link/add", MakeAddTransformRequest("tmp")), nullptr);

  auto del_req = std::make_shared<core_msgs::srv::DeleteTransform::Request>();
  del_req->name = "tmp";
  del_req->reset_current = true;
  const auto resp = CallService<core_msgs::srv::DeleteTransform>(
    "/pm_settings_test/base_link/delete", del_req);
  ASSERT_NE(resp, nullptr);
  EXPECT_TRUE(resp->result);
}

TEST_F(BaseLinkIntegrationTest, DeleteWithApplyOther)
{
  ASSERT_NE(CallService<core_msgs::srv::AddTransform>(
    "/pm_settings_test/base_link/add",
    MakeAddTransformRequest("first")), nullptr);
  ASSERT_NE(CallService<core_msgs::srv::AddTransform>(
    "/pm_settings_test/base_link/add",
    MakeAddTransformRequest("second", false)), nullptr);

  auto del_req = std::make_shared<core_msgs::srv::DeleteTransform::Request>();
  del_req->name = "first";
  del_req->reset_current = false;
  del_req->apply_other = "second";
  const auto resp = CallService<core_msgs::srv::DeleteTransform>(
    "/pm_settings_test/base_link/delete", del_req);
  ASSERT_NE(resp, nullptr);
  EXPECT_TRUE(resp->result);
}

TEST_F(BaseLinkIntegrationTest, Reset)
{
  auto req = std::make_shared<std_srvs::srv::Trigger::Request>();
  const auto resp = CallService<std_srvs::srv::Trigger>(
    "/pm_settings_test/base_link/reset", req);
  ASSERT_NE(resp, nullptr);
  EXPECT_TRUE(resp->success);
}

TEST_F(BaseLinkIntegrationTest, RejectsEmptyName)
{
  auto req = MakeAddTransformRequest("");
  const auto resp = CallService<core_msgs::srv::AddTransform>(
    "/pm_settings_test/base_link/add", req);
  ASSERT_NE(resp, nullptr);
  EXPECT_FALSE(resp->result);
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

}  // namespace pm::settings::test