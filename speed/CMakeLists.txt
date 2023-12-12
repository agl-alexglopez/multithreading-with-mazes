add_library(speed)
target_sources(speed
  PUBLIC
    FILE_SET CXX_MODULES
    BASE_DIRS 
      ${PROJECT_SOURCE_DIR}/speed
    FILES
      ${PROJECT_SOURCE_DIR}/speed/speed.cc
)
