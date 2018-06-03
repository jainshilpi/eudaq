SET(wirechamber_name "WireChamberProducer.exe")

SET(sourcefilesWireChambers src/CAEN_v1290.cc src/Unpacker.cc)

add_definitions(-DLINUX)		#necessary for compilation on LINUX
add_definitions(-std=c++11)

find_package(ROOT REQUIRED)
INCLUDE_DIRECTORIES( include ${EXTERN_BOOST_INCLUDE_PREFIX} ${ROOT_INCLUDE_DIR})
LINK_DIRECTORIES( ${LINK_DIRECTORIES} ${EXTERN_BOOST_LIB_PREFIX} )

ADD_EXECUTABLE(${wirechamber_name} src/WireChamberProducer.cxx ${sourcefilesWireChambers})

TARGET_LINK_LIBRARIES(${wirechamber_name} EUDAQ ${EUDAQ_THREADS_LIB} libCAENComm.so libCAENDigitizer.so libCAENVME.so)


INSTALL(TARGETS ${wirechamber_name}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

