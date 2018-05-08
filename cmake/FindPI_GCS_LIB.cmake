# Try to find PI_GCS library
# Once done this will define
#  PI_GCS_LIB_FOUND - if system found PI_GCS library
#  PI_GCS_LIB_INCLUDE_DIRS - The PI_GCS include directories
#  PI_GCS_LIB_LIBRARIES - The libraries needed to use PI_GCS
#  PI_GCS_LIB_DEFINITIONS - Compiler switches required for using PI_GCS

if(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Linux")
	find_path(PI_GCS_LIB_INCLUDE_DIR 
		NAMES PI_GCS2_DLL.h
		HINTS "${CMAKE_SOURCE_DIR}/extern/pi/linux/")
		
	find_library(PI_GCS_LIB_LIBRARY 
		NAMES libpi_pi_gcs2.so
		HINTS "${CMAKE_SOURCE_DIR}/extern/pi/linux/")
endif(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Linux")

if(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")
	find_path(PI_GCS_LIB_INCLUDE_DIR 
		NAMES PI_GCS2_DLL.h 
		HINTS "${CMAKE_SOURCE_DIR}/extern/pi/win/")

	find_library(PI_GCS_LIB_LIBRARY 
		NAMES PI_GCS2_DLL
		HINTS "${CMAKE_SOURCE_DIR}/extern/pi/win/")
endif(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")

#IF(PI_GCS_LIB_LIBRARIES AND PI_GCS_LIB_INCLUDE)
   #SET(PI_GCS_LIB_FOUND TRUE)
   #MESSAGE(STATUS "Found PI library and header")
#ELSE(PI_GCS_LIB_LIBRARIES AND PI_GCS_LIB_INCLUDE)
   #SET(PI_GCS_LIB_FOUND FALSE)
   #MESSAGE(STATUS "PI library and header NOT found")
#ENDIF(PI_GCS_LIB_LIBRARIES AND PI_GCS_LIB_INCLUDE)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PI_GCS_LIB DEFAULT_MSG PI_GCS_LIB_INCLUDE_DIR PI_GCS_LIB_LIBRARY)

if (PI_GCS_LIB_FOUND)
    set(PI_GCS_LIB_LIBRARIES ${PI_GCS_LIB_LIBRARY} )
    set(PI_GCS_LIB_INCLUDE_DIRS ${PI_GCS_LIB_INCLUDE_DIR} )
    set(PI_GCS_LIB_DEFINITIONS )
endif(PI_GCS_LIB_FOUND)

mark_as_advanced(PI_GCS_LIB_ROOT_DIR PI_GCS_LIB_INCLUDE_DIR PI_GCS_LIB_LIBRARY)
