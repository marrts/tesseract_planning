/**
 * @file teasseract_task_composer_plugin_factories_unit.cpp
 * @brief
 *
 * @author Levi Armstrong
 * @date Feb 17, 2023
 * @version TODO
 * @bug No known bugs
 *
 * @copyright Copyright (c) 2023, Levi Armstrong
 *
 * @par License
 * Software License Agreement (Apache License)
 * @par
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * @par
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <tesseract_common/macros.h>
TESSERACT_COMMON_IGNORE_WARNINGS_PUSH
#include <gtest/gtest.h>
TESSERACT_COMMON_IGNORE_WARNINGS_POP

#include <tesseract_task_composer/core/task_composer_executor.h>
#include <tesseract_task_composer/core/task_composer_node.h>
#include <tesseract_task_composer/core/task_composer_plugin_factory.h>
#include <tesseract_common/utils.h>
#include <tesseract_common/types.h>
#include <tesseract_common/resource_locator.h>
#include <tesseract_common/yaml_utils.h>
#include <tesseract_common/yaml_extenstions.h>

using namespace tesseract_planning;

void runTaskComposerFactoryTest(TaskComposerPluginFactory& factory, YAML::Node plugin_config)
{
  const YAML::Node& plugin_info = plugin_config[tesseract_common::TaskComposerPluginInfo::CONFIG_KEY];
  const YAML::Node& search_paths = plugin_info["search_paths"];
  const YAML::Node& search_libraries = plugin_info["search_libraries"];
  const YAML::Node& executor_plugins = plugin_info["executors"]["plugins"];
  const YAML::Node& task_plugins = plugin_info["tasks"]["plugins"];

  {
    std::set<std::string> sp = factory.getSearchPaths();
    EXPECT_EQ(sp.size(), 2);

    for (auto it = search_paths.begin(); it != search_paths.end(); ++it)
    {
      EXPECT_TRUE(sp.find(it->as<std::string>()) != sp.end());
    }
  }

  {
    std::set<std::string> sl = factory.getSearchLibraries();
    EXPECT_EQ(sl.size(), 3);

    for (auto it = search_libraries.begin(); it != search_libraries.end(); ++it)
    {
      EXPECT_TRUE(sl.find(it->as<std::string>()) != sl.end());
    }
  }

  EXPECT_EQ(executor_plugins.size(), 1);
  for (auto cm_it = executor_plugins.begin(); cm_it != executor_plugins.end(); ++cm_it)
  {
    auto name = cm_it->first.as<std::string>();

    TaskComposerExecutor::UPtr cm = factory.createTaskComposerExecutor(name);
    EXPECT_TRUE(cm != nullptr);
  }
#ifdef TESSERACT_TASK_COMPOSER_HAS_TRAJOPT_IFOPT
  EXPECT_EQ(task_plugins.size(), 36);
#else
  EXPECT_EQ(task_plugins.size(), 32);
#endif
  for (auto cm_it = task_plugins.begin(); cm_it != task_plugins.end(); ++cm_it)
  {
    auto name = cm_it->first.as<std::string>();

    TaskComposerNode::UPtr cm = factory.createTaskComposerNode(name);
    EXPECT_TRUE(cm != nullptr);
  }

  factory.saveConfig(std::filesystem::path(tesseract_common::getTempPath()) / "task_composer_plugins_export.yaml");

  // Failures
  {
    TaskComposerExecutor::UPtr cm = factory.createTaskComposerExecutor("DoesNotExist");
    EXPECT_TRUE(cm == nullptr);
  }
  {
    TaskComposerNode::UPtr cm = factory.createTaskComposerNode("DoesNotExist");
    EXPECT_TRUE(cm == nullptr);
  }
  {
    tesseract_common::PluginInfo plugin_info;
    plugin_info.class_name = "DoesNotExistFactory";
    TaskComposerExecutor::UPtr cm = factory.createTaskComposerExecutor("DoesNotExist", plugin_info);
    EXPECT_TRUE(cm == nullptr);
  }
  {
    tesseract_common::PluginInfo plugin_info;
    plugin_info.class_name = "DoesNotExistFactory";
    TaskComposerNode::UPtr cm = factory.createTaskComposerNode("DoesNotExist", plugin_info);
    EXPECT_TRUE(cm == nullptr);
  }
}

TEST(TesseractTaskComposerFactoryUnit, LoadAndExportPluginTest)  // NOLINT
{
  tesseract_common::GeneralResourceLocator locator;
  {  // File Path Construction
#ifdef TESSERACT_TASK_COMPOSER_HAS_TRAJOPT_IFOPT
    std::filesystem::path config_path(std::string(TESSERACT_TASK_COMPOSER_DIR) + "/config/"
                                                                                 "task_composer_plugins.yaml");
#else
    std::filesystem::path config_path(std::string(TESSERACT_TASK_COMPOSER_DIR) + "/config/"
                                                                                 "task_composer_plugins_no_"
                                                                                 "trajopt_"
                                                                                 "ifopt.yaml");
#endif
    TaskComposerPluginFactory factory(config_path, locator);
    YAML::Node plugin_config = tesseract_common::loadYamlFile(config_path.string(), locator);
    runTaskComposerFactoryTest(factory, plugin_config);

    auto export_config_path = std::filesystem::path(tesseract_common::getTempPath()) / "task_composer_plugins_"
                                                                                       "export.yaml";
    TaskComposerPluginFactory check_factory(export_config_path, locator);
    runTaskComposerFactoryTest(check_factory, plugin_config);
  }

  {  // String Constructor
#ifdef TESSERACT_TASK_COMPOSER_HAS_TRAJOPT_IFOPT
    std::filesystem::path config_path(std::string(TESSERACT_TASK_COMPOSER_DIR) + "/config/"
                                                                                 "task_composer_plugins.yaml");
#else
    std::filesystem::path config_path(std::string(TESSERACT_TASK_COMPOSER_DIR) + "/config/"
                                                                                 "task_composer_plugins_no_"
                                                                                 "trajopt_"
                                                                                 "ifopt.yaml");
#endif

    TaskComposerPluginFactory factory(tesseract_common::fileToString(config_path), locator);
    YAML::Node plugin_config = YAML::LoadFile(config_path.string());
    runTaskComposerFactoryTest(factory, plugin_config);

    auto export_config_path = std::filesystem::path(tesseract_common::getTempPath()) / "task_composer_plugins_"
                                                                                       "export.yaml";
    TaskComposerPluginFactory check_factory(export_config_path, locator);
    runTaskComposerFactoryTest(check_factory, plugin_config);
  }

  {  // YAML Node Constructor
#ifdef TESSERACT_TASK_COMPOSER_HAS_TRAJOPT_IFOPT
    std::filesystem::path config_path(std::string(TESSERACT_TASK_COMPOSER_DIR) + "/config/"
                                                                                 "task_composer_plugins.yaml");
#else
    std::filesystem::path config_path(std::string(TESSERACT_TASK_COMPOSER_DIR) + "/config/"
                                                                                 "task_composer_plugins_no_"
                                                                                 "trajopt_"
                                                                                 "ifopt.yaml");
#endif

    YAML::Node plugin_config = YAML::LoadFile(config_path.string());
    TaskComposerPluginFactory factory(plugin_config, locator);
    runTaskComposerFactoryTest(factory, plugin_config);

    auto export_config_path = std::filesystem::path(tesseract_common::getTempPath()) / "task_composer_plugins_"
                                                                                       "export.yaml";
    TaskComposerPluginFactory check_factory(export_config_path, locator);
    runTaskComposerFactoryTest(check_factory, plugin_config);
  }

  // TaskComposerPluginInfo Constructor
  {
#ifdef TESSERACT_TASK_COMPOSER_HAS_TRAJOPT_IFOPT
    std::filesystem::path config_path(std::string(TESSERACT_TASK_COMPOSER_DIR) + "/config/"
                                                                                 "task_composer_plugins.yaml");
#else
    std::filesystem::path config_path(std::string(TESSERACT_TASK_COMPOSER_DIR) + "/config/"
                                                                                 "task_composer_plugins_no_"
                                                                                 "trajopt_"
                                                                                 "ifopt.yaml");
#endif

    YAML::Node plugin_config = YAML::LoadFile(config_path.string());
    const YAML::Node& plugins = plugin_config[tesseract_common::TaskComposerPluginInfo::CONFIG_KEY];
    const auto search_paths = plugins["search_paths"].as<std::vector<std::string>>();
    const auto search_libraries = plugins["search_libraries"].as<std::vector<std::string>>();

    tesseract_common::TaskComposerPluginInfo info;
    info.search_paths.insert(search_paths.begin(), search_paths.end());
    info.search_libraries.insert(search_libraries.begin(), search_libraries.end());
    info.task_plugin_infos.plugins = plugins["tasks"]["plugins"].as<tesseract_common::PluginInfoMap>();
    info.executor_plugin_infos.plugins = plugins["executors"]["plugins"].as<tesseract_common::PluginInfoMap>();
    info.executor_plugin_infos.default_plugin = plugins["executors"]["default"].as<std::string>();

    TaskComposerPluginFactory factory(info);
    runTaskComposerFactoryTest(factory, plugin_config);

    auto export_config_path = std::filesystem::path(tesseract_common::getTempPath()) / "task_composer_plugins_"
                                                                                       "export.yaml";
    TaskComposerPluginFactory check_factory(export_config_path, locator);
    runTaskComposerFactoryTest(check_factory, plugin_config);
  }
}

TEST(TesseractTaskComposerFactoryUnit, PluginFactorAPIUnit)  // NOLINT
{
  TaskComposerPluginFactory factory;
  EXPECT_FALSE(factory.getSearchPaths().empty());
  EXPECT_EQ(factory.getSearchPaths().size(), 1);
  EXPECT_FALSE(factory.getSearchLibraries().empty());
  EXPECT_EQ(factory.getSearchLibraries().size(), 3);
  EXPECT_EQ(factory.getTaskComposerExecutorPlugins().size(), 0);
  EXPECT_EQ(factory.getTaskComposerNodePlugins().size(), 0);
  EXPECT_ANY_THROW(factory.getDefaultTaskComposerExecutorPlugin());  // NOLINT
  EXPECT_ANY_THROW(factory.getDefaultTaskComposerNodePlugin());      // NOLINT
  EXPECT_FALSE(factory.hasTaskComposerExecutorPlugins());
  EXPECT_FALSE(factory.hasTaskComposerNodePlugins());

  factory.addSearchPath("/usr/local/lib");
  EXPECT_EQ(factory.getSearchPaths().size(), 2);
  EXPECT_EQ(factory.getSearchLibraries().size(), 3);

  factory.addSearchLibrary("tesseract_collision");
  EXPECT_EQ(factory.getSearchPaths().size(), 2);
  EXPECT_EQ(factory.getSearchLibraries().size(), 4);

  {
    tesseract_common::PluginInfoMap map = factory.getTaskComposerExecutorPlugins();
    EXPECT_TRUE(map.find("NotFound") == map.end());

    tesseract_common::PluginInfo pi;
    pi.class_name = "TestTaskComposerExecutorFactory";
    factory.addTaskComposerExecutorPlugin("TestTaskComposerExecutor", pi);
    EXPECT_EQ(factory.getTaskComposerExecutorPlugins().size(), 1);
    EXPECT_TRUE(factory.hasTaskComposerExecutorPlugins());

    map = factory.getTaskComposerExecutorPlugins();
    EXPECT_TRUE(map.find("TestTaskComposerExecutor") != map.end());
    EXPECT_EQ(factory.getDefaultTaskComposerExecutorPlugin(), "TestTaskComposerExecutor");

    tesseract_common::PluginInfo pi2;
    pi2.class_name = "Test2TaskComposerExecutorFactory";
    factory.addTaskComposerExecutorPlugin("Test2TaskComposerExecutor", pi2);
    EXPECT_EQ(factory.getTaskComposerExecutorPlugins().size(), 2);
    EXPECT_TRUE(factory.hasTaskComposerExecutorPlugins());

    map = factory.getTaskComposerExecutorPlugins();
    EXPECT_TRUE(map.find("Test2TaskComposerExecutor") != map.end());
    EXPECT_EQ(factory.getDefaultTaskComposerExecutorPlugin(), "Test2TaskComposerExecutor");
    factory.setDefaultTaskComposerExecutorPlugin("Test2TaskComposerExecutor");
    EXPECT_EQ(factory.getDefaultTaskComposerExecutorPlugin(), "Test2TaskComposerExecutor");

    factory.removeTaskComposerExecutorPlugin("TestTaskComposerExecutor");
    map = factory.getTaskComposerExecutorPlugins();
    EXPECT_TRUE(map.find("Test2TaskComposerExecutor") != map.end());
    EXPECT_EQ(factory.getTaskComposerExecutorPlugins().size(), 1);
    // The default was removed so it should now be the first solver
    EXPECT_EQ(factory.getDefaultTaskComposerExecutorPlugin(), "Test2TaskComposerExecutor");

    // Failures
    EXPECT_ANY_THROW(factory.removeTaskComposerExecutorPlugin("DoesNotExist"));      // NOLINT
    EXPECT_ANY_THROW(factory.setDefaultTaskComposerExecutorPlugin("DoesNotExist"));  // NOLINT
  }

  {
    tesseract_common::PluginInfoMap map = factory.getTaskComposerNodePlugins();
    EXPECT_TRUE(map.find("NotFound") == map.end());

    tesseract_common::PluginInfo pi;
    pi.class_name = "TestTaskComposerNodeFactory";
    factory.addTaskComposerNodePlugin("TestTaskComposerNode", pi);
    EXPECT_EQ(factory.getTaskComposerNodePlugins().size(), 1);
    EXPECT_TRUE(factory.hasTaskComposerNodePlugins());

    map = factory.getTaskComposerNodePlugins();
    EXPECT_TRUE(map.find("TestTaskComposerNode") != map.end());
    EXPECT_EQ(factory.getDefaultTaskComposerNodePlugin(), "TestTaskComposerNode");

    tesseract_common::PluginInfo pi2;
    pi2.class_name = "Test2TaskComposerNodeFactory";
    factory.addTaskComposerNodePlugin("Test2TaskComposerNode", pi2);
    EXPECT_EQ(factory.getTaskComposerNodePlugins().size(), 2);
    EXPECT_TRUE(factory.hasTaskComposerNodePlugins());

    map = factory.getTaskComposerNodePlugins();
    EXPECT_TRUE(map.find("Test2TaskComposerNode") != map.end());
    EXPECT_EQ(factory.getDefaultTaskComposerNodePlugin(), "Test2TaskComposerNode");
    factory.setDefaultTaskComposerNodePlugin("Test2TaskComposerNode");
    EXPECT_EQ(factory.getDefaultTaskComposerNodePlugin(), "Test2TaskComposerNode");

    factory.removeTaskComposerNodePlugin("TestTaskComposerNode");

    map = factory.getTaskComposerNodePlugins();
    EXPECT_TRUE(map.find("Test2TaskComposerNode") != map.end());
    EXPECT_EQ(factory.getTaskComposerNodePlugins().size(), 1);
    // The default was removed so it should now be the first solver
    EXPECT_EQ(factory.getDefaultTaskComposerNodePlugin(), "Test2TaskComposerNode");

    // Failures
    EXPECT_ANY_THROW(factory.removeTaskComposerNodePlugin("DoesNotExist"));      // NOLINT
    EXPECT_ANY_THROW(factory.setDefaultTaskComposerNodePlugin("DoesNotExist"));  // NOLINT
  }
}

// TESTING TaskComposerPluginFactory Timing Analysis

// Helper function to perform timing analysis and check for memory leaks
void performTimingAnalysis(const std::vector<double>& timings, const std::string& test_name)
{
  // Linear regression to check for upward trend
  double n = static_cast<double>(timings.size());
  double sum_x = 0.0, sum_y = 0.0, sum_xy = 0.0, sum_x2 = 0.0;
  for (size_t i = 0; i < timings.size(); ++i)
  {
    sum_x += static_cast<double>(i);
    sum_y += timings[i];
    sum_xy += static_cast<double>(i) * timings[i];
    sum_x2 += static_cast<double>(i) * static_cast<double>(i);
  }
  double denominator = n * sum_x2 - sum_x * sum_x;
  double slope = (denominator != 0.0) ? (n * sum_xy - sum_x * sum_y) / denominator : 0.0;

  double mean = sum_y / n;
  EXPECT_LT(slope, mean * 0.10) << "Detected a significant upward trend in timings (slope=" << slope 
                                << ") for test: " << test_name;
}

// Helper function template to run timing tests with different factory creation methods
template<typename FactoryCreator>
void runFactoryTimingTest(const std::string& test_name, FactoryCreator factory_creator)
{
  constexpr int num_iterations = 10;
  std::vector<double> timings;

  for (int i = 0; i < num_iterations; ++i)
  {
    auto start = std::chrono::high_resolution_clock::now();
    factory_creator();
    auto end = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count();
    timings.push_back(elapsed);
    std::cout << test_name << " - Iteration " << i << ": " << elapsed << " seconds" << std::endl;
  }

  performTimingAnalysis(timings, test_name);
}

TEST(TesseractTaskComposerFactoryUnit, LoadTaskComposerPluginFactoryMultiTimes)
{
  std::filesystem::path config_path(std::string(TESSERACT_TASK_COMPOSER_DIR) + "/config/task_composer_plugins.yaml");
  YAML::Node plugin_config = YAML::LoadFile(config_path.string());
  tesseract_common::GeneralResourceLocator locator;

  runFactoryTimingTest("LoadFromYAMLNode", [&]() {
    tesseract_planning::TaskComposerPluginFactory factory(plugin_config, locator);
  });
}

TEST(TesseractTaskComposerFactoryUnit, LoadTaskComposerPluginFactoryMultiTimesFromFile)
{
  std::filesystem::path config_path(std::string(TESSERACT_TASK_COMPOSER_DIR) + "/config/task_composer_plugins.yaml");
  tesseract_common::GeneralResourceLocator locator;

  runFactoryTimingTest("LoadFromFile", [&]() {
    tesseract_planning::TaskComposerPluginFactory factory(config_path, locator);
  });
}

TEST(TesseractTaskComposerFactoryUnit, LoadTaskComposerPluginFactoryMultiTimesFromString)
{
  std::filesystem::path config_path(std::string(TESSERACT_TASK_COMPOSER_DIR) + "/config/task_composer_plugins.yaml");
  tesseract_common::GeneralResourceLocator locator;
  const std::string config_str = tesseract_common::fileToString(config_path);

  runFactoryTimingTest("LoadFromString", [&]() {
    tesseract_planning::TaskComposerPluginFactory factory(config_str, locator);
  });
}

TEST(TesseractTaskComposerFactoryUnit, LoadTaskComposerPluginFactoryMultiTimesFromFileWithLoadYamlFile)
{
  std::filesystem::path config_path(std::string(TESSERACT_TASK_COMPOSER_DIR) + "/config/task_composer_plugins.yaml");
  tesseract_common::GeneralResourceLocator locator;

  runFactoryTimingTest("LoadFromFileWithLoadYamlFile", [&]() {
    YAML::Node plugin_config = tesseract_common::loadYamlFile(config_path.string(), locator);
    tesseract_planning::TaskComposerPluginFactory factory(plugin_config, locator);
  });
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
