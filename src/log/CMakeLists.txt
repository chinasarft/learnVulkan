#CMAKE_MINIMUM_REQUIRED(VERSION 3.8)  
PROJECT(LOGGER CXX)

SET(LOGGER_SOURCE_FILES
	logger.cpp
)

SET(LOGGER_HEADER_FILES
	logger.h
)

if(WIN32)
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /source-charset:utf-8 /D_CRT_SECURE_NO_WARNINGS")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /source-charset:utf-8 /D_CRT_SECURE_NO_WARNINGS")
endif()

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/spdlog/include/")
message("=====>:${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/spdlog/include/")


#添加静态库  
ADD_LIBRARY(log STATIC ${LOGGER_SOURCE_FILES} ${LOGGER_HEADER_FILES})
set_property(TARGET log PROPERTY CXX_STANDARD 14)


#SET(TEST_SOURCE_FILE
#       test.cpp
#)
#ADD_EXECUTABLE(testlog${TEST_SOURCE_FILE})
#target_link_libraries(testloglog)

