#ifndef H_HGCALCONTROLLER
#define H_HGCALCONTROLLER

#include <iostream>
#include <fstream>
#include <vector>
#include <deque>
#include <sstream>
#include <vector>
#include <iomanip>

#include <TFile.h>
#include <TTree.h>

#include "IpbusHwController.h"

enum HGCalControllerState{ 
  INIT,
  CONFED,
  RUNNING,
};

struct HGCalConfig
{
  std::string connectionFile;
  uint16_t rdoutMask;
  uint32_t blockSize;
  int logLevel;
  int uhalLogLevel;
  int timeToWaitAtEndOfRun;//in seconds
  bool saveRawData;
  bool checkCRC;
  bool throwFirstTrigger;
};

class HGCalDataBlock
{
 public:
  HGCalDataBlock(){;}
  ~HGCalDataBlock(){;}
  inline void addORMBlock(std::vector<uint32_t> data){ m_data.insert(m_data.end(),data.begin(),data.end()); }
  inline void addHeader(uint32_t header){ m_data.insert(m_data.begin(),header); }
  inline void addTrailer(uint32_t trailer){ m_data.push_back(trailer); }
  inline std::vector<uint32_t> &getData(){return m_data;}
 private:
  std::vector<uint32_t> m_data;
};

class HGCalController
{
 public:
  HGCalController(){;}
  ~HGCalController(){;}
  void init();
  void configure(HGCalConfig config);
  void startrun(int runId);
  void stoprun();
  void terminate();
  inline std::deque<HGCalDataBlock>& getDequeData(){return m_deqData;}
  
 private:
  void run();
  static void readFIFOThread( ipbus::IpbusHwController* orm);
  void readAndThrowFirstTrigger();
  //
  int m_runId;
  int m_evtId;
  int m_triggerId;
  boost::mutex m_mutex;
  HGCalConfig m_config;
  HGCalControllerState m_state;
  bool m_gotostop;
  std::ofstream m_rawFile;
  std::vector< ipbus::IpbusHwController* > m_rdout_orms;
  std::deque<HGCalDataBlock> m_deqData;
  
  // root stuff to store timing information
  TFile *m_rootfile;
  TTree *m_roottree;
  float m_daqrate;
  float m_eventtime;
  float m_rdoutreadytime;
  float m_readouttime;
  float m_datestamptime;
  float m_rdoutdonetime;

};

#endif
