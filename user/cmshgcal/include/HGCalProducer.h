#ifndef HGCALPRODUCER_HH
#define HGCALPRODUCER_HH

#include "eudaq/Producer.hh"

#include <iostream>
#include <ostream>
#include <vector>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>

#include <boost/thread/thread.hpp>

#include "IpbusHwController.h"
#include "TriggerController.h"

namespace eudaq {
  
  class HGCalProducer: public eudaq::Producer
  {
  public:
    HGCalProducer(const std::string & name, const std::string & runcontrol);
    void DoInitialise() override final;
    void DoConfigure() override final;
    void DoStartRun() override final;
    void DoStopRun() override final;
    void DoReset() override final{;}
    void DoTerminate() override final;
    void MainLoop() ;

    //static const uint32_t m_id_factory = eudaq::cstr2hash("HGCalProducer");
  private:

    void startSoftOnSYNCBoard( std::string syncPi );
    void startSoftOnRDOUTBoard( std::string rdoutPi );
    void stopSoftOnSYNCBoard( std::string syncPi );
    void stopSoftOnRDOUTBoard( std::string rdoutPi );
    bool checkCRC( const std::string & crcNodeName, ipbus::IpbusHwController *ptr );
    
    unsigned int m_run, m_ev, m_uhalLogLevel, m_blockSize;
    std::vector< ipbus::IpbusHwController* > m_rdout_orms;
    TriggerController *m_triggerController;
    ACQ_MODE m_acqmode;
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
  };
}

#endif // HGCALPRODUCER_HH

