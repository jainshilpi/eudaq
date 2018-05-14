SET(hgcal_name "HGCalProducer.exe")
SET(hgcal_standalone_daq "HGCalOnly_DAQ.exe")

SET(sourcefilesHGCalProducer src/IpbusHwController.cc src/TriggerController.cc src/HGCalController.cc src/unpacker.cc)

find_package(CACTUS REQUIRED)
find_package(ROOT REQUIRED)
INCLUDE_DIRECTORIES( include ${UHAL_LOG_INCLUDE_PREFIX} ${UHAL_GRAMMARS_INCLUDE_PREFIX} ${EXTERN_BOOST_INCLUDE_PREFIX} ${EXTERN_PUGIXML_INCLUDE_PREFIX} ${UHAL_UHAL_INCLUDE_PREFIX} ${ROOT_INCLUDE_DIR})
LINK_DIRECTORIES( ${LINK_DIRECTORIES} ${EXTERN_BOOST_LIB_PREFIX} ${EXTERN_PUGIXML_LIB_PREFIX} ${UHAL_LOG_LIB_PREFIX} ${UHAL_GRAMMARS_LIB_PREFIX} ${UHAL_UHAL_LIB_PREFIX} )

ADD_EXECUTABLE(${hgcal_name} src/HGCalProducer.cxx ${sourcefilesHGCalProducer})
ADD_EXECUTABLE(${hgcal_standalone_daq} src/HGCalOnly_DAQ.cxx ${sourcefilesHGCalProducer})

TARGET_LINK_LIBRARIES(${hgcal_name} EUDAQ ${EUDAQ_THREADS_LIB} ${ROOT_LIBRARIES} boost_timer boost_thread boost_filesystem boost_regex boost_system boost_thread boost_program_options cactus_extern_pugixml cactus_uhal_log cactus_uhal_grammars cactus_uhal_uhal)
TARGET_LINK_LIBRARIES(${hgcal_standalone_daq} EUDAQ ${EUDAQ_THREADS_LIB} ${ROOT_LIBRARIES} boost_timer boost_thread boost_filesystem boost_regex boost_system boost_thread boost_program_options cactus_extern_pugixml cactus_uhal_log cactus_uhal_grammars cactus_uhal_uhal)

INSTALL(TARGETS  ${hgcal_name}	   
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
INSTALL(TARGETS  ${hgcal_standalone_daq}	   
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

add_subdirectory(onlinemon)
