cmake_minimum_required(VERSION 3.10)

project(gpumix CXX)

#find_package(CXX11 REQUIRED)
foreach(i ${CMAKE_CXX_COMPILE_FEATURES})
  message("${i}")
endforeach()

if(APPLE)
  if(!MOLTENVK_LIB_PATH)
    message("there is not MOLTEN_LIB_PATH")
  endif()

  message("this is apple platform")
  set(CMAKE_FIND_FRAMEWORK FIRST)
  FIND_LIBRARY(MoltenVK_LIB NAMES "MoltenVK" PATHS "/Users/liuye/vulkan/MoltenVK/Package/Release/MoltenVK/macOS")
  if(NOT MoltenVK_LIB)
    message("cannot find MoltenVK_LIB")
  else()
    message("found MoltenVK:${MoltenVK_LIB}")
  endif()

  FIND_PATH(MoltenVK_INC NAMES "vulkan/vulkan.h" PATHS "/Users/liuye/vulkan/MoltenVK/Package/Release/MoltenVK/include")
  if(NOT MoltenVK_INC)
    message("cannot find MoltenVK_INC")
  else()
    message("found MoltenVK_INC:${MoltenVK_INC}")
  endif()

endif()
#find_package(Vulkan)

find_library(IOSURFACE_LIBRARY IOSurface)
find_library(METAL_LIBRARY Metal)
find_library(QuartzCore_LIBRARY QuartzCore)

SET(Gpumix_src 
#	src/allocator.cpp
	src/helper.cpp
	src/main.cpp
	src/offscreen.cpp
	src/onscreen.cpp)

SET(Gpumix_header
	src/helper.h
	src/offscreen.h
	src/onscreen.h)

add_subdirectory(third_party/glfw)

add_executable(demo1 ${Gpumix_header} ${Gpumix_src})
set_property(TARGET demo1 PROPERTY CXX_STANDARD 14)

target_include_directories(demo1 PUBLIC
	"/Users/liuye/vulkan/MoltenVK/Package/Release/MoltenVK/include/"
	 "third_party/stb"
	 "third_party/glm"
	"third_party/spdlog/include/)

target_link_libraries(demo1 ${MoltenVK_LIB} ${IOSURFACE_LIBRARY} ${QuartzCore_LIBRARY} ${METAL_LIBRARY} glfw)

#find_library(COCOA_LIBRARY Cocoa)
#message("cocoa:${COCOA_LIBRARY}")
#find_path(COCOA_INCLUDE Cocoa.h)
#message("cocoa:${COCOA_INCLUDE}")
message("metal:${METAL_LIBRARY}")
message("iosurface:${IOSURFACE_LIBRARY}")
message("QuartzCor:${QuartzCore_LIBRARY}")

