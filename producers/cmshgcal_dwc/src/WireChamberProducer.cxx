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
#include "Unpacker.h"

#include <chrono>

enum RUNMODE{
  DWC_DEBUG = 0,
  DWC_RUN
};


static const std::string EVENT_TYPE = "WireChambers";

class WireChamberProducer : public eudaq::Producer {
  public:

  WireChamberProducer(const std::string & name, const std::string & runcontrol)
    : eudaq::Producer(name, runcontrol), m_run(0), m_ev(0), stopping(false), done(false), started(0) {
      tdc = new CAEN_V1290();
      initialized = false;
      tdc_unpacker = NULL;
      outTree=NULL;
      _mode = DWC_DEBUG;
      std::cout<<"Initialisation of the DWC Producer..."<<std::endl;
    }

  virtual void OnConfigure(const eudaq::Configuration & config) {
    std::cout << "Configuring: " << config.Name() << std::endl;

    //Read the data output file prefix
    dataFilePrefix = config.Get("dataFilePrefix", "../data/dwc_run_");


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


    CAEN_V1290::CAEN_V1290_Config_t _config;

    _config.baseAddress = config.Get("baseAddress", 0x00AA0000);
    _config.model = static_cast<CAEN_V1290::CAEN_V1290_Model_t>(config.Get("model", 1));
    _config.triggerTimeSubtraction = static_cast<bool>(config.Get("triggerTimeSubtraction", 1));
    _config.triggerMatchMode = static_cast<bool>(config.Get("triggerMatchMode", 1));
    _config.emptyEventEnable = static_cast<bool>(config.Get("emptyEventEnable", 1));
    _config.edgeDetectionMode = static_cast<CAEN_V1290::CAEN_V1290_EdgeDetection_t>(config.Get("edgeDetectionMode", 3));
    _config.timeResolution = static_cast<CAEN_V1290::CAEN_V1290_TimeResolution_t>(config.Get("timeResolution", 3));
    _config.maxHitsPerEvent = static_cast<CAEN_V1290::CAEN_V1290_MaxHits_t>(config.Get("maxHitsPerEvent", 8));
    _config.enabledChannels = config.Get("enabledChannels", 0x00FF);
    _config.windowWidth = config.Get("windowWidth", 0x40);
    _config.windowOffset = config.Get("windowOffset", -1);

    //read the enabled channels
    N_channels = config.Get("N_channels", 16);
    EUDAQ_INFO("Enabled channels:");
    for (unsigned int channel=0; channel<N_channels; channel++){
      channels_enabled[channel] = (config.Get(("channel_"+std::to_string(channel)).c_str(), -1)==1) ? true : false;
      std::cout<<"TDC channel "<<channel<<" connected ? "<<channels_enabled[channel]<<std::endl;
    }

    
    dwc1_left_channel = config.Get("dwc1_left_channel", 0); 
    dwc1_right_channel = config.Get("dwc1_right_channel", 1);
    dwc1_down_channel = config.Get("dwc1_down_channel", 2); 
    dwc1_up_channel = config.Get("dwc1_up_channel", 3);
    
    dwc2_left_channel = config.Get("dwc2_left_channel", 4); 
    dwc2_right_channel = config.Get("dwc2_right_channel", 5);
    dwc2_down_channel = config.Get("dwc2_down_channel", 6); 
    dwc2_up_channel = config.Get("dwc2_up_channel", 7);
    
    dwc3_left_channel = config.Get("dwc3_left_channel", 8); 
    dwc3_right_channel = config.Get("dwc3_right_channel", 9);      
    dwc3_down_channel = config.Get("dwc3_down_channel", 10); 
    dwc3_up_channel = config.Get("dwc3_up_channel", 11);
    
    dwc4_left_channel = config.Get("dwc4_left_channel", 12); 
    dwc4_right_channel = config.Get("dwc4_right_channel", 13);
    dwc4_down_channel = config.Get("dwc4_down_channel", 14); 
    dwc4_up_channel = config.Get("dwc4_up_channel", 15);
      

    if (_mode == DWC_RUN) {
      if (!initialized) {  //the initialization is to be run just once
        initialized = tdc->Init();
      }
      if (initialized) {
        tdc->Config(_config);
        tdc->SetupModule();
      }
    }

    defaultTimestamp = config.Get("defaultTimestamp", -999);
    SetStatus(eudaq::Status::LVL_OK, "Configured (" + config.Name() + ")");

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
        tdc->BufferClear();
      else 
        EUDAQ_INFO("ATTENTION !!! Communication to the TDC has not been established");
    }

    dwc_timestamps.clear();
    channels.clear();
    for (size_t channel=0; channel<N_channels; channel++) {
      channels.push_back(-1);
      dwc_timestamps.push_back(defaultTimestamp);
    }

    if (tdc_unpacker != NULL) delete tdc_unpacker;
    tdc_unpacker = new Unpacker(N_channels);

    if (outTree != NULL) delete outTree;
    outTree = new TTree("DelayWireChambers", "DelayWireChambers");
    outTree->Branch("run", &m_run);
    outTree->Branch("event", &m_ev);
    outTree->Branch("channels", &channels);
    outTree->Branch("dwc_timestamps", &dwc_timestamps);
    outTree->Branch("timeSinceStart", &timeSinceStart);


