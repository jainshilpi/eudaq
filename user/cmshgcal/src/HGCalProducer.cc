#include "HGCalProducer.h"
#include "eudaq/Configuration.hh"
#include "eudaq/Logger.hh"

#include <iostream>
#include <ostream>
#include <sstream>
#include <vector>
#include <memory>
#include <iomanip>
#include <iterator>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>

#include <boost/crc.hpp>
#include <boost/cstdint.hpp>
#include <boost/timer/timer.hpp>
#include <boost/format.hpp>

#define FORMAT_VERSION 1

void readFIFOThread( ipbus::IpbusHwController* orm, uint32_t *blockSize)
{
  orm->ReadDataBlock("FIFO",*blockSize);
}

void startTriggerThread( TriggerController* trg_ctrl, uint32_t *run, ACQ_MODE* mode)
{
  trg_ctrl->startrunning( *run, *mode );
}

// namespace{
//   auto dummy0 = eudaq::Factory<eudaq::Producer>::
//     Register<eudaq::HGCalProducer, const std::string&, const std::string&>(eudaq::HGCalProducer::m_id_factory);
// }

namespace eudaq{

  HGCalProducer::HGCalProducer(const std::string & name, const std::string & runcontrol)
    : eudaq::Producer(name, runcontrol),
    m_run(0),
    m_ev(0),
    m_uhalLogLevel(5),
    m_blockSize(963),
    m_state(STATE_UNCONF),
    m_acqmode(DEBUG),
    m_triggerController(NULL)
  {}

  bool HGCalProducer::checkCRC( const std::string & crcNodeName, ipbus::IpbusHwController *ptr )
  {
    std::vector<uint32_t> tmp=ptr->getData();
    uint32_t *data=&tmp[0];
    boost::crc_32_type checksum;
    checksum.process_bytes(data,(std::size_t)tmp.size());
    uint32_t crc=ptr->ReadRegister(crcNodeName.c_str());
    std::ostringstream os( std::ostringstream::ate );
    if( crc==checksum.checksum() ){
      os.str(""); os << "checksum success (" << crc << ")";
      EUDAQ_DEBUG( os.str().c_str() );
      return true;
    }
    os.str(""); os << "in HGCalProducer.cxx : checkCRC( const std::string & crcNodeName, ipbus::IpbusHwController *ptr ) -> checksum fail : sent " << crc << "\t compute" << checksum.checksum() << " -> sleep 10 sec; PLEASE REACT";
    EUDAQ_ERROR( os.str().c_str() );
    eudaq::mSleep(10000);
    return false;
  }

