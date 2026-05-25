/*!
 * \date 23.04.2026
 * \author Ilya Derevnin <i.derevnin@promo-bot.ru>
 * \copyright Copyright (C) 2015-2026 Promobot LLC. All rights reserved.
 */

#include <filesystem>

#include <gtest/gtest.h>

#include "pm_settings/core/settings_manager.hpp"
#include "pm_settings/core/validator_registry.hpp"
#include "pm_settings/persistence/config_file_manager.hpp"

using namespace pm::settings;

namespace
{

/// Минимальный фейковый обработчик для проверки четырёхэтапного алгоритма.
class FakeHandler : public ISettingsHandler
{
public:
  explicit FakeHandler(ParamType t) : m_type(t) {}

  OperationResult Apply(const std::any & value) override
  {
    ++apply_calls;
    if (fail_apply) {
      return OperationResult::Fail(ResultCode::RUNTIME_UPDATE_FAILED, "forced apply failure");
    }
    m_previous = m_current;
    m_current = value;
    return OperationResult::Ok("applied");
  }

  OperationResult Rollback() override
  {
    ++rollback_calls;
    if (fail_rollback) {
      return OperationResult::Fail(ResultCode::ROLLBACK_FAILED, "forced rollback failure");
    }
    m_current = m_previous;
    return OperationResult::Ok("rolled back");
  }

  std::any GetCurrent() const override { return m_current; }
  ParamType GetType() const noexcept override { return m_type; }

  int apply_calls = 0;
  int rollback_calls = 0;
  bool fail_apply = false;
  bool fail_rollback = false;

private:
  ParamType m_type;
  std::any m_current;
  std::any m_previous;
};

/// Валидатор, который отклоняет всё (для проверки того, что Apply не вызывается).
class RejectingValidator : public IValidator
{
public:
  ValidationResult Validate(const std::any &, const ValidationContext &) const override
  {
    return ValidationResult::Error("always rejects");
  }
};

/// Валидатор-всё-пропускалка.
class PassingValidator : public IValidator
{
public:
  ValidationResult Validate(const std::any &, const ValidationContext &) const override
  {
    return ValidationResult::Ok();
  }
};

std::shared_ptr<ConfigFileManager> MakeConfigManager()
{
  const auto tmp = std::filesystem::temp_directory_path() /
    ("pm_mgr_test_" + std::to_string(std::hash<const void *>{}(&tmp)));
  std::filesystem::create_directories(tmp);
  return std::make_shared<ConfigFileManager>(tmp);
}

}  // namespace

TEST(SettingsManagerTest, ProcessAppliesValidParameter)
{
  auto registry = std::make_shared<ValidatorRegistry>();
  registry->Register(ParamType::TCP, std::make_shared<PassingValidator>());

  auto manager = SettingsManager(registry, MakeConfigManager());
  auto handler = std::make_shared<FakeHandler>(ParamType::TCP);
  manager.RegisterHandler(handler);

  const auto result = manager.Process(ParamType::TCP, std::any{std::string{"payload"}});
  EXPECT_TRUE(result.success);
  EXPECT_EQ(handler->apply_calls, 1);
  EXPECT_EQ(handler->rollback_calls, 0);
}

TEST(SettingsManagerTest, ValidationFailureSkipsApply)
{
  auto registry = std::make_shared<ValidatorRegistry>();
  registry->Register(ParamType::TCP, std::make_shared<RejectingValidator>());

  auto manager = SettingsManager(registry, MakeConfigManager());
  auto handler = std::make_shared<FakeHandler>(ParamType::TCP);
  manager.RegisterHandler(handler);

  const auto result = manager.Process(ParamType::TCP, std::any{});
  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.code, ResultCode::VALIDATION_FAILED);
  EXPECT_EQ(handler->apply_calls, 0);
}

TEST(SettingsManagerTest, ApplyFailureTriggersRollback)
{
  auto registry = std::make_shared<ValidatorRegistry>();
  registry->Register(ParamType::TCP, std::make_shared<PassingValidator>());

  auto manager = SettingsManager(registry, MakeConfigManager());
  auto handler = std::make_shared<FakeHandler>(ParamType::TCP);
  handler->fail_apply = true;
  manager.RegisterHandler(handler);

  const auto result = manager.Process(ParamType::TCP, std::any{});
  EXPECT_FALSE(result.success);
  EXPECT_EQ(handler->apply_calls, 1);
  EXPECT_EQ(handler->rollback_calls, 1);
}

TEST(SettingsManagerTest, RollbackFailureReportedSeparately)
{
  auto registry = std::make_shared<ValidatorRegistry>();
  registry->Register(ParamType::TCP, std::make_shared<PassingValidator>());

  auto manager = SettingsManager(registry, MakeConfigManager());
  auto handler = std::make_shared<FakeHandler>(ParamType::TCP);
  handler->fail_apply = true;
  handler->fail_rollback = true;
  manager.RegisterHandler(handler);

  const auto result = manager.Process(ParamType::TCP, std::any{});
  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.code, ResultCode::ROLLBACK_FAILED);
}

TEST(SettingsManagerTest, NoHandlerReturnsInternalError)
{
  auto registry = std::make_shared<ValidatorRegistry>();
  registry->Register(ParamType::TCP, std::make_shared<PassingValidator>());

  auto manager = SettingsManager(registry, MakeConfigManager());
  const auto result = manager.Process(ParamType::TCP, std::any{});
  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.code, ResultCode::INTERNAL_ERROR);
}

TEST(SettingsManagerTest, MissingValidatorSkipsValidation)
{
  auto registry = std::make_shared<ValidatorRegistry>();
  // не регистрируем валидатор
  auto manager = SettingsManager(registry, MakeConfigManager());
  auto handler = std::make_shared<FakeHandler>(ParamType::SCENE_OBJECT);
  manager.RegisterHandler(handler);

  const auto result = manager.Process(ParamType::SCENE_OBJECT, std::any{});
  EXPECT_TRUE(result.success);
  EXPECT_EQ(handler->apply_calls, 1);
}

TEST(SettingsManagerTest, GetCurrentDelegatesToHandler)
{
  auto registry = std::make_shared<ValidatorRegistry>();
  auto manager = SettingsManager(registry, MakeConfigManager());
  auto handler = std::make_shared<FakeHandler>(ParamType::TCP);
  manager.RegisterHandler(handler);

  (void)manager.Process(ParamType::TCP, std::any{std::string{"marker"}});
  const auto current = manager.GetCurrent(ParamType::TCP);
  const auto * str = std::any_cast<std::string>(&current);
  ASSERT_NE(str, nullptr);
  EXPECT_EQ(*str, "marker");
}