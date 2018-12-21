cmake_minimum_required (VERSION 2.8.12)
project(FTCI2C)
# Set common compiler flags. These flags will be used regardless of build configuration
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

find_package (Threads REQUIRED)
find_package(PkgConfig)
pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(DLT REQUIRED automotive-dlt)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/src/inc
	${CMAKE_CURRENT_SOURCE_DIR}/src/hicom/inc
	${CMAKE_CURRENT_SOURCE_DIR}/src/LIB/inc
)

set(INCLUDE 
		${CMAKE_CURRENT_SOURCE_DIR}/FT2232hMpsseI2c.h
		${CMAKE_CURRENT_SOURCE_DIR}/FT2232c.h
		${CMAKE_CURRENT_SOURCE_DIR}/FT2232h.h
		${CMAKE_CURRENT_SOURCE_DIR}/FTCI2C.h
		${CMAKE_CURRENT_SOURCE_DIR}/stdafx.h
		${CMAKE_CURRENT_SOURCE_DIR}/WinTypes.h
)

set(LIB_INCLUDE
		${CMAKE_CURRENT_SOURCE_DIR}/FTD2XX.h
)

set(SOURCE
		${CMAKE_CURRENT_SOURCE_DIR}/FT2232c.c
		${CMAKE_CURRENT_SOURCE_DIR}/FT2232h.c
		${CMAKE_CURRENT_SOURCE_DIR}/FT2232hMpsseI2c.c
		${CMAKE_CURRENT_SOURCE_DIR}/FTCI2C.c
		${CMAKE_CURRENT_SOURCE_DIR}/stdafx.c
		${CMAKE_CURRENT_SOURCE_DIR}/main.c
)
#add_definitions(-w)
set(ZMOD44xx_SRC ${SOURCE} ${INCLUDE} ${LIB_INCLUDE})

find_library(FTD2XX_LIB_PATH NAMES libftd2xx.so.1.2.7 PATHS ${CMAKE_CURRENT_SOURCE_DIR} NO_CMAKE_FIND_ROOT_PATH)
#find_library(IAQ_LIB_PATH iaq PATHS ${CMAKE_CURRENT_SOURCE_DIR}/LIB/lib NO_CMAKE_FIND_ROOT_PATH)
#find_library(ODOR_LIB_PATH odor PATHS ${CMAKE_CURRENT_SOURCE_DIR}/LIB/lib NO_CMAKE_FIND_ROOT_PATH)
#find_library(R_CDA_LIB_PATH r_cda PATHS ${CMAKE_CURRENT_SOURCE_DIR}/LIB/lib NO_CMAKE_FIND_ROOT_PATH)
#find_library(TVOC_LIB_PATH tvoc PATHS ${CMAKE_CURRENT_SOURCE_DIR}/LIB/lib NO_CMAKE_FIND_ROOT_PATH)

#set(UNITLIB ${ECO2_LIB_PATH} ${IAQ_LIB_PATH} ${ODOR_LIB_PATH} ${R_CDA_LIB_PATH} ${TVOC_LIB_PATH})

#add_executable(ftci2c ${ZMOD44xx_SRC})
add_library(ftci2c	SHARED ${ZMOD44xx_SRC})

target_link_libraries(ftci2c	pthread
				rt
				${FTD2XX_LIB_PATH}
)

install(TARGETS ftci2c LIBRARY DESTINATION /usr/lib)
#install(TARGETS ftci2c RUNTIME DESTINATION /usr/bin)
