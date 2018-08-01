//a producer reading out MCP signals from a CAEN V1742, first used in the HGCal beam tests in October 2018
//Thorben Quast, thorben.quast@cern.ch
//01 August 2018

#include "eudaq/Configuration.hh"
#include "eudaq/Producer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"
#include <iostream>
#include <ostream>
#include <cstdlib>
#include <string>
#include <vector>


#include <TFile.h>
#include <TTree.h>

#include "CAEN_v1742.h"

#include <chrono>

enum RUNMODE{
  MCP_DEBUG = 0,
  MCP_RUN
};


static const std::string EVENT_TYPE = "MCP";

class MCPProducer : public eudaq::Producer {
  public:

  MCPProducer(const std::string & name, const std::string & runcontrol)
    : eudaq::Producer(name, runcontrol), m_run(0), m_ev(0), stopping(false), done(false), started(0) {      
      initialized = false;
      _mode = MCP_DEBUG;
      CAENV1742_instance = new CAEN_V1742(0);
      std::cout<<"Initialisation of the MCP Producer..."<<std::endl;
    }

  virtual void OnConfigure(const eudaq::Configuration & config) {
    SetConnectionState(eudaq::ConnectionState::STATE_UNCONF, "Configuring (" + config.Name() + ")");
    std::cout << "Reading configuration file: " << config.Name() << std::endl;

    int mode = config.Get("AcquisitionMode", 0);

    //todo for 02 August and later: 
    //- extend the configuration
    //- take some data in the lab
    //- write the unpacking
    CAEN_V1742::CAEN_V1742_Config_t _config;
    _config.BaseAddress = config.Get("BASEADDRESS", 0x33330000);
     std::string link_type_str = config.Get("LINK_TYPE", "OpticalLink");
    if (link_type_str=="OpticalLink") _config.LinkType = CAEN_DGTZ_PCI_OpticalLink;
    else if (link_type_str=="USB") _config.LinkType = CAEN_DGTZ_USB;
    else std::cout<<"Unknown link type: "<<link_type_str<<std::endl;
    _config.LinkNum = config.Get("LINK_NUM", 0);
    _config.ConetNode = config.Get("CONET_NODE", 0);

    //Remaining items to be added in the configuration
    /*  
    int Nch ;
    int Nbit ;
    float Ts ;
    int RecordLength ;
    CAEN_DGTZ_DRS4Frequency_t DRS4Frequency;
    unsigned int PostTrigger ;
    int NumEvents ;
    int InterruptNumEvents ; 
    int TestPattern ;
    int DesMode ;
    int TriggerEdge ;
    int FPIOtype ;
    CAEN_DGTZ_TriggerMode_t ExtTriggerMode ;
    uint8_t EnableMask ;
    CAEN_DGTZ_TriggerMode_t ChannelTriggerMode[CAEN_V1742_MAXSET] ;
    uint32_t DCoffset[CAEN_V1742_MAXSET] ;
    int32_t  DCoffsetGrpCh[CAEN_V1742_MAXSET][CAEN_V1742_MAXCH] ;
    uint32_t Threshold[CAEN_V1742_MAXSET] ;
    uint8_t GroupTrgEnableMask[CAEN_V1742_MAXSET] ;
    uint32_t FTDCoffset[CAEN_V1742_MAXSET] ;
    uint32_t FTThreshold[CAEN_V1742_MAXSET] ;
    CAEN_DGTZ_TriggerMode_t FastTriggerMode ;
    uint32_t   FastTriggerEnabled ;
    int GWn ;
    uint32_t GWaddr[CAEN_V1742_MAXGW] ;
    uint32_t GWdata[CAEN_V1742_MAXGW] ;
    int useCorrections ;
    */











    switch( mode ){
      case 0 :
        _mode = MCP_DEBUG;
        break;
      case 1:
      default :
        _mode = MCP_RUN;
        break;
    }
    std::cout<<"Mode at configuration: "<<_mode<<std::endl;

    

    CAENV1742_instance->Config(_config);
    CAENV1742_instance->Print(0);
    initialized = true;
    SetConnectionState(eudaq::ConnectionState::STATE_CONF, "Configured (" + config.Name() + ")");

  }

