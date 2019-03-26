

#Add sources to the project
set(SOURCES_PREFIX ${CMAKE_SOURCE_DIR}/src)
add_subdirectory(src)
set(SOS_SOURCELIST ${SOURCES})


# Fatfs kernel for cortex m3 and m4
set(SOS_CONFIG release)
set(SOS_OPTION kernel)
set(SOS_INCLUDE_DIRECTORIES include include/fatfs)
include(${SOS_TOOLCHAIN_CMAKE_PATH}/sos-lib-std.cmake)

set(SOS_CONFIG debug)
include(${SOS_TOOLCHAIN_CMAKE_PATH}/sos-lib-std.cmake)
