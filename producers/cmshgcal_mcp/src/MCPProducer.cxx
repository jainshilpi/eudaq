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
#include <sstream>


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
      std::cout<<"Initialisation of the MCP Producer..."<<std::endl;
      _mode = MCP_DEBUG;
      CAENV1742_instance = new CAEN_V1742(0);
      CAENV1742_instance->Init();
    }

  virtual void OnConfigure(const eudaq::Configuration & config) {
    SetConnectionState(eudaq::ConnectionState::STATE_UNCONF, "Configuring (" + config.Name() + ")");
    CAENV1742_instance->setDefaults();

    std::cout << "Reading configuration file: " << config.Name() << std::endl;

    int mode = config.Get("AcquisitionMode", 0);
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



    //read configuration specific to the board
    CAEN_V1742::CAEN_V1742_Config_t _config;
    

    // addressing configurations
    _config.BaseAddress = config.Get("BASEADDRESS", 0x33330000);
     std::string link_type_str = config.Get("LINK_TYPE", "OpticalLink");
    if (link_type_str=="OpticalLink") _config.LinkType = CAEN_DGTZ_PCI_OpticalLink;
    else if (link_type_str=="USB") _config.LinkType = CAEN_DGTZ_USB;
    else std::cout<<"Unknown link type: "<<link_type_str<<std::endl;
    _config.LinkNum = config.Get("LINK_NUM", 0);
    _config.ConetNode = config.Get("CONET_NODE", 0);

    std::string useCorrections_string = config.Get("CORRECTION_LEVEL", "AUTO");
    if (useCorrections_string == "AUTO") _config.useCorrections = -1 ;
    else                  _config.useCorrections = atoi (useCorrections_string.c_str ()) ;
    

    // RECORD_LENGTH = number of samples in the acquisition window 
    // For the models 742 the options available are only 1024, 520, 256 and 136
    _config.RecordLength = config.Get("RECORD_LENGTH", 1024);


    // TEST_PATTERN: if enabled, data from ADC are replaced by test pattern (triangular wave)                                         
    // options: YES, NO     
    if (config.Get("TEST_PATTERN", "NO") == "YES") _config.TestPattern = 1 ;
    else _config.TestPattern = 0 ;

    // DRS4_Frequency.  Values: 0 = 5 GHz, 1 = 2.5 GHz,  2 = 1 GHz
    _config.DRS4Frequency = (CAEN_DGTZ_DRS4Frequency_t)config.Get("DRS4_Frequency", 0);

    // EXTERNAL_TRIGGER: external trigger input settings. When enabled, the ext. trg. can be either                                   
    // propagated (ACQUISITION_AND_TRGOUT) or not (ACQUISITION_ONLY) through the TRGOUT                                               
    // options: DISABLED, ACQUISITION_ONLY, ACQUISITION_AND_TRGOUT                                                                          
    std::string externalTrigger_string = config.Get("EXTERNAL_TRIGGER", "DISABLED");
    if (externalTrigger_string == "ACQUISITION_ONLY")       
      _config.ExtTriggerMode = CAEN_DGTZ_TRGMODE_ACQ_ONLY ;
    else if (externalTrigger_string == "ACQUISITION_AND_TRGOUT") 
      _config.ExtTriggerMode = CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT ;
    else if (externalTrigger_string == "DISABLED")                    
      _config.ExtTriggerMode = CAEN_DGTZ_TRGMODE_DISABLED ;
    else {
      std::cout<< "[CAEN_V1742]::[WARNING]:: EXTERNAL_TRIGGER " << externalTrigger_string << " is an invalid option"<<std::endl;
      _config.ExtTriggerMode = CAEN_DGTZ_TRGMODE_DISABLED ;
    }                   

    //FAST_TRIGGER: fast trigger input settings. ONLY FOR 742 MODELS. When enabled, the fast trigger is used for the data acquisition
    //options: DISABLED, ACQUISITION_ONLY                                                                                            
    std::string fastTrigger_string = config.Get("FAST_TRIGGER", "ACQUISITION_ONLY");
    if (fastTrigger_string == "ACQUISITION_ONLY")       
      _config.FastTriggerMode = CAEN_DGTZ_TRGMODE_ACQ_ONLY ;
    else if (fastTrigger_string == "DISABLED")                    
      _config.FastTriggerMode = CAEN_DGTZ_TRGMODE_DISABLED ;
    else {
      std::cout<< "[CAEN_V1742]::[WARNING]:: FAST_TRIGGER " << fastTrigger_string << " is an invalid option"<<std::endl;
      _config.FastTriggerMode = CAEN_DGTZ_TRGMODE_DISABLED ;
    }   

    // ENABLED_FAST_TRIGGER_DIGITIZING: ONLY FOR 742 MODELS. If enabled the fast trigger signal is digitized and it is present in data readout as channel n.8 for each group.
    // options: YES, NO    
    if (config.Get("ENABLED_FAST_TRIGGER_DIGITIZING", "NO") == "YES") _config.FastTriggerEnabled = 1 ;
    else _config.FastTriggerEnabled = 0 ;                                                                                                           



    // MAX_NUM_EVENTS_BLT: maximum number of events to read out in one Block Transfer. 
    // options: 1 to 1023    
    _config.NumEvents = config.Get("MAX_NUM_EVENTS_BLT", 1);


    // POST_TRIGGER: post trigger size in percent of the whole acquisition window                                                            
    // options: 0 to 100                                                                                                                     
    // On models 742 there is a delay of about 35nsec on signal Fast Trigger TR; the post trigger is added to this delay      
    _config.PostTrigger = config.Get("POST_TRIGGER", 15);


    // TRIGGER_EDGE: decides whether the trigger occurs on the rising or falling edge of the signal                                          
    // options: RISING, FALLING  
    std::string triggerEdge_string = config.Get("TRIGGER_EDGE", "FALLING");
    if (triggerEdge_string == "FALLING")       
      _config.TriggerEdge = 1 ;
    else if (triggerEdge_string == "RISING")                    
      _config.TriggerEdge = 0 ;
    else {
      std::cout<< "[CAEN_V1742]::[WARNING]:: TRIGGER_EDGE " << triggerEdge_string << " is an invalid option"<<std::endl;
      _config.TriggerEdge = 1 ;
    }   


    // USE_INTERRUPT: number of events that must be ready for the readout when the IRQ is asserted.                                          
    // Zero means that the interrupts are not used (readout runs continuously)      
    _config.InterruptNumEvents = config.Get("USE_INTERRUPT", 0);


    // FPIO_LEVEL: type of the front panel I/O LEMO connectors                                                                               
    // options: NIM, TTL       
    std::string fpiolevel_string = config.Get("FPIO_LEVEL", "NIM");
    if (fpiolevel_string == "NIM")       
      _config.FPIOtype = 0 ;
    else if (fpiolevel_string == "TTL")                    
      _config.FPIOtype = 1 ;
    else {
      std::cout<< "[CAEN_V1742]::[WARNING]:: FPIO_LEVEL " << fpiolevel_string << " is an invalid option"<<std::endl;
      _config.FPIOtype = 0 ;
    }   


    // ENABLE_INPUT: enable/disable one channel (or one group in the case of the Mod 740 and Mod 742)                                                
    // options: YES, NO       
    if (config.Get("ENABLE_INPUT", "YES") == "YES") _config.EnableMask = 0xFF ;
    else _config.EnableMask = 0x00 ;   



    // CHANNEL_TRIGGER: channel auto trigger settings. When enabled, the ch. auto trg. can be either                                                 
    // propagated (ACQUISITION_AND_TRGOUT) or not (ACQUISITION_ONLY) through the TRGOUT                                                              
    // options: DISABLED, ACQUISITION_ONLY, ACQUISITION_AND_TRGOUT          
    std::string channelTrigger_string = config.Get("CHANNELS_TRIGGER", "DISABLED");
    CAEN_DGTZ_TriggerMode_t tm ;
    if (channelTrigger_string == "ACQUISITION_ONLY")       
      tm = CAEN_DGTZ_TRGMODE_ACQ_ONLY ;
    else if (channelTrigger_string == "ACQUISITION_AND_TRGOUT") 
      tm = CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT ;
    else if (channelTrigger_string == "DISABLED")                    
      tm = CAEN_DGTZ_TRGMODE_DISABLED ;
    else {
      std::cout<< "[CAEN_V1742]::[WARNING]:: CHANNELS_TRIGGER " << channelTrigger_string << " is an invalid option"<<std::endl;
      tm = CAEN_DGTZ_TRGMODE_DISABLED ;
    }  
    for (int i = 0 ; i < CAEN_V1742_MAXSET ; ++i) _config.ChannelTriggerMode[i] = tm;


    // DC_OFFSET: DC offset adjust (DAC channel setting) in percent of the Full Scale.                                                               
    // For model 740 and 742* the DC offset adjust is the same for all channel in the group                                                          
    // -50: analog input dynamic range = -Vpp to 0 (negative signals)*                                                                               
    // +50: analog input dynamic range = 0 to +Vpp (positive signals)*                                                                               
    // 0:   analog input dynamic range = -Vpp/2 to +Vpp/2 (bipolar signals)*                                                                         
    // options: -50.0 to 50.0  (floating point)                                                                                                      
    // *NOTE: Ranges are different for 742 Model.....see GRP_CH_DC_OFFSET description    
    uint32_t val = (uint32_t) ( (config.Get("DC_OFFSET_CHANNELS", 0)+50) * 65535 / 100) ;
    for (int i = 0 ; i < CAEN_V1742_MAXSET ; ++i) _config.DCoffset[i] = val ;

  
    //same as DC_OFFSET_CHANNELS but for the trigger, scale is absolute and not relative
    val = config.Get("DC_OFFSET_TRIGGERS", 0);
    for (int i = 0 ; i < CAEN_V1742_MAXSET ; ++i) _config.FTDCoffset[i] = val ;


    // GROUP_TRG_ENABLE_MASK: this option is used only for the Models x740. These models have the                                                  
    // channels grouped 8 by 8; one group of 8 channels has a common trigger that is generated as                                                  
    // the OR of the self trigger of the channels in the group that are enabled by this mask.                                                      
    // options: 0 to FF        
    val = config.Get("GROUP_TRG_ENABLE_MASK", 0xFF);
    for (int i = 0 ; i < CAEN_V1742_MAXSET ; ++i) _config.GroupTrgEnableMask[i] = val  & 0xFF;


    // TRIGGER_THRESHOLD: threshold for the channel auto trigger (ADC counts)                                                                        
    // options 0 to 2^N-1 (N=Number of bit of the ADC)    
    val = config.Get("TRIGGER_THRESHOLD_CHANNELS", 100);
    for (int i = 0 ; i < CAEN_V1742_MAXSET ; ++i) _config.Threshold[i] = val;

    val = config.Get("TRIGGER_THRESHOLD_TRIGGERS", 32768);
    for (int i = 0 ; i < CAEN_V1742_MAXSET ; ++i) _config.FTThreshold[i] = val;



    // GRP_CH_DC_OFFSET dc_0, dc_1, dc_2, dc_3, dc_4, dc_5, dc_6, dc_7                                                                             
    // Available only for model 742, allows to set different DC offset adjust for each channel (DAC channel setting) in percent of the Full Scale.
    // -50: analog input dynamic range = -3Vpp/2 to -Vpp/2 (max negative dynamic)                                                                  
    // +50: analog input dynamic range = +Vpp/2 to +3Vpp/2 (max positive dynamic)                                                                  
    // 0: analog input dynamic range = -Vpp/2 to +Vpp/2 (bipolar signals)                                                                          
    // options: -50.0 to 50.0  (floating point)      
    std::cout<<CAEN_V1742_MAXSET<<"  "<<CAEN_V1742_MAXCH<<std::endl;


    std::stringstream ss("GRP_CH_DC_OFFSET");
    for (int i = 0 ; i < CAEN_V1742_MAXSET ; ++i)  {
      for (int k=0; k < CAEN_V1742_MAXCH; ++k) {
        ss.str("");ss<<"GRP_CH_DC_OFFSET_"<<i<<"_"<<k;
        int dc_val = (int) ( (config.Get(ss.str().c_str(), 0) +50) * 65535 / 100) ; 
        _config.DCoffsetGrpCh[i][k] = dc_val ;
      }
    }

    

    CAENV1742_instance->Config(_config);
    CAENV1742_instance->Print(0);
    
    if (mode==MCP_RUN) {
      if(!initialized) CAENV1742_instance->Init(); //will be executed only once after the intitial configuration
      CAENV1742_instance->SetupModule();
    }


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
        CAENV1742_instance->StartAcquisition();
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

    if (_mode==MCP_RUN) {
      if (initialized)
        CAENV1742_instance->StopAcquisition();
      else {
        EUDAQ_INFO("ATTENTION !!! Communication to the MCP has not been established");
        SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Communication to the TDC has not been established");
      }
    }

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
        CAENV1742_instance->Read(dataStream);
        if(dataStream.size()==0) {
          eudaq::mSleep(0.1);   //100 micro seconds of waiting
          continue;
        }        
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
      dataStream.clear();

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
