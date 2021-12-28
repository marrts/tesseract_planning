find_package(descartes_light REQUIRED)
find_package(tesseract_collision REQUIRED)
find_package(ompl REQUIRED)

# Descartes Planner
add_library(
  ${PROJECT_NAME}_descartes
  src/descartes_motion_planner.cpp
  src/descartes_collision.cpp
  src/descartes_collision_edge_evaluator.cpp
  src/descartes_robot_sampler.cpp
  src/descartes_motion_planner_status_category.cpp
  src/serialize.cpp
  src/deserialize.cpp
  src/descartes_utils.cpp
  src/profile/descartes_default_plan_profile.cpp)
target_link_libraries(
  ${PROJECT_NAME}_descartes
  PUBLIC ${PROJECT_NAME}_core
         ${OMPL_LIBRARIES}
         tesseract::tesseract_collision_core
         descartes::descartes_light
         descartes::descartes_light_ompl
         Boost::boost
         ${CMAKE_THREAD_LIBS_INIT})
target_compile_options(${PROJECT_NAME}_descartes PRIVATE ${TESSERACT_COMPILE_OPTIONS_PRIVATE})
target_compile_options(${PROJECT_NAME}_descartes PUBLIC ${TESSERACT_COMPILE_OPTIONS_PUBLIC})
target_compile_definitions(${PROJECT_NAME}_descartes PUBLIC ${TESSERACT_COMPILE_DEFINITIONS})
target_clang_tidy(${PROJECT_NAME}_descartes ENABLE ${TESSERACT_ENABLE_CLANG_TIDY})
target_cxx_version(${PROJECT_NAME}_descartes PUBLIC VERSION ${TESSERACT_CXX_VERSION})
target_code_coverage(
  ${PROJECT_NAME}_descartes
  ALL
  EXCLUDE ${COVERAGE_EXCLUDE}
  ENABLE ${TESSERACT_ENABLE_CODE_COVERAGE})
target_include_directories(${PROJECT_NAME}_descartes PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
                                                            "$<INSTALL_INTERFACE:include>")
target_include_directories(${PROJECT_NAME}_descartes SYSTEM PUBLIC ${OMPL_INCLUDE_DIRS})
if(OMPL_VERSION VERSION_LESS "1.4.0")
  target_compile_definitions(${PROJECT_NAME}_ompl PUBLIC OMPL_LESS_1_4_0=ON)
endif()

# Mark cpp header files for installation
install(DIRECTORY include/${PROJECT_NAME} DESTINATION include)

# Install targets
install_targets(TARGETS ${PROJECT_NAME}_descartes)
