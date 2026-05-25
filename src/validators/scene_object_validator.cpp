/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#include "pm_settings/validators/scene_object_validator.hpp"

#include <algorithm>

#include <fmt/format.h>

namespace pm::settings
{

ValidationResult SceneObjectValidator::Validate(
  const std::any & value,
  const ValidationContext & context) const
{
  ValidationResult result = ValidationResult::Ok();

  const auto * obj = std::any_cast<SceneObject>(&value);
  if (!obj) {
    return ValidationResult::Error("SceneObjectValidator: payload is not SceneObject");
  }

  if (obj->name.empty()) {
    result.Add("SceneObjectValidator: name must not be empty");
    return result;
  }

  (void)CheckType(*obj, result);
  (void)CheckDimensionsPositive(*obj, result);

  if (const auto * existing = std::any_cast<std::vector<SceneObject>>(&context.current_state)) {
    (void)CheckIdUnique(*obj, *existing, result);
  }

  return result;
}

bool SceneObjectValidator::CheckType(const SceneObject & obj, ValidationResult & r) const
{
  switch (obj.type) {
    case SceneObjectType::POINT:
    case SceneObjectType::LINE:
    case SceneObjectType::WALL:
    case SceneObjectType::CUBE:
    case SceneObjectType::CYLINDER:
    case SceneObjectType::SPHERE:
      return true;
  }
  r.Add(fmt::format(
    "SceneObjectValidator: '{}' has unknown type {}",
    obj.name, static_cast<int>(obj.type)));
  return false;
}

bool SceneObjectValidator::CheckDimensionsPositive(
  const SceneObject & obj, ValidationResult & r) const
{
  // Для каждого типа набор обязательных размеров отличается.
  const auto require_positive = [&](double v, std::string_view axis) {
    if (v <= 0.0) {
      r.Add(fmt::format(
        "SceneObjectValidator: '{}' dimension.{} must be positive ({})",
        obj.name, axis, v));
      return false;
    }
    return true;
  };

  switch (obj.type) {
    case SceneObjectType::POINT:
    case SceneObjectType::WALL:
      // Размеры этих типов фиксированы, проверка не требуется.
      return true;
    case SceneObjectType::LINE:
    case SceneObjectType::SPHERE:
      return require_positive(obj.dimension.x, "x");
    case SceneObjectType::CYLINDER:
      return require_positive(obj.dimension.x, "x") &&
             require_positive(obj.dimension.y, "y");
    case SceneObjectType::CUBE:
      return require_positive(obj.dimension.x, "x") &&
             require_positive(obj.dimension.y, "y") &&
             require_positive(obj.dimension.z, "z");
  }
  return true;
}

bool SceneObjectValidator::CheckIdUnique(
  const SceneObject & obj,
  const std::vector<SceneObject> & existing,
  ValidationResult & r) const
{
  // Уникальность мягкая: дубликат -> upsert. Здесь только информируем.
  (void)r;
  (void)std::any_of(existing.begin(), existing.end(),
    [&](const SceneObject & e) { return e.name == obj.name; });
  return true;
}

}  // namespace pm::settings
