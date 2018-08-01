SET(mcp_name "MCPProducer.exe")

SET(sourcefilesMCP src/CAEN_v1742.cc)

add_definitions(-DLINUX)		#necessary for compilation on LINUX
add_definitions(-std=c++11)

find_package(ROOT REQUIRED)

INCLUDE_DIRECTORIES( include ${EXTERN_BOOST_INCLUDE_PREFIX} ${ROOT_INCLUDE_DIR})
LINK_DIRECTORIES( ${LINK_DIRECTORIES} ${EXTERN_BOOST_LIB_PREFIX} )

ADD_EXECUTABLE(${mcp_name} src/MCPProducer.cxx ${sourcefilesMCP})

TARGET_LINK_LIBRARIES(${mcp_name} EUDAQ ${EUDAQ_THREADS_LIB} libCAENComm.so libCAENDigitizer.so libCAENVME.so)


INSTALL(TARGETS ${mcp_name}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

