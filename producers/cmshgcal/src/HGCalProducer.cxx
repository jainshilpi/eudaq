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
#include <boost/tokenizer.hpp>

#include <TFile.h>
#include <TH1D.h>

#include "IpbusHwController.h"
#include "HGCalController.h"
#include "compressor.h"

#define FORMAT_VERSION 1
#define MAX_NUMBER_OF_ORM 16

static const std::string EVENT_TYPE = "HexaBoard";
static uint32_t runID=0;
void startHGCCtrlThread(HGCalController* ctrl)
{
  ctrl->startrun(runID);
}

class HGCalProducer : public eudaq::Producer {
public:
  HGCalProducer(const std::string & name, const std::string & runcontrol) : eudaq::Producer(name, runcontrol)
  {
    m_run=m_ev=0;
    m_state=STATE_UNCONF;
  }

 private:
  unsigned m_run, m_ev;
  bool m_doCompression;
  int m_compressionLevel;
  
  HGCalController *m_hgcController;
  boost::thread m_hgcCtrlThread;
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
      
public:

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
	if( m_hgcController->getDequeData().empty() ){
	  boost::this_thread::sleep( boost::posix_time::microseconds(100) );
	  continue;
	}
	eudaq::RawDataEvent ev(EVENT_TYPE,m_run,m_ev);
	m_ev++;
	HGCalDataBlocks dataBlk=m_hgcController->getDequeData()[0];
	int iblock(0);
	int head[1];
	for( std::map< int,std::vector<uint32_t> >::iterator it=dataBlk.getData().begin(); it!=dataBlk.getData().end(); ++it ){
	  head[0]=iblock+1;
	  ev.AddBlock( 2*iblock,head,sizeof(head) );
	  if( m_doCompression==true ){
	    std::string compressedData=compressor::compress(it->second,m_compressionLevel);
	    ev.AddBlock( 2*iblock+1,compressedData.c_str(), compressedData.size() );
	    std::cout<<"Running:  skiroc mask="<<std::setw(8)<<std::setfill('0')<<std::hex<<it->second[0]<<"  Size of the data (bytes): "<<std::dec<<it->second.size()*4<<"  compressedData.size() = " << compressedData.size() << std::endl;
	  }
	  else
	    ev.AddBlock( 2*iblock+1,it->second );
	  iblock++;
	}
	m_hgcController->getDequeData().pop_front();
	
	SendEvent(ev);
      }
      if (m_state == STATE_GOTOSTOP) {
	while(1){
	  if( m_hgcController->getDequeData().empty() ){
	    break;
	  }
	  std::cout<<"Deque size = " << m_hgcController->getDequeData().size() << std::endl;
	  eudaq::RawDataEvent ev(EVENT_TYPE,m_run,m_ev);
	  m_ev++;
	  HGCalDataBlocks dataBlk=m_hgcController->getDequeData()[0];
	  int iblock(0);
	  int head[1];
	  for( std::map< int,std::vector<uint32_t> >::iterator it=dataBlk.getData().begin(); it!=dataBlk.getData().end(); ++it ){
	    head[0]=iblock+1;
	    ev.AddBlock( 2*iblock,head,sizeof(head) );
	    if( m_doCompression==true ){
	      std::string compressedData=compressor::compress(it->second,m_compressionLevel);
	      ev.AddBlock( 2*iblock+1,compressedData.c_str(), compressedData.size() );
	    }
	    else
	      ev.AddBlock( 2*iblock+1,it->second );
	    iblock++;
	  }
	  m_hgcController->getDequeData().pop_front();
	  SendEvent(ev);
	}

	SendEvent( eudaq::RawDataEvent::EORE(EVENT_TYPE,m_run,m_ev) );
	eudaq::mSleep(1000);
	m_state = STATE_CONFED;
	continue;
      }
    };
  }

