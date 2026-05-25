/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#include "pm_settings/adapters/moveit_adapter.hpp"

#include <array>
#include <functional>
#include <utility>

#include <core_utils/log.h>
#include <fmt/format.h>
#include <shape_msgs/msg/solid_primitive.hpp>

namespace pm::settings
{

namespace
{

constexpr double DEFAULT_POINT_RADIUS  = 0.01;
constexpr double DEFAULT_LINE_RADIUS   = 0.01;
constexpr double DEFAULT_WALL_SIZE     = 4.0;
constexpr double DEFAULT_WALL_Z_SIZE   = 0.01;

shape_msgs::msg::SolidPrimitive BuildPrimitive(const SceneObject & object)
{
  // Маппинг типа объекта на конфигурацию SolidPrimitive.
  // Использование std::to_array + dispatch table - паттерн, принятый в проекте,
  // позволяет добавить новый тип примитива без модификации цепочки if/else.
  using SetterFn = std::function<void(const SceneObject &, shape_msgs::msg::SolidPrimitive &)>;
  static const auto kSetters = std::to_array<std::pair<SceneObjectType, SetterFn>>({
    {SceneObjectType::POINT, [](const SceneObject &, shape_msgs::msg::SolidPrimitive & p) {
      p.type = shape_msgs::msg::SolidPrimitive::SPHERE;
      p.dimensions = {DEFAULT_POINT_RADIUS};
    }},
    {SceneObjectType::LINE, [](const SceneObject & o, shape_msgs::msg::SolidPrimitive & p) {
      p.type = shape_msgs::msg::SolidPrimitive::CYLINDER;
      p.dimensions = {o.dimension.x, DEFAULT_LINE_RADIUS};
    }},
    {SceneObjectType::WALL, [](const SceneObject &, shape_msgs::msg::SolidPrimitive & p) {
      p.type = shape_msgs::msg::SolidPrimitive::BOX;
      p.dimensions = {DEFAULT_WALL_SIZE, DEFAULT_WALL_SIZE, DEFAULT_WALL_Z_SIZE};
    }},
    {SceneObjectType::CUBE, [](const SceneObject & o, shape_msgs::msg::SolidPrimitive & p) {
      p.type = shape_msgs::msg::SolidPrimitive::BOX;
      p.dimensions = {o.dimension.x, o.dimension.y, o.dimension.z};
    }},
    {SceneObjectType::CYLINDER, [](const SceneObject & o, shape_msgs::msg::SolidPrimitive & p) {
      p.type = shape_msgs::msg::SolidPrimitive::CYLINDER;
      p.dimensions = {o.dimension.x, o.dimension.y};
    }},
    {SceneObjectType::SPHERE, [](const SceneObject & o, shape_msgs::msg::SolidPrimitive & p) {
      p.type = shape_msgs::msg::SolidPrimitive::SPHERE;
      p.dimensions = {o.dimension.x};
    }},
  });

  shape_msgs::msg::SolidPrimitive primitive;
  for (const auto & [type, setter] : kSetters) {
    if (type == object.type) {
      setter(object, primitive);
      return primitive;
    }
  }

  LOG_E << fmt::format(
    "MoveItAdapter: unknown object type {} for '{}'",
    static_cast<int>(object.type), object.name);
  return primitive;
}

}  // namespace

MoveItAdapter::MoveItAdapter(const std::shared_ptr<rclcpp::Node> & node)
: m_node(node)
{
  m_planning_scene_pub = m_node->create_publisher<moveit_msgs::msg::PlanningScene>(
    "/planning_scene",
    rclcpp::SystemDefaultsQoS().transient_local());

  // PlanningSceneInterface пытается подключиться к move_group в конструкторе.
  // Здесь оборачиваем в try/catch на случай отсутствия запущенного move_group
  // (например, в unit-тестах или при изолированной диагностике).
  try {
    m_planning_scene_iface =
      std::make_unique<moveit::planning_interface::PlanningSceneInterface>();
  } catch (const std::exception & e) {
    LOG_W << fmt::format(
      "MoveItAdapter: PlanningSceneInterface init failed: {}", e.what());
  }
}

MoveItAdapter::~MoveItAdapter() = default;

bool MoveItAdapter::UpdateTCPTransform(
  const geometry_msgs::msg::TransformStamped & transform)
{
  if (!m_planning_scene_pub) {
    LOG_W << "MoveItAdapter: planning scene publisher is null";
    return false;
  }

  moveit_msgs::msg::PlanningScene diff;
  diff.is_diff = true;

  auto stamped = transform;
  stamped.header.stamp = m_node->now();
  diff.fixed_frame_transforms = {stamped};

  m_planning_scene_pub->publish(diff);

  LOG_I << fmt::format(
    "MoveItAdapter: TCP transform applied {} -> {}",
    stamped.header.frame_id, stamped.child_frame_id);
  return true;
}

bool MoveItAdapter::UpdateBaseLinkTransform(
  const geometry_msgs::msg::TransformStamped & transform)
{
  if (!m_planning_scene_pub) {
    LOG_W << "MoveItAdapter: planning scene publisher is null";
    return false;
  }

  moveit_msgs::msg::PlanningScene diff;
  diff.is_diff = true;

  auto stamped = transform;
  stamped.header.stamp = m_node->now();
  stamped.header.frame_id = std::string(frames::WORLD);
  stamped.child_frame_id = std::string(frames::BASE);
  diff.fixed_frame_transforms = {stamped};

  m_planning_scene_pub->publish(diff);

  LOG_I << fmt::format(
    "MoveItAdapter: base_link transform applied: pos=({:.3f}, {:.3f}, {:.3f})",
    stamped.transform.translation.x,
    stamped.transform.translation.y,
    stamped.transform.translation.z);
  return true;
}

bool MoveItAdapter::AddSceneObject(const SceneObject & object)
{
  const auto collision = BuildCollisionObject(object);

  moveit_msgs::msg::PlanningScene scene;
  scene.is_diff = true;
  scene.robot_state.is_diff = true;
  scene.world.collision_objects.push_back(collision);

  if (m_planning_scene_pub) {
    m_planning_scene_pub->publish(scene);
  }

  if (m_planning_scene_iface) {
    try {
      m_planning_scene_iface->applyCollisionObject(collision);
    } catch (const std::exception & e) {
      LOG_W << fmt::format(
        "MoveItAdapter: applyCollisionObject failed for '{}': {}",
        object.name, e.what());
    }
  }

  return true;
}

bool MoveItAdapter::RemoveSceneObject(const std::string & object_id)
{
  if (m_planning_scene_iface) {
    try {
      m_planning_scene_iface->removeCollisionObjects({object_id});
      return true;
    } catch (const std::exception & e) {
      LOG_E << fmt::format(
        "MoveItAdapter: removeCollisionObjects failed for '{}': {}",
        object_id, e.what());
      return false;
    }
  }
  return false;
}

moveit_msgs::msg::PlanningScene MoveItAdapter::GetPlanningScene() const
{
  // Метод-заглушка для будущей интеграции с /get_planning_scene service.
  return moveit_msgs::msg::PlanningScene{};
}

bool MoveItAdapter::PublishFullScene(const std::vector<SceneObject> & objects)
{
  if (!m_planning_scene_pub) {
    return false;
  }

  std::vector<moveit_msgs::msg::CollisionObject> collisions;
  collisions.reserve(objects.size());
  for (const auto & obj : objects) {
    collisions.push_back(BuildCollisionObject(obj));
  }

  moveit_msgs::msg::PlanningScene scene;
  scene.is_diff = true;
  scene.robot_state.is_diff = true;
  scene.world.collision_objects = collisions;

  m_planning_scene_pub->publish(scene);

  if (m_planning_scene_iface && !collisions.empty()) {
    try {
      m_planning_scene_iface->applyCollisionObjects(collisions);
    } catch (const std::exception & e) {
      LOG_W << fmt::format("MoveItAdapter: applyCollisionObjects failed: {}", e.what());
    }
  }

  return true;
}

moveit_msgs::msg::CollisionObject MoveItAdapter::BuildCollisionObject(
  const SceneObject & object) const
{
  moveit_msgs::msg::CollisionObject collision;
  collision.header.frame_id = object.parent_frame.empty()
    ? std::string(frames::BASE)
    : object.parent_frame;
  collision.header.stamp = m_node->now();
  collision.id = object.name;

  collision.primitives.push_back(BuildPrimitive(object));
  collision.primitive_poses.push_back(object.pose);
  collision.operation = moveit_msgs::msg::CollisionObject::ADD;
  return collision;
}

}  // namespace pm::settings
