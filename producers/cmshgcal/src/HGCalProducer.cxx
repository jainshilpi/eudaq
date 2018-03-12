#include "eudaq/Configuration.hh"
#include "eudaq/Producer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"

#include <iostream>
#include <ostream>
#include <sstream>
#include <vector>
#include <memory>
#include <iomanip>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>

#include <boost/crc.hpp>
#include <boost/cstdint.hpp>
#include <boost/timer/timer.hpp>
#include <boost/thread/thread.hpp>
#include <boost/format.hpp>

#include <TFile.h>
#include <TH1D.h>

#include "IpbusHwController.h"
#include "TriggerController.h"

#define FORMAT_VERSION 1
#define MAX_NUMBER_OF_ORM 16

static const std::string EVENT_TYPE = "HexaBoard";

// void readFIFOThread( ipbus::IpbusHwController* orm, uint32_t *blockSize)
// {
//   orm->ReadDataBlock("FIFO",*blockSize);
// }
void readFIFOThread( ipbus::IpbusHwController* orm)
{
  orm->ReadDataBlock("FIFO");
}

void startTriggerThread( TriggerController* trg_ctrl, uint32_t *run, ACQ_MODE* mode)
{
  trg_ctrl->startrunning( *run, *mode );
}

class HGCalProducer : public eudaq::Producer {
public:
  HGCalProducer(const std::string & name, const std::string & runcontrol) : eudaq::Producer(name, runcontrol)
  {
    m_run=m_ev=0;
    m_uhalLogLevel=5;
    m_blockSize=30787;
    m_state=STATE_UNCONF;
    m_rdoutMask=0x0;
  }

 private:
  unsigned m_run, m_ev, m_uhalLogLevel, m_blockSize;
  uint16_t m_rdoutMask;
  ACQ_MODE m_acqmode;
  
  TriggerController *m_triggerController;
  std::vector< ipbus::IpbusHwController* > m_rdout_orms;
  boost::thread m_triggerThread;
  enum DAQState {
    STATE_ERROR,
    STATE_UNCONF,
    STATE_GOTOCONF,
    STATE_CONFED,
    STATE_GOTORUN,
    STATE_RUNNING,
    STATE_GOTOSTOP,
    STATE_GOTOTERM
  } m_state;
    
  std::ofstream m_rawFile;
  
  TFile *m_outrootfile;
  TH1D *m_hreadouttime;
  TH1D *m_hwritertime;

public:
  bool checkCRC( const std::string & crcNodeName, ipbus::IpbusHwController *ptr )
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

  void MainLoop() 
  {
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
	eudaq::RawDataEvent ev(EVENT_TYPE,m_run,m_ev);
	boost::thread threadVec[m_rdout_orms.size()];
	
	for( int i=0; i<(int)m_rdout_orms.size(); i++)
	  threadVec[i]=boost::thread(readFIFOThread,m_rdout_orms[i]);
	  //threadVec[i]=boost::thread(readFIFOThread,m_rdout_orms[i],&m_blockSize);
	
	for( int i=0; i<(int)m_rdout_orms.size(); i++){
	  threadVec[i].join();}

	times=timerReadout.elapsed();
	m_hreadouttime->Fill(times.wall/1e9);

	int head[1];
	boost::timer::cpu_timer timerWriter;
	for( int i=0; i<(int)m_rdout_orms.size(); i++){
	  std::vector<uint32_t> the_data = m_rdout_orms[i]->getData() ;
	  //  checkCRC( "RDOUT.CRC",m_rdout_orms[i]);
	  uint32_t trailer=i;//8 bits for orm id
	  trailer|=m_triggerController->eventNumber()<<8;//24 bits for trigger number
	  
	  the_data.push_back(trailer);

          head[0] = i+1;
          ev.AddBlock(   2*i, head, sizeof(head));
	  ev.AddBlock( 2*i+1, the_data);

	  std::cout<<"rdout board "<<i<<"  skiroc mask="<<std::setw(8)<<std::setfill('0')<<std::hex<<the_data[0]<<"  Size of the data (bytes): "<<std::dec<<the_data.size()*4<<std::endl;

	  the_data.push_back( m_rdout_orms[i]->ReadRegister("CLK_COUNT0") );
	  the_data.push_back( m_rdout_orms[i]->ReadRegister("CLK_COUNT1") );
	  
          m_rawFile.write(reinterpret_cast<const char*>(&the_data[0]), the_data.size()*sizeof(uint32_t));

	  the_data.pop_back();
	  the_data.pop_back();
	  the_data.pop_back();
	}
	times=timerWriter.elapsed();
	m_hwritertime->Fill(times.wall/1e9);

	m_ev=m_triggerController->eventNumber();
	SendEvent(ev);
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
	SendEvent( eudaq::RawDataEvent::EORE(EVENT_TYPE,m_run,++m_ev) );
	eudaq::mSleep(1000);
	m_state = STATE_CONFED;
	continue;
      }
    };
  }