private:
  virtual void OnInitialise(const eudaq::Configuration &param)
  {
    EUDAQ_INFO("HGCalProducer received initialise command");
    m_hgcController=new HGCalController();
    m_hgcController->init();
    SetConnectionState(eudaq::ConnectionState::STATE_UNCONF, "Initialised");
  }

  virtual void OnConfigure(const eudaq::Configuration & config) 
  {
    EUDAQ_INFO("HGCalProducer receive configure command with configuration file : "+config.Name());

    m_doCompression = config.Get("DoCompression",false);
    m_compressionLevel = config.Get("CompressionLevel",5);

    if(m_doCompression)
      std::cout << "We will perform compression with compression level = " << m_compressionLevel << std::endl;
    else
      std::cout << "We will not perform any compression" << std::endl;
    
    HGCalConfig hgcConfig;
    hgcConfig.connectionFile = config.Get("ConnectionFile","file://./etc/connection.xml");
    hgcConfig.rootFilePath = config.Get("TimingRootFilePath","./data_root");
    hgcConfig.rdoutMask = config.Get("RDoutMask",4);
    hgcConfig.blockSize = config.Get("DataBlockSize",30787);
    hgcConfig.uhalLogLevel = config.Get("UhalLogLevel", 5);
    hgcConfig.timeToWaitAtEndOfRun = config.Get("TimeToWaitAtEndOfRun", 1000);
    hgcConfig.saveRawData = config.Get("saveRawData", false);
    hgcConfig.checkCRC = config.Get("checkCRC", false);
    hgcConfig.throwFirstTrigger = config.Get("throwFirstTrigger", true);
    hgcConfig.saveRootFile = config.Get("SaveRootFile", true);

    std::string listOfRdoutBoards = config.Get("RDoutBoards", "RDOUT_ORM0");
    hgcConfig.listOfRdoutBoards.clear();
    typedef boost::tokenizer<boost::escaped_list_separator<char> > tokenizer;
    tokenizer tok(listOfRdoutBoards);
    for (tokenizer::iterator it=tok.begin(); it!=tok.end(); ++it)
      hgcConfig.listOfRdoutBoards.push_back(*it) ;
    
    m_hgcController->configure(hgcConfig);

    m_state=STATE_CONFED;
    SetConnectionState(eudaq::ConnectionState::STATE_CONF, "Configured (" + config.Name() + ")");
  }

  virtual void OnStartRun(unsigned param) 
  {
    m_state = STATE_GOTORUN;
    m_run = param;
    runID=m_run;
    m_ev = 0;

    EUDAQ_INFO("HGCalProducer received start run command");

    m_hgcCtrlThread=boost::thread(startHGCCtrlThread,m_hgcController);

    SendEvent( eudaq::RawDataEvent::BORE(EVENT_TYPE, m_run) );
    m_state = STATE_RUNNING;
    SetConnectionState(eudaq::ConnectionState::STATE_RUNNING, "Running");
  }

  virtual void OnStopRun() {
    EUDAQ_INFO("HGCalProducer received stop run command");
    try {
      m_hgcController->stoprun();
      eudaq::mSleep(1000);
      m_state = STATE_GOTOSTOP;
      while (m_state == STATE_GOTOSTOP) {
	eudaq::mSleep(1000); //waiting for EORE being send
      }
      SetConnectionState(eudaq::ConnectionState::STATE_CONF, "Stopped");
    } catch (const std::exception & e) {
      SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Stop Error: " + eudaq::to_string(e.what()));
    } catch (...) {
      SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Stop Error");
    }
  }

  virtual void OnTerminate() {
    EUDAQ_INFO("HGCalProducer received terminate command");
    if(NULL!=m_hgcController){
      m_hgcController->stoprun();
      eudaq::mSleep(1000);
      m_state = STATE_GOTOSTOP;
      while (m_state == STATE_GOTOSTOP) {
	eudaq::mSleep(1000); //waiting for EORE being send
      }
      m_hgcController->terminate();
    }
    m_state = STATE_GOTOTERM;
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
