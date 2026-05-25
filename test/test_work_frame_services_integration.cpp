/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#include <core_msgs/srv/add_work_frame.hpp>
#include <core_msgs/srv/apply_work_frame.hpp>
#include <core_msgs/srv/delete_work_frame.hpp>
#include <core_msgs/srv/get_work_frame.hpp>
#include <core_msgs/srv/list_work_frame.hpp>
#include <std_srvs/srv/trigger.hpp>

#include "integration_fixture.hpp"

namespace pm::settings::test
{

using WorkFrameIntegrationTest = IntegrationFixture;

TEST_F(WorkFrameIntegrationTest, AddWorkFrame)
{
  auto req = std::make_shared<core_msgs::srv::AddWorkFrame::Request>();
  req->name = "table_frame";
  req->apply = true;
  const auto resp = CallService<core_msgs::srv::AddWorkFrame>(
    "/pm_settings_test/work_frame/add", req);
  ASSERT_NE(resp, nullptr);
  EXPECT_TRUE(resp->result);
}

TEST_F(WorkFrameIntegrationTest, ApplyWorkFrame)
{
  auto req = std::make_shared<core_msgs::srv::ApplyWorkFrame::Request>();
  req->name = "table_frame";
  const auto resp = CallService<core_msgs::srv::ApplyWorkFrame>(
    "/pm_settings_test/work_frame/apply", req);
  ASSERT_NE(resp, nullptr);
  EXPECT_TRUE(resp->result);
}

TEST_F(WorkFrameIntegrationTest, DeleteWorkFrame)
{
  auto req = std::make_shared<core_msgs::srv::DeleteWorkFrame::Request>();
  req->name = "table_frame";
  const auto resp = CallService<core_msgs::srv::DeleteWorkFrame>(
    "/pm_settings_test/work_frame/delete", req);
  ASSERT_NE(resp, nullptr);
  EXPECT_TRUE(resp->result);
}

TEST_F(WorkFrameIntegrationTest, GetWorkFrameList)
{
  auto req = std::make_shared<core_msgs::srv::ListWorkFrame::Request>();
  const auto resp = CallService<core_msgs::srv::ListWorkFrame>(
    "/pm_settings_test/work_frame/get_list", req);
  ASSERT_NE(resp, nullptr);
  EXPECT_TRUE(resp->result);
}

TEST_F(WorkFrameIntegrationTest, GetCurrentWorkFrame)
{
  auto req = std::make_shared<core_msgs::srv::GetWorkFrame::Request>();
  const auto resp = CallService<core_msgs::srv::GetWorkFrame>(
    "/pm_settings_test/work_frame/get_current", req);
  ASSERT_NE(resp, nullptr);
  EXPECT_TRUE(resp->result);
}

TEST_F(WorkFrameIntegrationTest, ResetWorkFrame)
{
  auto req = std::make_shared<std_srvs::srv::Trigger::Request>();
  const auto resp = CallService<std_srvs::srv::Trigger>(
    "/pm_settings_test/work_frame/reset", req);
  ASSERT_NE(resp, nullptr);
  EXPECT_TRUE(resp->success);
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

}  // namespace pm::settings::test