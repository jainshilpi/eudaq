cmake_dependent_option(USER_CMSHGCAL_DWC_BUILD "build user/cmshgcal_dwc" ON
  "USER_CMSHGCAL_DWC_BUILD" OFF)

if(NOT USER_CMSHGCAL_DWC_BUILD)
  message("user/cmshgcal_dwc is not to be built")
  return()
endif()

SET(wirechamber_name "WireChamberProducer")

SET(sourcefilesWireChambers src/CAEN_v1290.cc src/Unpacker.cc)

add_definitions(-DLINUX)		#necessary for compilation on LINUX
add_definitions(-std=c++11)

list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
find_package(ROOT)

find_package(ROOT REQUIRED)
if(ROOT_USE_FILE)
  include(${ROOT_USE_FILE})
endif()

include_directories(${CMAKE_BINARY_DIR})

if(ROOT_VERSION_MAJOR GREATER 5)
  add_definitions(-DEUDAQ_LIB_ROOT6)
endif()


INCLUDE_DIRECTORIES( include ${EXTERN_BOOST_INCLUDE_PREFIX} ${ROOT_INCLUDE_DIR})
LINK_DIRECTORIES( ${LINK_DIRECTORIES} ${EXTERN_BOOST_LIB_PREFIX} )

ADD_EXECUTABLE(${wirechamber_name} src/WireChamberProducer.cxx ${sourcefilesWireChambers})

TARGET_LINK_LIBRARIES(${wirechamber_name} ${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB} libCAENComm.so libCAENDigitizer.so libCAENVME.so ${ROOT_LIBRARIES})


INSTALL(TARGETS ${wirechamber_name}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

