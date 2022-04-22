cmake_minimum_required(VERSION 3.4.1)

set(${CMAKE_C_FLAGS}, "${CMAKE_C_FLAGS}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -Wall -Werror")
link_directories(../jniLibs/arm64-v8a)

add_library(main SHARED main.c)

SET(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Configs" FORCE)
SET(BUILD_FOLDER release-android-aarch64)
if(CMAKE_BUILD_TYPE MATCHES "Debug")
	SET(BUILD_FOLDER debug-android-aarch64)
endif()
add_custom_target(copy_libs
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/../../../../../build/${BUILD_FOLDER}/libioquake3_opengl2.so ${CMAKE_SOURCE_DIR}/../jniLibs/arm64-v8a/libioquake3_opengl2.so
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/../../../../../build/${BUILD_FOLDER}/libSDL2.so ${CMAKE_SOURCE_DIR}/../jniLibs/arm64-v8a/libSDL2.so
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/../../../../../build/${BUILD_FOLDER}/libopenxr_loader.so ${CMAKE_SOURCE_DIR}/../jniLibs/arm64-v8a/libopenxr_loader.so
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/../../../../../build/${BUILD_FOLDER}/baseq3/cgameaarch64.so ${CMAKE_SOURCE_DIR}/../jniLibs/arm64-v8a/libcgameaarch64_baseq3.so
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/../../../../../build/${BUILD_FOLDER}/baseq3/qagameaarch64.so ${CMAKE_SOURCE_DIR}/../jniLibs/arm64-v8a/libqagameaarch64_baseq3.so
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/../../../../../build/${BUILD_FOLDER}/baseq3/uiaarch64.so ${CMAKE_SOURCE_DIR}/../jniLibs/arm64-v8a/libuiaarch64_baseq3.so
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/../../../../../build/${BUILD_FOLDER}/missionpack/cgameaarch64.so ${CMAKE_SOURCE_DIR}/../jniLibs/arm64-v8a/libcgameaarch64_missionpack.so
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/../../../../../build/${BUILD_FOLDER}/missionpack/qagameaarch64.so ${CMAKE_SOURCE_DIR}/../jniLibs/arm64-v8a/libqagameaarch64_missionpack.so
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/../../../../../build/${BUILD_FOLDER}/missionpack/uiaarch64.so ${CMAKE_SOURCE_DIR}/../jniLibs/arm64-v8a/libuiaarch64_missionpack.so
	)

add_dependencies(main copy_libs)

target_include_directories(main PRIVATE
	${CMAKE_SOURCE_DIR}/code/OpenXR-SDK/include
	${CMAKE_SOURCE_DIR}/code/OpenXR/Include
	${CMAKE_SOURCE_DIR}/code/SDL2/include
	${CMAKE_SOURCE_DIR}/code)

target_compile_definitions(main PRIVATE
	ARCH_STRING=android)

find_library(log-lib log)

target_link_libraries(main
	android
	ioquake3_opengl2
	SDL2
	openxr_loader
	${log-lib})