  // This gets called whenever a new run is started
  // It receives the new run number as a parameter
  virtual void OnStartRun(unsigned param) {
    m_run = param;
    m_ev = 0;

    EUDAQ_INFO("Start Run: "+param);
    // It must send a BORE to the Data Collector
    eudaq::RawDataEvent bore(eudaq::RawDataEvent::BORE(EVENT_TYPE, m_run));
    SendEvent(bore);

    if (_mode==MCP_RUN) {
      if (initialized)
        EUDAQ_INFO("Initialisation ok.");
      else {
        EUDAQ_INFO("ATTENTION !!! Communication to the MCP has not been established");
        SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Communication to the TDC has not been established");
      }
    }

    SetConnectionState(eudaq::ConnectionState::STATE_RUNNING, "Running");
    startTime = std::chrono::steady_clock::now();
    started=true;
  }

  // This gets called whenever a run is stopped
  virtual void OnStopRun() {
    SetConnectionState(eudaq::ConnectionState::STATE_CONF, "Stopping");
    EUDAQ_INFO("Stopping Run");
    started=false;
    // Set a flag tao signal to the polling loop that the run is over
    stopping = true;

    // Send an EORE after all the real events have been sent
    // You can also set tags on it (as with the BORE) if necessary
    SendEvent(eudaq::RawDataEvent::EORE("Test", m_run, ++m_ev));

    
    stopping = false;
    SetConnectionState(eudaq::ConnectionState::STATE_CONF, "Stopped");
  }

  // This gets called when the Run Control is terminating,
  // we should also exit.
  virtual void OnTerminate() {
    EUDAQ_INFO("Terminating...");
    done = true;
    eudaq::mSleep(200);
  }


  void ReadoutLoop() {

    while(!done) {
      if (!started) {
        eudaq::mSleep(200);
        continue;
      }

      if (stopping) continue;

      if (_mode==MCP_RUN) {
        bool performReadout = false;
        if(!performReadout) continue;        
      } else eudaq::mSleep(1000);

      m_ev++;
      //get the timestamp since start:
      timeSinceStart = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - startTime).count();
      std::cout<<"+++ Event: "<<m_ev<<": "<<timeSinceStart/1000.<<" ms +++"<<std::endl;
      //making an EUDAQ event
      eudaq::RawDataEvent ev(EVENT_TYPE,m_run,m_ev);
      ev.setTimeStamp(timeSinceStart);
      
      
      
      //readout magic will go here
      
      
          
      ev.AddBlock(0, dataStream);
      //Adding the event to the EUDAQ format
      SendEvent(ev);

    }
  }

  private:
    RUNMODE _mode;

    unsigned m_run, m_ev;
    bool stopping, done, started;
    bool initialized;

    std::chrono::steady_clock::time_point startTime; 
    uint64_t timeSinceStart;
 
    
    CAEN_V1742* CAENV1742_instance;
    std::vector<WORD> dataStream;

};

// The main function that will create a Producer instance and run it
int main(int /*argc*/, const char ** argv) {
  // You can use the OptionParser to get command-line arguments
  // then they will automatically be described in the help (-h) option
  eudaq::OptionParser op("MCP Producer", "0.1",
  "Just an example, modify it to suit your own needs");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol",
  "tcp://localhost:44000", "address",
  "The address of the RunControl.");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
  "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name (op, "n", "name", "MCP", "string",
  "The name of this Producer.");

  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    EUDAQ_INFO("Starting the producer");
    MCPProducer producer(name.Value(), rctrl.Value());
    producer.ReadoutLoop();
    EUDAQ_INFO("Quitting");
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
