set(namecontrol "PIControl")
set(namecontroller "PIController")

FIND_PACKAGE(PI_GCS_LIB REQUIRED)

IF(NOT PI_GCS_LIB_FOUND)
  MESSAGE(FATAL_ERROR "Couldn't find PI_GCS library or header!")
ENDIF()

add_definitions(${PI_GCS_LIB_DEFINITIONS})
include_directories(${PI_GCS_LIB_INCLUDE_DIR})

set(sourcefilecontrol src/HexGrid.cc src/PIWrapper.cc src/PIControl.cxx)
set(sourcefilecontroller src/HexGrid.cc src/PIWrapper.cc src/PIController.cxx)

file(COPY ${PI_GCS_LIB_LIBRARIES} DESTINATION ${CMAKE_BINARY_DIR})

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(${namecontrol} ${sourcefilecontrol})
add_executable(${namecontroller} ${sourcefilecontroller})

##MESSAGE(${PIWrapper_LIBRARIES})
target_link_libraries(${namecontrol} EUDAQ ${PI_GCS_LIB_LIBRARIES})
target_link_libraries(${namecontroller} EUDAQ ${PI_GCS_LIB_LIBRARIES})

##MESSAGE("EUDAQ" EUDAQ ${PIWrapper_LIBRARIES} "   " ${PIWrapper_INCLUDE_DIRS})

INSTALL(TARGETS PIControl PIController
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)