  void HGCalProducer::MainLoop() 
  {
    StartCommandReceiver();
    std::ostringstream os( std::ostringstream::ate );
    while (m_state != STATE_GOTOTERM){
      if( m_state != STATE_RUNNING ) {
	os.str("");
	os << "je suis dans la main loop et le state c'est STATE_";
	switch(m_state){
	case STATE_ERROR: {os << "ERROR"; break;}
	case STATE_UNCONF: {os << "UNCONF"; break;}
	case STATE_GOTOCONF: {os << "GOTOCONF"; break;}
	case STATE_CONFED: {os << "CONFED"; break;}
	case STATE_GOTORUN: {os << "GOTORUN"; break;}
	case STATE_GOTOSTOP: {os << "GOTOSTOP"; break;}
	}
	std::cout << os.str() << std::endl;
	eudaq::mSleep(1000);
      }
      if (m_state == STATE_UNCONF) {
	eudaq::mSleep(100);
	continue;
      }    

      if (m_state == STATE_RUNNING) {
	if( !m_triggerController->checkState( (STATES)RDOUT_RDY ) ) continue;
	if( m_ev==m_triggerController->eventNumber() ) continue;
	boost::timer::cpu_timer timerReadout;
	boost::timer::cpu_times times;
	//eudaq::RawDataEvent ev(EVENT_TYPE,m_run,m_ev);
	boost::thread threadVec[m_rdout_orms.size()];
	
	for( int i=0; i<(int)m_rdout_orms.size(); i++)
	  threadVec[i]=boost::thread(readFIFOThread,m_rdout_orms[i],&m_blockSize);
	
	for( int i=0; i<(int)m_rdout_orms.size(); i++){
	  threadVec[i].join();}

	times=timerReadout.elapsed();
	//m_hreadouttime->Fill(times.wall/1e9);

	int head[1];
	boost::timer::cpu_timer timerWriter;
	for( int i=0; i<(int)m_rdout_orms.size(); i++){
	  std::vector<uint32_t> the_data = m_rdout_orms[i]->getData() ;
	  // Adding trailer
	  //  checkCRC( "RDOUT.CRC",m_rdout_orms[i]);
	  uint32_t trailer=i;//8 bits for orm id
	  std::cout << "board id = " << trailer;
	  trailer|=m_triggerController->eventNumber()<<8;//24 bits for trigger number
	  std::cout << "\t event number id = " << m_triggerController->eventNumber();
	  std::cout << "\t trailer = " << trailer << std::endl;	  //m_rdout_orms[i]->addTrailerToData(trailer);
	  
	  the_data.push_back(trailer);

	  // Send it to euDAQ converter plugins:
          head[0] = i+1;
          // ev.AddBlock(   2*i, head, sizeof(head));
	  // ev.AddBlock( 2*i+1, the_data);

	  std::cout<<i<<"  head[0]="<<head[0]<<"  Size of the data (bytes): "<<std::dec<<the_data.size()*4<<std::endl;

	  //Add trigger timestamp to raw data :
	  the_data.push_back( m_rdout_orms[i]->ReadRegister("CLK_COUNT0") );
	  the_data.push_back( m_rdout_orms[i]->ReadRegister("CLK_COUNT1") );
	  std::cout << std::setw(8) << std::setfill('0') << std::hex << m_rdout_orms[i]->ReadRegister("CLK_COUNT0") << "\t" << m_rdout_orms[i]->ReadRegister("CLK_COUNT1") << std::endl;
	  
	  // Write it into raw file:
          m_rawFile.write(reinterpret_cast<const char*>(&the_data[0]), the_data.size()*sizeof(uint32_t));
	  
	}
	times=timerWriter.elapsed();
	//m_hwritertime->Fill(times.wall/1e9);

	m_ev=m_triggerController->eventNumber();
	//SendEvent(ev);
	for( std::vector<ipbus::IpbusHwController*>::iterator it=m_rdout_orms.begin(); it!=m_rdout_orms.end(); ++it ){
	  (*it)->ResetTheData();
	  while(1){
	    if( (*it)->ReadRegister("DATE_STAMP") )
	      break;
            else
              boost::this_thread::sleep( boost::posix_time::microseconds(1) );     
	  }
	}
	std::cout << "receive and save a new event" << std::endl;
	m_triggerController->readoutCompleted();
      }
      if (m_state == STATE_GOTOSTOP) {
	std::cout << "on envoie un evt tout pourri pour la fin" << std::endl;
	//SendEvent( eudaq::RawDataEvent::EORE(EVENT_TYPE,m_run,++m_ev) );
	std::cout << "ca a du marcher" << std::endl;
	eudaq::mSleep(1000);
	m_state = STATE_CONFED;
	continue;
      }
    }
  }

