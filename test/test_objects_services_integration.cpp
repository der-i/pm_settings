/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#include <core_msgs/srv/add_object.hpp>
#include <core_msgs/srv/delete_object.hpp>
#include <core_msgs/srv/get_object.hpp>
#include <core_msgs/srv/get_object_names.hpp>
#include <std_srvs/srv/trigger.hpp>

#include "integration_fixture.hpp"

namespace pm::settings::test
{

namespace
{

core_msgs::srv::AddObject::Request::SharedPtr MakeAddObjectRequest(
  const std::string & name,
  std::uint8_t type = core_msgs::srv::AddObject::Request::TYPE_CUBE)
{
  auto req = std::make_shared<core_msgs::srv::AddObject::Request>();
  req->name = name;
  req->parent_frame = "base_link";
  req->type = type;
  req->pose.orientation.w = 1.0;
  req->dimension.x = 0.1;
  req->dimension.y = 0.1;
  req->dimension.z = 0.1;
  return req;
}

}  // namespace

using ObjectsIntegrationTest = IntegrationFixture;

TEST_F(ObjectsIntegrationTest, AddObject)
{
  const auto resp = CallService<core_msgs::srv::AddObject>(
    "/pm_settings_test/objects/add", MakeAddObjectRequest("table"));
  ASSERT_NE(resp, nullptr);
  EXPECT_TRUE(resp->result);
}

TEST_F(ObjectsIntegrationTest, AddObjectWithZeroDimensionFails)
{
  auto req = MakeAddObjectRequest("bad");
  req->dimension.y = 0.0;
  const auto resp = CallService<core_msgs::srv::AddObject>(
    "/pm_settings_test/objects/add", req);
  ASSERT_NE(resp, nullptr);
  EXPECT_FALSE(resp->result);
}

TEST_F(ObjectsIntegrationTest, GetObjectByName)
{
  ASSERT_NE(CallService<core_msgs::srv::AddObject>(
    "/pm_settings_test/objects/add", MakeAddObjectRequest("table")), nullptr);

  auto get_req = std::make_shared<core_msgs::srv::GetObject::Request>();
  get_req->name = "table";
  const auto resp = CallService<core_msgs::srv::GetObject>(
    "/pm_settings_test/objects/get_info", get_req);
  ASSERT_NE(resp, nullptr);
  EXPECT_TRUE(resp->result);
  EXPECT_EQ(resp->parent_frame, "base_link");
}

TEST_F(ObjectsIntegrationTest, GetObjectNotFound)
{
  auto req = std::make_shared<core_msgs::srv::GetObject::Request>();
  req->name = "ghost";
  const auto resp = CallService<core_msgs::srv::GetObject>(
    "/pm_settings_test/objects/get_info", req);
  ASSERT_NE(resp, nullptr);
  EXPECT_FALSE(resp->result);
}

TEST_F(ObjectsIntegrationTest, DeleteObject)
{
  ASSERT_NE(CallService<core_msgs::srv::AddObject>(
    "/pm_settings_test/objects/add", MakeAddObjectRequest("table")), nullptr);

  auto del_req = std::make_shared<core_msgs::srv::DeleteObject::Request>();
  del_req->name = "table";
  const auto resp = CallService<core_msgs::srv::DeleteObject>(
    "/pm_settings_test/objects/remove", del_req);
  ASSERT_NE(resp, nullptr);
  EXPECT_TRUE(resp->result);
}

TEST_F(ObjectsIntegrationTest, ListReturnsAddedNames)
{
  ASSERT_NE(CallService<core_msgs::srv::AddObject>(
    "/pm_settings_test/objects/add", MakeAddObjectRequest("a")), nullptr);
  ASSERT_NE(CallService<core_msgs::srv::AddObject>(
    "/pm_settings_test/objects/add", MakeAddObjectRequest("b")), nullptr);

  auto req = std::make_shared<core_msgs::srv::GetObjectNames::Request>();
  const auto resp = CallService<core_msgs::srv::GetObjectNames>(
    "/pm_settings_test/objects/list", req);
  ASSERT_NE(resp, nullptr);
  EXPECT_EQ(resp->object_names.size(), 2u);
}

TEST_F(ObjectsIntegrationTest, UpsertUpdatesExisting)
{
  ASSERT_NE(CallService<core_msgs::srv::AddObject>(
    "/pm_settings_test/objects/add", MakeAddObjectRequest("t")), nullptr);

  auto req = MakeAddObjectRequest("t");
  req->dimension.x = 1.0;
  ASSERT_NE(CallService<core_msgs::srv::AddObject>(
    "/pm_settings_test/objects/add", req), nullptr);

  auto names_req = std::make_shared<core_msgs::srv::GetObjectNames::Request>();
  const auto names_resp = CallService<core_msgs::srv::GetObjectNames>(
    "/pm_settings_test/objects/list", names_req);
  ASSERT_NE(names_resp, nullptr);
  EXPECT_EQ(names_resp->object_names.size(), 1u);
}

TEST_F(ObjectsIntegrationTest, AddPointWithoutDimensions)
{
  auto req = MakeAddObjectRequest("p",
    core_msgs::srv::AddObject::Request::TYPE_POINT);
  req->dimension = {};
  const auto resp = CallService<core_msgs::srv::AddObject>(
    "/pm_settings_test/objects/add", req);
  ASSERT_NE(resp, nullptr);
  EXPECT_TRUE(resp->result);
}

TEST_F(ObjectsIntegrationTest, ForceUpdateScene)
{
  auto req = std::make_shared<std_srvs::srv::Trigger::Request>();
  const auto resp = CallService<std_srvs::srv::Trigger>(
    "/pm_settings_test/objects/update_scene", req);
  ASSERT_NE(resp, nullptr);
  EXPECT_TRUE(resp->success);
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

}  // namespace pm::settings::test