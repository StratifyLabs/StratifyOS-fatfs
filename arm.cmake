

#Add sources to the project
set(SOURCES_PREFIX ${CMAKE_SOURCE_DIR}/src)
add_subdirectory(src)
set(HEADERS
	${CMAKE_SOURCE_DIR}/include/fatfs.h
	${CMAKE_SOURCE_DIR}/include/fatfs/ff.h
	${CMAKE_SOURCE_DIR}/include/fatfs/ffconf.h
	${CMAKE_SOURCE_DIR}/include/fatfs/integer.h
	)
set(SOS_SOURCELIST ${SOURCES} ${HEADERS})


# Fatfs kernel for cortex m3 and m4
set(SOS_CONFIG release)
set(SOS_OPTION kernel)
set(SOS_INCLUDE_DIRECTORIES include include/fatfs)
include(${SOS_TOOLCHAIN_CMAKE_PATH}/sos-lib-std.cmake)

set(SOS_CONFIG debug)
include(${SOS_TOOLCHAIN_CMAKE_PATH}/sos-lib-std.cmake)

add_custom_target(
	format
	COMMAND /usr/local/bin/clang-format
	-i
	--verbose
	${SOS_SOURCELIST}
	)
