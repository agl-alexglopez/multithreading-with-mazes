file (GLOB_RECURSE MAZE_CC_FILES
  ${CMAKE_SOURCE_DIR}/demo/*.cc
  ${CMAKE_SOURCE_DIR}/run_maze/*.cc
  ${CMAKE_SOURCE_DIR}/measure/*.cc
  ${CMAKE_SOURCE_DIR}/module/*.cc
  ${CMAKE_SOURCE_DIR}/speed/*.cc
  ${CMAKE_SOURCE_DIR}/maze/*.cc
  ${CMAKE_SOURCE_DIR}/painters/*.cc
  ${CMAKE_SOURCE_DIR}/printers/*.cc
  ${CMAKE_SOURCE_DIR}/builders/*.cc
  ${CMAKE_SOURCE_DIR}/solvers/*.cc
)

add_custom_target (format "clang-format" -i ${MAZE_CC_FILES} COMMENT "Formatting source code...")

foreach (tidy_target ${MAZE_CC_FILES})
  get_filename_component (basename ${tidy_target} NAME)
  get_filename_component (dirname ${tidy_target} DIRECTORY)
  get_filename_component (basedir ${dirname} NAME)
  set (tidy_target_name "${basedir}__${basename}")
  set (tidy_command clang-tidy --quiet -p=${PROJECT_BINARY_DIR} ${tidy_target})
  add_custom_target (tidy_${tidy_target_name} ${tidy_command})
  list (APPEND MAZE_TIDY_TARGETS tidy_${tidy_target_name})
endforeach (tidy_target)

add_custom_target (tidy DEPENDS ${MAZE_TIDY_TARGETS})