  void HGCalProducer::DoConfigure() 
  {
    auto config = GetConfiguration();
    std::cout << "Configuring: " << config->Name() << std::endl;
    config->Print(std::cout);

    // Do any configuration of the hardware here
    // Configuration file values are accessible as config->Get(name, default)
    m_uhalLogLevel = config->Get("UhalLogLevel", 5);
    m_blockSize = config->Get("DataBlockSize",962);
    int mode=config->Get("AcquisitionMode",0);
    switch( mode ){
    case 0 : m_acqmode = DEBUG; break;
    case 1 : m_acqmode = BEAMTEST; break;
    default : m_acqmode = DEBUG; break;
    }
    unsigned int n_orms = config->Get("NumberOfORMs",1);
    std::string syncPi=config->Get("SYNC_Pi_Alias",std::string("piS"));
    std::string rdoutPis=config->Get("RDOUT_Pi_Aliases",std::string("piRBDev"));
    std::istringstream iss(rdoutPis);
    std::vector<std::string> aliases;
    std::copy(std::istream_iterator<std::string>(iss),
	      std::istream_iterator<std::string>(),
	      back_inserter(aliases));
    if( aliases.size()!=n_orms )
      std::cout << "ERROR : Size of RDOUT_Pi_Aliases in conf should be the same as NumberOfOrms (a crash is expected)" << std::endl;

    startSoftOnSYNCBoard( syncPi );

    for (unsigned int ipi=0; ipi<n_orms; ipi++)
      startSoftOnRDOUTBoard( aliases[ipi] );
    // End of scripts on RPIs

    std::ostringstream deviceName( std::ostringstream::ate );
    for( int iorm=0; iorm<n_orms; iorm++ ){
      deviceName.str(""); deviceName << config->Get("RDOUT_ORM_PrefixName","RDOUT_ORM") << iorm;
      ipbus::IpbusHwController *orm = new ipbus::IpbusHwController(config->Get("ConnectionFile","file://./etc/connection.xml"),deviceName.str());
      m_rdout_orms.push_back( orm );
    }    

    m_triggerController = new TriggerController(m_rdout_orms);
    m_state=STATE_CONFED;

    for( int iorm=0; iorm<n_orms; iorm++ ){
      std::cout << "ORM " << iorm << "\n"
		<< "Check0 = " << std::hex << m_rdout_orms[iorm]->ReadRegister("check0") << "\t"
		<< "Check1 = " << std::hex << m_rdout_orms[iorm]->ReadRegister("check1") << "\t"
		<< "Check2 = " << std::hex << m_rdout_orms[iorm]->ReadRegister("check2") << "\t"
		<< "Check3 = " << std::hex << m_rdout_orms[iorm]->ReadRegister("check3") << "\t"
		<< "Check4 = " << std::hex << m_rdout_orms[iorm]->ReadRegister("check4") << "\t"
		<< "Check5 = " << std::hex << m_rdout_orms[iorm]->ReadRegister("check5") << "\t"
		<< "Check6 = " << std::hex << m_rdout_orms[iorm]->ReadRegister("check6") << "\n"
		<< "Check31 = " << std::dec << m_rdout_orms[iorm]->ReadRegister("check31") << "\t"
		<< "Check32 = " << std::dec << m_rdout_orms[iorm]->ReadRegister("check32") << "\t"
		<< "Check33 = " << std::dec << m_rdout_orms[iorm]->ReadRegister("check33") << "\t"
		<< "Check34 = " << std::dec << m_rdout_orms[iorm]->ReadRegister("check34") << "\t"
		<< "Check35 = " << std::dec << m_rdout_orms[iorm]->ReadRegister("check35") << "\t"
		<< "Check36 = " << std::dec << m_rdout_orms[iorm]->ReadRegister("check36") << std::endl;

      uint32_t mask=m_rdout_orms[iorm]->ReadRegister("SKIROC_MASK");
      std::cout << "ORM " << iorm << "\t SKIROC_MASK = " << mask << std::endl;
      uint32_t cst0=m_rdout_orms[iorm]->ReadRegister("CONSTANT0");
      std::cout << "ORM " << iorm << "\t CONST0 = " << std::hex << cst0 << std::endl;
      uint32_t cst1=m_rdout_orms[iorm]->ReadRegister("CONSTANT1");
      std::cout << "ORM " << iorm << "\t CONST1 = " << std::hex << cst1 << std::endl;
      boost::this_thread::sleep( boost::posix_time::milliseconds(1000) );
    }    

  }

  void HGCalProducer::startSoftOnSYNCBoard( std::string syncPi )
  {
    std::cout << "Starting sync_debug.exe on " << syncPi << std::endl;
    int executionStatus = -99; 
    std::ostringstream cmd( std::ostringstream::ate );
    cmd.str("");cmd<<"ssh -T "<<syncPi<<" \" sudo killall sync_debug.exe \"";
    executionStatus = system(cmd.str().c_str());
    if (executionStatus != 0) 
      std::cout << "Error: unable to kill on " << syncPi << ". It's may be already dead..." << std::endl;
    else 
      std::cout << "Successfully killed on " << syncPi << "!" << std::endl;
    cmd.str("");cmd<<"ssh -T "<<syncPi<<" \" nohup sudo /home/pi/SYNCH_BOARD/bin/sync_debug.exe 0 > log.log 2>&1& \"";
    executionStatus = system(cmd.str().c_str());
    if (executionStatus != 0) 
      std::cout << "Error: unable to run sync on "<< syncPi << std::endl;
    else
      std::cout << "Successfully run sync on " << syncPi << "!" << std::endl;
  }

  void HGCalProducer::startSoftOnRDOUTBoard( std::string rdoutPi )
  {
    std::cout << "Starting new_rdout.exe on " << rdoutPi << std::endl;
    int executionStatus = -99; 
    std::ostringstream cmd( std::ostringstream::ate );
    cmd.str("");cmd<<"ssh -T "<<rdoutPi<<" \" sudo killall new_rdout.exe \"";
    executionStatus = system(cmd.str().c_str());
    if (executionStatus != 0) 
      std::cout << "Error: unable to kill on " << rdoutPi << ". It's may be already dead..." << std::endl;
    else 
      std::cout << "Successfully killed on " << rdoutPi << "!" << std::endl;
    cmd.str("");cmd<<"ssh -T "<<rdoutPi<<" \" nohup sudo /home/pi/RDOUT_BOARD_IPBus/rdout_software/bin/new_rdout.exe 200 100000 0 > log.log 2>&1& \"";
    executionStatus = system(cmd.str().c_str());
    if (executionStatus != 0) 
      std::cout << "Error: unable to run sync on "<< rdoutPi << std::endl;
    else
      std::cout << "Successfully run sync on " << rdoutPi << "!" << std::endl;
  }

