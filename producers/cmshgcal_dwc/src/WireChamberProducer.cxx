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

#include "CAEN_v1290.h"

#include <chrono>

enum RUNMODE{
  DWC_DEBUG = 0,
  DWC_RUN
};


static const std::string EVENT_TYPE = "DWC";

class WireChamberProducer : public eudaq::Producer {
  public:

  WireChamberProducer(const std::string & name, const std::string & runcontrol)
    : eudaq::Producer(name, runcontrol), m_run(0), m_ev(0), stopping(false), done(false), started(0) {      
      initialized = false;
      _mode = DWC_DEBUG;
      NumberOfTDCs=-1;
    }

  void OnInitialise(const eudaq::Configuration &init){
    std::cout<<"Initialisation of the DWC Producer..."<<std::endl;
    try {
      std::cout << "Reading: " << init.Name() << std::endl;
      //necessary: setup the communication board (VX2718)
      //corresponding values for the init function are taken from September 2016 configuration
      //https://github.com/cmsromadaq/H4DAQ/blob/master/data/H2_2016_08_HGC/config_pcminn03_RC.xml#L26
      VX2718handle = new int;
      int status = CAENVME_Init(static_cast<CVBoardTypes>(1), 0, 0, VX2718handle); 
      if (status) {
        std::cout << "[CAEN_VX2718]::[ERROR]::Cannot open VX2718 board." << std::endl;
      } 
      
      // Do any initialisation of the hardware here 
      // "start-up configuration", which is usally done only once in the beginning
      // Configuration file values are accessible as config.Get(name, default)

      // At the end, set the ConnectionState that will be displayed in the Run Control.
      // and set the state of the machine.
      SetConnectionState(eudaq::ConnectionState::STATE_UNCONF, "Initialised (" + init.Name() + ")");
    } 
    catch (...) {
      std::cout << "Unknown exception" << std::endl;
      EUDAQ_ERROR("Error occurred in initialization phase of DWCProducer");
      SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Initialisation Error");
    } 
  }    


