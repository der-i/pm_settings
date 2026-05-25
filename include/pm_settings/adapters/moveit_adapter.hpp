/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#ifndef PM_SETTINGS__ADAPTERS__MOVEIT_ADAPTER_HPP_
#define PM_SETTINGS__ADAPTERS__MOVEIT_ADAPTER_HPP_

#include <memory>
#include <string>
#include <vector>

#include <geometry_msgs/msg/transform_stamped.hpp>
#include <moveit/planning_scene_interface/planning_scene_interface.hpp>
#include <moveit_msgs/msg/collision_object.hpp>
#include <moveit_msgs/msg/planning_scene.hpp>
#include <rclcpp/rclcpp.hpp>

#include "pm_settings/types/configurations.hpp"

namespace pm::settings
{

/// @brief Адаптер для взаимодействия с MoveIt2.
/// @details Инкапсулирует взаимодействие с MoveIt2 через обновление
/// кинематической модели (/robot_description), управление Planning Scene
/// (/apply_planning_scene) и публикацию трансформаций.
class MoveItAdapter
{
public:
  explicit MoveItAdapter(const std::shared_ptr<rclcpp::Node> & node);
  ~MoveItAdapter();

  MoveItAdapter(const MoveItAdapter &) = delete;
  MoveItAdapter & operator=(const MoveItAdapter &) = delete;

  /// @brief Обновить трансформацию ЦТИ (flange -> tool).
  [[nodiscard]] bool UpdateTCPTransform(const geometry_msgs::msg::TransformStamped & transform);

  /// @brief Обновить трансформацию базового звена (world -> base_link).
  /// @details Публикует diff Planning Scene с полем fixed_frame_transforms,
  /// что заставляет MoveIt2 пересчитать кинематическую конфигурацию.
  [[nodiscard]] bool UpdateBaseLinkTransform(
    const geometry_msgs::msg::TransformStamped & transform);

  /// @brief Применить объект сцены планирования.
  [[nodiscard]] bool AddSceneObject(const SceneObject & object);

  /// @brief Удалить объект сцены по идентификатору.
  [[nodiscard]] bool RemoveSceneObject(const std::string & object_id);

  /// @brief Получить текущее состояние Planning Scene.
  [[nodiscard]] moveit_msgs::msg::PlanningScene GetPlanningScene() const;

  /// @brief Принудительно опубликовать весь набор известных объектов сцены.
  [[nodiscard]] bool PublishFullScene(const std::vector<SceneObject> & objects);

private:
  [[nodiscard]] moveit_msgs::msg::CollisionObject BuildCollisionObject(
    const SceneObject & object) const;

  std::shared_ptr<rclcpp::Node> m_node;
  std::shared_ptr<rclcpp::Publisher<moveit_msgs::msg::PlanningScene>> m_planning_scene_pub;
  std::unique_ptr<moveit::planning_interface::PlanningSceneInterface> m_planning_scene_iface;
};

}  // namespace pm::settings

#endif  // PM_SETTINGS__ADAPTERS__MOVEIT_ADAPTER_HPP_
