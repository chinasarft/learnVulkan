cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
cmake_policy(VERSION 2.8)

set(NAME mixulkan)

project(${NAME})

if(APPLE)
  if(NOT MOLTENVK_LIB_PATH)
    message(FATAL_ERROR "not set MOLTEN_LIB_PATH")
  endif()
  if(NOT MOLTENVK_INC_PATH)
    message(FATAL_ERROR "not set MOLTENVK_INC_PATH")
  endif()

  set(CMAKE_FIND_FRAMEWORK FIRST)

  FIND_LIBRARY(MoltenVK_LIB NAMES "MoltenVK" PATHS "${MOLTENVK_LIB_PATH}")
  if(NOT MoltenVK_LIB)
    message(FATAL_ERROR "cannot find MoltenVK_LIB")
  else()
    set(VULKAN_LIBRARY "${MoltenVK_LIB}")
    message("found MoltenVK:${MoltenVK_LIB}")
  endif()

  FIND_PATH(MoltenVK_INC NAMES "vulkan/vulkan.h" PATHS "${MOLTENVK_INC_PATH}")
  if(NOT MoltenVK_INC)
    message(FATAL_ERROR "cannot find MoltenVK_INC")
  else()
    set(VULKAN_INCLUDE_DIR ${MoltenVK_INC})
    message("found MoltenVK_INC:${MoltenVK_INC}")
  endif()
  set(Vulkan_FOUND true)

  FIND_LIBRARY(IOSURFACE_LIBRARY IOSurface)
  if(NOT IOSURFACE_LIBRARY)
    message(FATAL_ERROR "cannot find IOSurface")
  endif()
  FIND_LIBRARY(METAL_LIBRARY Metal)
  if(NOT METAL_LIBRARY)
    message(FATAL_ERROR "cannot find Metal")
  endif()
  FIND_LIBRARY(QuartzCore_LIBRARY QuartzCore)
  if(NOT QuartzCore_LIBRARY)
    message(FATAL_ERROR "cannot find QuartzCore")
  endif()

else()

  find_package(Vulkan)
endif()

IF (NOT Vulkan_FOUND)
        MESSAGE(FATAL_ERROR "not found Vulkan")
ENDIF()

if(APPLE)
  set(VULKAN_ROOT "${MOLTENVK_LIB_PATH}")
  set(GLFW_VULKAN_STATIC on) 
  set(VULKAN_STATIC_LIBRARY "${VULKAN_LIBRARY}")
endif()
add_subdirectory(third_party/glfw)

add_subdirectory(src/log)

add_executable(demo main.cpp helper.h helper.cpp)
set_property(TARGET demo PROPERTY CXX_STANDARD 14) 

target_include_directories(demo PUBLIC
        "${VULKAN_INCLUDE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb"
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/glm"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/log"
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/spdlog/include")

if(APPLE)
  target_link_libraries(demo ${VULKAN_LIBRARY} ${IOSURFACE_LIBRARY} ${QuartzCore_LIBRARY} ${METAL_LIBRARY} glfw log)
endif()