  void HGCalProducer::stopSoftOnSYNCBoard( std::string syncPi )
  {
    std::cout << "Stopping sync_debug.exe on " << syncPi << std::endl;
    int executionStatus = -99; 
    std::ostringstream cmd( std::ostringstream::ate );
    cmd.str("");cmd<<"ssh -T "<<syncPi<<" \" sudo killall sync_debug.exe \"";
    executionStatus = system(cmd.str().c_str());
    if (executionStatus != 0) 
      std::cout << "Error: unable to kill on " << syncPi << ". This is strange, it was supposed to run..." << std::endl;
    else 
      std::cout << "Successfully killed on " << syncPi << "!" << std::endl;
  }

  void HGCalProducer::stopSoftOnRDOUTBoard( std::string rdoutPi )
  {
    std::cout << "Stoppinf new_rdout.exe on " << rdoutPi << std::endl;
    int executionStatus = -99; 
    std::ostringstream cmd( std::ostringstream::ate );
    cmd.str("");cmd<<"ssh -T "<<rdoutPi<<" \" sudo killall new_rdout.exe \"";
    executionStatus = system(cmd.str().c_str());
    if (executionStatus != 0) 
      std::cout << "Error: unable to kill on " << rdoutPi << ". This is strange, it was supposed to run..." << std::endl;
    else
      std::cout << "Successfully killed on " << rdoutPi << std::endl;
  }
  
  // This gets called whenever a new run is started
  // It receives the new run number as a parameter
  void HGCalProducer::DoStartRun() 
  {
    m_state = STATE_GOTORUN;
    
    m_run = GetRunNumber();
    m_ev = 0;
    
    // Let's open a file for raw data:
    char rawFilename[256];
    sprintf(rawFilename, "raw_data/HexaData_Run%04d.raw", m_run); // The path is relative to eudaq/bin
    m_rawFile.open(rawFilename, std::ios::binary);

    uint32_t header[3];
    header[0]=time(0);
    header[1]=m_rdout_orms.size();
    header[1]|=m_run<<8;
    header[2]=FORMAT_VERSION;
    m_rawFile.write(reinterpret_cast<const char*>(&header[0]), sizeof(header));
    
    for( std::vector<ipbus::IpbusHwController*>::iterator it=m_rdout_orms.begin(); it!=m_rdout_orms.end(); ++it ){
      (*it)->ResetTheData();
      while(1){
	if( (*it)->ReadRegister("DATE_STAMP") )
	  break;
	else
	  boost::this_thread::sleep( boost::posix_time::microseconds(1) );     
      }
    }
    m_triggerThread=boost::thread(startTriggerThread,m_triggerController,&m_run,&m_acqmode);

    m_state = STATE_RUNNING;
  }

  void HGCalProducer::DoStopRun()
  {
    m_triggerController->stopRun();
    eudaq::mSleep(1000);
    m_state = STATE_GOTOSTOP;

    while (m_state == STATE_GOTOSTOP) {
      eudaq::mSleep(1000); //waiting for EORE being send
    }

    uint32_t trailer=time(0); 
    m_rawFile.write(reinterpret_cast<const char*>(&trailer), sizeof(trailer));
    m_rawFile.close();

    std::cout << "HGCalProducer is fully stopped" << std::endl;
    
  }

  void HGCalProducer::DoTerminate()
  {
    if( m_state!=STATE_UNCONF ){
      auto config = GetConfiguration();
      unsigned int n_orms = config->Get("NumberOfORMs",1);
      std::string syncPi=config->Get("SYNC_Pi_Alias",std::string("piS"));
      std::string rdoutPis=config->Get("RDOUT_Pi_Aliases",std::string("piRBDev"));
      std::istringstream iss(rdoutPis);
      std::vector<std::string> aliases;
      std::copy(std::istream_iterator<std::string>(iss),
		std::istream_iterator<std::string>(),
		back_inserter(aliases));
      stopSoftOnSYNCBoard( syncPi );
      for (unsigned int ipi=0; ipi<n_orms; ipi++)
	stopSoftOnRDOUTBoard( aliases[ipi] );

      m_triggerController->stopRun();
    }
 
    m_state = STATE_GOTOTERM;

    eudaq::mSleep(1000);
  }
}