  virtual void OnConfigure(const eudaq::Configuration & config) {
    SetConnectionState(eudaq::ConnectionState::STATE_UNCONF, "Configuring (" + config.Name() + ")");
    std::cout << "Configuring: " << config.Name() << std::endl;


    int mode = config.Get("AcquisitionMode", 0);

    switch( mode ){
      case 0 :
        _mode = DWC_DEBUG;
        break;
      case 1:
      default :
        _mode = DWC_RUN;
        break;
    }
    std::cout<<"Mode at configuration: "<<_mode<<std::endl;


    //clear the TDCs
    
    if (NumberOfTDCs==-1) {   //do not allow for dynamic changing of TDCs because the number of DQM plots depend on it and are determined at first runtime.
      for (size_t i=0; i<tdcs.size(); i++) delete tdcs[i];
      tdcs.clear();
      NumberOfTDCs = config.Get("NumberOfTDCs", 1);
      for (int id=1; id<=NumberOfTDCs; id++)
        tdcs.push_back(new CAEN_V1290(id));      
    } else {
      std::cout<<"Number of TDCs(="<<NumberOfTDCs<<") has not been changed. Restart the producer to change the number of TDCs."<<std::endl;
    }

    if (!initialized){           
      bool tdcs_initialized = true;
      for (int i=0; i<NumberOfTDCs; i++) {
          tdcs[i]->SetHandle(*VX2718handle);
          tdcs_initialized = tdcs[i]->Init() && tdcs_initialized;
      }
      initialized = tdcs_initialized;
    }


    for (int i=0; i<NumberOfTDCs; i++) {
      if (_mode == DWC_RUN) {
        if (initialized) {
          CAEN_V1290::CAEN_V1290_Config_t _config;
          _config.baseAddress = config.Get(("baseAddress_"+std::to_string(i+1)).c_str(), 0x00AA0000);
          _config.model = static_cast<CAEN_V1290::CAEN_V1290_Model_t>(config.Get(("model_"+std::to_string(i+1)).c_str(), 1));
          _config.triggerTimeSubtraction = static_cast<bool>(config.Get(("triggerTimeSubtraction_"+std::to_string(i+1)).c_str(), 1));
          _config.triggerMatchMode = static_cast<bool>(config.Get(("triggerMatchMode_"+std::to_string(i+1)).c_str(), 1));
          _config.emptyEventEnable = static_cast<bool>(config.Get(("emptyEventEnable_"+std::to_string(i+1)).c_str(), 1));
          _config.edgeDetectionMode = static_cast<CAEN_V1290::CAEN_V1290_EdgeDetection_t>(config.Get(("edgeDetectionMode_"+std::to_string(i+1)).c_str(), 3));
          _config.timeResolution = static_cast<CAEN_V1290::CAEN_V1290_TimeResolution_t>(config.Get(("timeResolution_"+std::to_string(i+1)).c_str(), 3));
          _config.maxHitsPerEvent = static_cast<CAEN_V1290::CAEN_V1290_MaxHits_t>(config.Get(("maxHitsPerEvent_"+std::to_string(i+1)).c_str(), 8));
          _config.enabledChannels = config.Get(("enabledChannels_"+std::to_string(i+1)).c_str(), 0x00FF);
          _config.windowWidth = config.Get(("windowWidth_"+std::to_string(i+1)).c_str(), 0x40);
          _config.windowOffset = config.Get(("windowOffset_"+std::to_string(i+1)).c_str(), -1);
          tdcs[i]->Config(_config);
          tdcs[i]->SetupModule();
        }
      }      
    }    
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

    if (_mode==DWC_RUN) {
      if (initialized)
        for (size_t i=0; i<tdcs.size(); i++) tdcs[i]->BufferClear();
      else {
        EUDAQ_INFO("ATTENTION !!! Communication to the TDC has not been established");
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

    for (size_t i=0; i<tdcs.size(); i++) delete tdcs[i];

  }


  void ReadoutLoop() {

    while(!done) {
      if (!started) {
        eudaq::mSleep(200);
        continue;
      }

      if (stopping) continue;

      if (_mode==DWC_RUN) {
        bool performReadout = false;
        for (int i=0; i<tdcs.size(); i++) {
          performReadout = tdcs[i]->ReadyToRead();
          if (performReadout) break;
        }
        
        if(!performReadout) continue;        
      }

      m_ev++;
      //get the timestamp since start:
      timeSinceStart = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - startTime).count();
      std::cout<<"+++ Event: "<<m_ev<<": "<<timeSinceStart/1000.<<" ms +++"<<std::endl;
      //making an EUDAQ event
      eudaq::RawDataEvent ev(EVENT_TYPE,m_run,m_ev);
      ev.setTimeStamp(timeSinceStart);
      
      for (int i=0; i<tdcs.size(); i++) {
        dataStream.clear();
        if (_mode==DWC_RUN && initialized) {
          tdcs[i]->Read(dataStream);
        } else if (_mode==DWC_DEBUG) {
          tdcs[i]->generatePseudoData(dataStream);
        }
          
        ev.AddBlock(i, dataStream);
      }

      //Adding the event to the EUDAQ format
      SendEvent(ev);
      if (_mode==DWC_DEBUG) eudaq::mSleep(10);
    }
  }

  private:
    RUNMODE _mode;

    int *VX2718handle;

    unsigned m_run, m_ev;
    bool stopping, done, started;
    bool initialized;

    std::chrono::steady_clock::time_point startTime; 
    uint64_t timeSinceStart;
 
    //set on configuration
    int NumberOfTDCs;
    std::vector<CAEN_V1290*> tdcs;
    
    std::vector<WORD> dataStream;

};

// The main function that will create a Producer instance and run it
int main(int /*argc*/, const char ** argv) {
  // You can use the OptionParser to get command-line arguments
  // then they will automatically be described in the help (-h) option
  eudaq::OptionParser op("Delay Wire Chamber Producer", "0.1",
  "Just an example, modify it to suit your own needs");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol",
  "tcp://localhost:44000", "address",
  "The address of the RunControl.");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
  "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name (op, "n", "name", "DWCs", "string",
  "The name of this Producer.");

  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    EUDAQ_INFO("Starting the producer");
    WireChamberProducer producer(name.Value(), rctrl.Value());
    producer.ReadoutLoop();
    EUDAQ_INFO("Quitting");
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
