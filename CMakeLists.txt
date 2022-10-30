cmake_minimum_required (VERSION 3.12)
cmsdk2_minimum_required(2.1.2)

project(fatfs
	LANGUAGES C
	VERSION 1.2.1)

set(${PROJECT_NAME}_VERSION ${PROJECT_VERSION} CACHE INTERNAL "StratifyOS project version")

cmsdk_add_out_of_source_directory(PRIVATE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src ${RELEASE_TARGET}_src)
cmsdk_add_out_of_source_directory(INTERFACE_SOURCES include ${RELEASE_TARGET}_include)

cmsdk2_add_library(
	NAME ${PROJECT_NAME}
	CONFIG release
	ARCH ${CMSDK_ARCH}
	TARGET RELEASE_TARGET)

target_sources(${RELEASE_TARGET}
	PRIVATE
	${INTERFACE_SOURCES}
	${PRIVATE_SOURCES}
	${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.cmake)

target_include_directories(${RELEASE_TARGET}
	PUBLIC
	$<INSTALL_INTERFACE:include/${PROJECT_NAME}>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	PRIVATE
	include/fatfs)

cmsdk2_add_library(
	NAME ${PROJECT_NAME}
	CONFIG debug
	ARCH ${CMSDK_ARCH}
	TARGET DEBUG_TARGET)

cmsdk2_copy_target(
	SOURCE ${RELEASE_TARGET}
	DESTINATION ${DEBUG_TARGET})

cmsdk2_library_add_dependencies(
	TARGET ${RELEASE_TARGET}
	DEPENDENCIES StratifyOS_mcu)

cmsdk2_library_add_dependencies(
	TARGET ${DEBUG_TARGET}
	DEPENDENCIES StratifyOS_mcu)

install(DIRECTORY include/
	DESTINATION include/${PROJECT_NAME}
	PATTERN CMakelists.txt EXCLUDE)

install(FILES ${PROJECT_NAME}.cmake
	DESTINATION ${SOS_SDK_PATH}/cmake/targets)



