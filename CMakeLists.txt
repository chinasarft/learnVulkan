cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
cmake_policy(VERSION 2.8)

set(NAME mixulkan)

project(${NAME})

find_package(Vulkan)

IF (NOT Vulkan_FOUND)
	MESSAGE(FATAL_ERROR "not found Vulkan")
ENDIF()