private:
  virtual void OnInitialise(const eudaq::Configuration &param)
  {
    SetConnectionState(eudaq::ConnectionState::STATE_UNCONF, "Initialised");
  }

  virtual void OnConfigure(const eudaq::Configuration & config) 
  {
    if( m_state == STATE_CONFED ){
      std::cout << "HGCalProducer was already confed -> we try to reset before" << std::endl;
      OnReset();
    }
	
    std::cout << "Configuring: " << config.Name() << std::endl;

    m_rdoutMask = config.Get("RDoutMask",1);
    m_uhalLogLevel = config.Get("UhalLogLevel", 5);
    m_blockSize = config.Get("DataBlockSize",962);
    const int mode=config.Get("AcquisitionMode",0);
    switch( mode ){
    case 0 : m_acqmode = DEBUG; break;
    case 1 : m_acqmode = BEAMTEST; break;
    default : m_acqmode = DEBUG; break;
    }

    std::ostringstream os( std::ostringstream::ate );
    unsigned int bit=1;
    std::cout << "m_rdoutMask = " << m_rdoutMask << std::endl;
    for( int iorm=0; iorm<MAX_NUMBER_OF_ORM; iorm++ ){
      std::cout << "iorm = " << iorm << "..." << std::endl;
      bool activeSlot=(m_rdoutMask&bit);
      bit = bit << 1;
      if(!activeSlot) continue;
      std::cout << "... found in the rdout mask " << std::endl;

      os.str(""); os << "RDOUT_ORM" << iorm;
      //ipbus::IpbusHwController *orm = new ipbus::IpbusHwController(config.Get("ConnectionFile","file://./etc/connection.xml"),os.str());
      ipbus::IpbusHwController *orm = new ipbus::IpbusHwController(config.Get("ConnectionFile","file://./etc/connection.xml"),os.str(),m_blockSize);
      m_rdout_orms.push_back( orm );
    }    
    m_triggerController = new TriggerController(m_rdout_orms);

    for( std::vector<ipbus::IpbusHwController *>::iterator it=m_rdout_orms.begin(); it!=m_rdout_orms.end(); ++it ){
      std::string ormId=(*it)->getInterface()->id();
      std::cout << "ORM " << ormId << "\n"
    		<< "Check0 = " << std::hex << (*it)->ReadRegister("check0") << "\t"
    		<< "Check1 = " << std::hex << (*it)->ReadRegister("check1") << "\t"
    		<< "Check2 = " << std::hex << (*it)->ReadRegister("check2") << "\t"
    		<< "Check3 = " << std::hex << (*it)->ReadRegister("check3") << "\t"
    		<< "Check4 = " << std::hex << (*it)->ReadRegister("check4") << "\t"
    		<< "Check5 = " << std::hex << (*it)->ReadRegister("check5") << "\t"
    		<< "Check6 = " << std::hex << (*it)->ReadRegister("check6") << "\n"
    		<< "Check31 = " << std::dec << (*it)->ReadRegister("check31") << "\t"
    		<< "Check32 = " << std::dec << (*it)->ReadRegister("check32") << "\t"
    		<< "Check33 = " << std::dec << (*it)->ReadRegister("check33") << "\t"
    		<< "Check34 = " << std::dec << (*it)->ReadRegister("check34") << "\t"
    		<< "Check35 = " << std::dec << (*it)->ReadRegister("check35") << "\t"
    		<< "Check36 = " << std::dec << (*it)->ReadRegister("check36") << std::endl;
    
      const uint32_t mask=(*it)->ReadRegister("SKIROC_MASK");
      std::cout << "ORM " << ormId << "\t SKIROC_MASK = " <<std::setw(8)<< std::hex<<mask << std::endl;
      const uint32_t cst0=(*it)->ReadRegister("CONSTANT0");
      std::cout << "ORM " << ormId << "\t CONST0 = " << std::hex << cst0 << std::endl;
      const uint32_t cst1=(*it)->ReadRegister("CONSTANT1");
      std::cout << "ORM " << ormId << "\t CONST1 = " << std::hex << cst1 << std::endl;
    
      boost::this_thread::sleep( boost::posix_time::milliseconds(1000) );
    }    

    m_state=STATE_CONFED;
    SetConnectionState(eudaq::ConnectionState::STATE_CONF, "Configured (" + config.Name() + ")");
}

  virtual void OnStartRun(unsigned param) 
  {
    m_state = STATE_GOTORUN;
    m_run = param;
    m_ev = 0;
    
    SendEvent( eudaq::RawDataEvent::BORE(EVENT_TYPE, m_run) );

    m_outrootfile = new TFile("../data/time.root","RECREATE");
    m_hreadouttime = new TH1D("rdoutTime","",10000,0,1);    
    m_hwritertime = new TH1D("writingTime","",10000,0,1);
    
    char rawFilename[256];
    sprintf(rawFilename, "../raw_data/HexaData_Run%04d.raw", m_run); // The path is relative to eudaq/bin; raw_data is a symbolic link
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
    SetConnectionState(eudaq::ConnectionState::STATE_RUNNING, "Running");
  }

  virtual void OnStopRun() {
    try {
      m_triggerController->stopRun();
      eudaq::mSleep(1000);
      m_state = STATE_GOTOSTOP;
      m_outrootfile->Write();
      m_outrootfile->Close();

      while (m_state == STATE_GOTOSTOP) {
	eudaq::mSleep(1000); //waiting for EORE being send
      }

      uint32_t trailer=time(0); 
      m_rawFile.write(reinterpret_cast<const char*>(&trailer), sizeof(trailer));
      m_rawFile.close();

      SetConnectionState(eudaq::ConnectionState::STATE_CONF, "Stopped");
      
    } catch (const std::exception & e) {
      SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Stop Error: " + eudaq::to_string(e.what()));
    } catch (...) {
      SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Stop Error");
    }
  }

  virtual void OnTerminate() {
    m_triggerController->stopRun();

    m_state = STATE_GOTOTERM;
    
    eudaq::mSleep(1000);
  }

  virtual void OnReset() {
    try {
      std::cout << "Reset : " << std::endl;
      m_triggerController->stopRun();
      std::cout << "m_triggerController->stopRun()...sleep 100 milliseconds" << std::endl;
      boost::this_thread::sleep( boost::posix_time::milliseconds(100) );

      std::cout << "delete Ipbus controllers...sleep 100 milliseconds" << std::endl;
      for( std::vector<ipbus::IpbusHwController *>::iterator it=m_rdout_orms.begin(); it!=m_rdout_orms.end(); ++it ){
	delete (*it);
      }
      m_rdout_orms.clear();
      delete m_triggerController;
      SetConnectionState(eudaq::ConnectionState::STATE_UNCONF, "Reset");
    } catch (const std::exception &e) {
      printf("Caught exception: %s\n", e.what());
      SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Reset Error");
    } catch (...) {
      printf("Unknown exception\n");
      SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Reset Error");
    }
  }

};

int main(int /*argc*/, const char ** argv) {
  eudaq::OptionParser op("HGCal Producer", "1.0", "The Producer task for the CMS-HGCal prototype");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol", "tcp://localhost:44000", "address", "The address of the RunControl application");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level", "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name (op, "n", "name", "CMS-HGCAL", "string", "The name of this Producer");
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    HGCalProducer producer(name.Value(),rctrl.Value());
    std::cout << "on demarre la main loop" << std::endl;
    producer.MainLoop();
    std::cout << "Quitting" << std::endl;
    eudaq::mSleep(300);
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
