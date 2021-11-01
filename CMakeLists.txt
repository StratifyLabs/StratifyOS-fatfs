cmake_minimum_required (VERSION 3.12)

project(fatfs CXX C)

sos_sdk_library_target(RELEASE ${PROJECT_NAME} "${LIB_OPTION}" release ${SOS_ARCH})
sos_sdk_add_out_of_source_directory(PRIVATE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src ${RELEASE_TARGET}_src)
sos_sdk_add_out_of_source_directory(INTERFACE_SOURCES include ${RELEASE_TARGET}_include)

add_library(${RELEASE_TARGET} STATIC)

target_sources(${RELEASE_TARGET}
	PRIVATE
	${INTERFACE_SOURCES}
	${PRIVATE_SOURCES}
	${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.cmake
	)

target_include_directories(${RELEASE_TARGET}
	PUBLIC
	$<INSTALL_INTERFACE:include/${PROJECT_NAME}>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/include/fatfs
	)

sos_sdk_library_target(DEBUG ${PROJECT_NAME} "${LIB_OPTION}" debug ${SOS_ARCH})
add_library(${DEBUG_TARGET} STATIC)
sos_sdk_copy_target(${RELEASE_TARGET} ${DEBUG_TARGET})
sos_sdk_library_add_arch_targets("${DEBUG_OPTIONS}" ${SOS_ARCH} "StratifyOS_mcu")
sos_sdk_library_add_arch_targets("${RELEASE_OPTIONS}" ${SOS_ARCH} "StratifyOS_mcu")

target_compile_options(${RELEASE_TARGET}
	PRIVATE
	-Os
	)

target_compile_options(${DEBUG_TARGET}
	PRIVATE
	-Os
	)

install(DIRECTORY include/
	DESTINATION include/${PROJECT_NAME}
	PATTERN CMakelists.txt EXCLUDE)

install(FILES ${PROJECT_NAME}.cmake
	DESTINATION ${SOS_SDK_PATH}/cmake/targets)



