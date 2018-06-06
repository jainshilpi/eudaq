SET(hgcal_name "HGCalProducer.exe")
SET(hgcal_standalone_daq "HGCalOnly_DAQ.exe")
SET(hgcal_unpacker "HGCalDataUnpacker.exe")

SET(sourcefilesHGCalProducer src/IpbusHwController.cc src/HGCalController.cc src/unpacker.cc)

find_package(CACTUS REQUIRED)
find_package(ROOT REQUIRED)
INCLUDE_DIRECTORIES( include ${UHAL_UHAL_INCLUDE_PREFIX} ${ROOT_INCLUDE_DIR})
LINK_DIRECTORIES( ${LINK_DIRECTORIES} ${UHAL_UHAL_LIB_PREFIX} )

ADD_EXECUTABLE(${hgcal_name} src/HGCalProducer.cxx ${sourcefilesHGCalProducer})
ADD_EXECUTABLE(${hgcal_standalone_daq} src/HGCalOnly_DAQ.cxx ${sourcefilesHGCalProducer})
ADD_EXECUTABLE(${hgcal_unpacker} src/HGCalDataUnpacker.cxx ${sourcefilesHGCalProducer})

set(BOOST_LIB boost_timer boost_thread boost_filesystem boost_regex boost_system boost_thread boost_program_options boost_iostreams)
set(CACTUS_LIB cactus_extern_pugixml cactus_uhal_log cactus_uhal_grammars cactus_uhal_uhal)

TARGET_LINK_LIBRARIES(${hgcal_name} EUDAQ ${EUDAQ_THREADS_LIB} ${ROOT_LIBRARIES} ${CACTUS_LIB} ${BOOST_LIB} )
TARGET_LINK_LIBRARIES(${hgcal_standalone_daq} EUDAQ ${EUDAQ_THREADS_LIB} ${ROOT_LIBRARIES} ${CACTUS_LIB} ${BOOST_LIB} )
TARGET_LINK_LIBRARIES(${hgcal_unpacker} EUDAQ ${EUDAQ_THREADS_LIB} ${ROOT_LIBRARIES} ${CACTUS_LIB} ${BOOST_LIB} )

INSTALL(TARGETS  ${hgcal_name} ${hgcal_standalone_daq} ${hgcal_unpacker}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
  
