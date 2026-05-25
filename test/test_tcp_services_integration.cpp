/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#include <core_msgs/srv/add_tcp.hpp>
#include <core_msgs/srv/apply_tcp.hpp>
#include <core_msgs/srv/delete_tcp.hpp>
#include <std_srvs/srv/trigger.hpp>

#include "integration_fixture.hpp"

namespace pm::settings::test
{

namespace
{

core_msgs::srv::AddTCP::Request::SharedPtr MakeAddTCPRequest(
  const std::string & name, bool apply = true, double z = 0.15)
{
  auto req = std::make_shared<core_msgs::srv::AddTCP::Request>();
  req->name = name;
  req->pose.position.z = z;
  req->pose.orientation.w = 1.0;
  req->apply = apply;
  return req;
}

}  // namespace

using TcpIntegrationTest = IntegrationFixture;

TEST_F(TcpIntegrationTest, AddTCPSucceeds)
{
  const auto response = CallService<core_msgs::srv::AddTCP>(
    "/pm_settings_test/tcp/add", MakeAddTCPRequest("gripper"));
  ASSERT_NE(response, nullptr);
  EXPECT_TRUE(response->result);
}

TEST_F(TcpIntegrationTest, AddTCPThenAppearsInList)
{
  ASSERT_NE(CallService<core_msgs::srv::AddTCP>(
    "/pm_settings_test/tcp/add", MakeAddTCPRequest("gripper")), nullptr);

  auto list_req = std::make_shared<std_srvs::srv::Trigger::Request>();
  const auto list_resp = CallService<std_srvs::srv::Trigger>(
    "/pm_settings_test/tcp/get_list", list_req);
  ASSERT_NE(list_resp, nullptr);
  EXPECT_TRUE(list_resp->success);
  EXPECT_NE(list_resp->message.find("gripper"), std::string::npos);
}

TEST_F(TcpIntegrationTest, AddTCPUpsertUpdatesExisting)
{
  ASSERT_NE(CallService<core_msgs::srv::AddTCP>(
    "/pm_settings_test/tcp/add", MakeAddTCPRequest("gripper", true, 0.1)), nullptr);

  const auto second = CallService<core_msgs::srv::AddTCP>(
    "/pm_settings_test/tcp/add", MakeAddTCPRequest("gripper", true, 0.2));
  ASSERT_NE(second, nullptr);
  EXPECT_TRUE(second->result);
  EXPECT_EQ(m_tcp_handler->GetList().size(), 1u);
}

TEST_F(TcpIntegrationTest, ApplyTCPByNameSucceeds)
{
  ASSERT_NE(CallService<core_msgs::srv::AddTCP>(
    "/pm_settings_test/tcp/add", MakeAddTCPRequest("gripper", false)), nullptr);

  auto apply_req = std::make_shared<core_msgs::srv::ApplyTCP::Request>();
  apply_req->name = "gripper";
  const auto resp = CallService<core_msgs::srv::ApplyTCP>(
    "/pm_settings_test/tcp/apply", apply_req);
  ASSERT_NE(resp, nullptr);
  EXPECT_TRUE(resp->result);
}

TEST_F(TcpIntegrationTest, ApplyTCPUnknownNameFails)
{
  auto apply_req = std::make_shared<core_msgs::srv::ApplyTCP::Request>();
  apply_req->name = "does_not_exist";
  const auto resp = CallService<core_msgs::srv::ApplyTCP>(
    "/pm_settings_test/tcp/apply", apply_req);
  ASSERT_NE(resp, nullptr);
  EXPECT_FALSE(resp->result);
}

TEST_F(TcpIntegrationTest, DeleteTCP)
{
  ASSERT_NE(CallService<core_msgs::srv::AddTCP>(
    "/pm_settings_test/tcp/add", MakeAddTCPRequest("to_delete")), nullptr);

  auto del_req = std::make_shared<core_msgs::srv::DeleteTCP::Request>();
  del_req->name = "to_delete";
  del_req->reset_current = true;
  const auto resp = CallService<core_msgs::srv::DeleteTCP>(
    "/pm_settings_test/tcp/delete", del_req);
  ASSERT_NE(resp, nullptr);
  EXPECT_TRUE(resp->result);
  EXPECT_TRUE(m_tcp_handler->GetList().empty());
}

TEST_F(TcpIntegrationTest, DeleteNonExistentTCPFails)
{
  auto del_req = std::make_shared<core_msgs::srv::DeleteTCP::Request>();
  del_req->name = "ghost";
  del_req->reset_current = false;
  const auto resp = CallService<core_msgs::srv::DeleteTCP>(
    "/pm_settings_test/tcp/delete", del_req);
  ASSERT_NE(resp, nullptr);
  EXPECT_FALSE(resp->result);
}

TEST_F(TcpIntegrationTest, GetCurrentReturnsNothingInitially)
{
  auto req = std::make_shared<std_srvs::srv::Trigger::Request>();
  const auto resp = CallService<std_srvs::srv::Trigger>(
    "/pm_settings_test/tcp/get_current", req);
  ASSERT_NE(resp, nullptr);
  EXPECT_FALSE(resp->success);
}

TEST_F(TcpIntegrationTest, ResetTCP)
{
  ASSERT_NE(CallService<core_msgs::srv::AddTCP>(
    "/pm_settings_test/tcp/add", MakeAddTCPRequest("gripper")), nullptr);

  auto reset_req = std::make_shared<std_srvs::srv::Trigger::Request>();
  const auto resp = CallService<std_srvs::srv::Trigger>(
    "/pm_settings_test/tcp/reset", reset_req);
  ASSERT_NE(resp, nullptr);
  EXPECT_TRUE(resp->success);
}

TEST_F(TcpIntegrationTest, RejectsUnnormalizedQuaternion)
{
  auto req = MakeAddTCPRequest("bad_quat");
  req->pose.orientation.w = 2.0;  // non-normalized
  const auto resp = CallService<core_msgs::srv::AddTCP>(
    "/pm_settings_test/tcp/add", req);
  ASSERT_NE(resp, nullptr);
  EXPECT_FALSE(resp->result);
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

}  // namespace pm::settings::test