    SetStatus(eudaq::Status::LVL_OK, "Running");
    startTime = std::chrono::steady_clock::now();
    started=true;
  }

  // This gets called whenever a run is stopped
  virtual void OnStopRun() {
    SetStatus(eudaq::Status::LVL_OK, "Stopping");
    EUDAQ_INFO("Stopping Run");
    started=false;
    // Set a flag tao signal to the polling loop that the run is over
    stopping = true;

    // Send an EORE after all the real events have been sent
    // You can also set tags on it (as with the BORE) if necessary
    SendEvent(eudaq::RawDataEvent::EORE("Test", m_run, ++m_ev));

    std::ostringstream os;
    os.str(""); os<<"Saving the data into the outputfile: "<<dataFilePrefix<<m_run<<".root";
    EUDAQ_INFO(os.str().c_str());
    //save the tree into a file
    TFile* outfile = new TFile((dataFilePrefix+std::to_string(m_run)+".root").c_str(), "RECREATE");
    outTree->Write();
    outfile->Close();

    stopping = false;
    SetStatus(eudaq::Status::LVL_OK, "Stopped");
  }

  // This gets called when the Run Control is terminating,
  // we should also exit.
  virtual void OnTerminate() {
    EUDAQ_INFO("Terminating...");
    done = true;
    eudaq::mSleep(200);

    delete tdc;
    delete tdc_unpacker;
  }




  void ReadoutLoop() {

    while(!done) {
      if (!started) {
        eudaq::mSleep(200);
        continue;
      }

      if (stopping) continue;
      if (_mode==DWC_RUN && initialized) {
        tdc->Read(dataStream);
      } else if (_mode==DWC_DEBUG) {
        eudaq::mSleep(541);
        tdc->generatePseudoData(dataStream);
      }

      if (dataStream.size() == 0)
        continue;

      m_ev++;
     

      tdcData unpacked = tdc_unpacker->ConvertTDCData(dataStream);

      for (int channel=0; channel<N_channels; channel++) {
        channels[channel] = channel;
        dwc_timestamps[channel] = channels_enabled[channel] ? unpacked.timeOfArrivals[channel] : defaultTimestamp;
      }

      //get the timestamp since start:
      timeSinceStart = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count();
      outTree->Fill();

      std::cout<<"+++ Event: "<<m_ev<<": "<<timeSinceStart<<" ms +++"<<std::endl;
      for (int channel=0; channel<N_channels; channel++) std::cout<<" "<<dwc_timestamps[channel]; std::cout<<std::endl;

      // ------
      // Here, can we do a conversion of the raw data to X,Y positions of the Wire Chambers ->yes
      // Let's assume we can, and the numbers are stored in a vector of floats
      
 
      //ATTENTION !
      //possible source of segfault if the key, i.e. the assigned channel is not in between 0 and N_channels. N_channels and all the channel assignments originate from the configuration
      PosXY.clear();
      PosXY.push_back(dwc_timestamps[dwc1_left_channel]);
      PosXY.push_back(dwc_timestamps[dwc1_right_channel]);
      PosXY.push_back(dwc_timestamps[dwc1_down_channel]);
      PosXY.push_back(dwc_timestamps[dwc1_up_channel]);
      
      //x-y of wire chamber 2
      PosXY.push_back(dwc_timestamps[dwc2_left_channel]);
      PosXY.push_back(dwc_timestamps[dwc2_right_channel]);
      PosXY.push_back(dwc_timestamps[dwc2_down_channel]);
      PosXY.push_back(dwc_timestamps[dwc2_up_channel]);
      
      //x-y of wire chamber 3
      PosXY.push_back(dwc_timestamps[dwc3_left_channel]);
      PosXY.push_back(dwc_timestamps[dwc3_right_channel]);
      PosXY.push_back(dwc_timestamps[dwc3_down_channel]);
      PosXY.push_back(dwc_timestamps[dwc3_up_channel]);  

      //x-y of wire chamber 4
      PosXY.push_back(dwc_timestamps[dwc4_left_channel]);
      PosXY.push_back(dwc_timestamps[dwc4_right_channel]);
      PosXY.push_back(dwc_timestamps[dwc4_down_channel]);
      PosXY.push_back(dwc_timestamps[dwc4_up_channel]);


      //making an EUDAQ event
      eudaq::RawDataEvent ev(EVENT_TYPE,m_run,m_ev);

      ev.AddBlock(0, dataStream);
      ev.AddBlock(1, PosXY);

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

    std::string dataFilePrefix;

    //set on configuration
    CAEN_V1290* tdc;
    Unpacker* tdc_unpacker;

    std::vector<WORD> dataStream;

    int N_channels;
    std::map<int, bool> channels_enabled;

    //generated for each run
    TTree* outTree;

    std::vector<int> dwc_timestamps;
    std::vector<int> channels;
    std::chrono::steady_clock::time_point startTime;
    int timeSinceStart;

    int defaultTimestamp;

    std::vector<float> PosXY;
    //mapping and conversion parameters for the online monitoring
    size_t dwc1_left_channel, dwc1_right_channel, dwc1_down_channel, dwc1_up_channel, dwc2_left_channel, dwc2_right_channel, dwc2_down_channel, dwc2_up_channel, dwc3_left_channel, dwc3_right_channel, dwc3_down_channel, dwc3_up_channel, dwc4_left_channel, dwc4_right_channel, dwc4_down_channel, dwc4_up_channel;

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
