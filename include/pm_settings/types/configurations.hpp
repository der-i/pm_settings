/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#ifndef PM_SETTINGS__TYPES__CONFIGURATIONS_HPP_
#define PM_SETTINGS__TYPES__CONFIGURATIONS_HPP_

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <geometry_msgs/msg/pose.hpp>

#include "pm_settings/types/common.hpp"

namespace pm::settings
{

/// @brief Тип примитива объекта сцены (соответствует core_msgs/srv/AddObject).
enum class SceneObjectType : std::uint8_t
{
  POINT    = 0,
  LINE     = 1,
  WALL     = 2,
  CUBE     = 3,
  CYLINDER = 4,
  SPHERE   = 5
};

/// @brief Конфигурация центральной точки инструмента.
struct TCPConfiguration
{
  std::string name;
  geometry_msgs::msg::Pose pose;
};

/// @brief Конфигурация монтажа манипулятора (трансформация базового звена).
struct MountingConfiguration
{
  std::string name;
  geometry_msgs::msg::Pose pose;
};

/// @brief Конфигурация рабочей системы координат.
struct WorkFrameConfiguration
{
  std::string name;
  geometry_msgs::msg::Pose pose;
};

/// @brief Пределы одного сустава.
struct JointLimit
{
  std::string name;
  double min_position{0.0};
  double max_position{0.0};
  double max_velocity{0.0};
  double max_acceleration{0.0};
};

/// @brief Пределы всех суставов манипулятора.
struct JointLimitsConfiguration
{
  std::vector<JointLimit> limits;
};

/// @brief Домашнее положение манипулятора (углы для каждого сустава).
struct HomePositionConfiguration
{
  std::vector<std::string> joint_names;
  std::vector<double> positions;
};

/// @brief Размеры объекта сцены. Интерпретация зависит от типа.
struct ObjectDimension
{
  double x{0.0};
  double y{0.0};
  double z{0.0};
};

/// @brief Описание объекта сцены планирования.
struct SceneObject
{
  std::string name;
  std::string parent_frame{std::string(frames::BASE)};
  SceneObjectType type{SceneObjectType::POINT};
  geometry_msgs::msg::Pose pose;
  ObjectDimension dimension;
};

}  // namespace pm::settings

#endif  // PM_SETTINGS__TYPES__CONFIGURATIONS_HPP_
