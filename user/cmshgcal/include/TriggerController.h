#ifndef H_TRIGGERCONTROLLER
#define H_TRIGGERCONTROLLER

#include <iostream>
#include <vector>

#include "IpbusHwController.h"

enum STATES{ 
  WAIT,
  RDOUT_RDY,
  RDOUT_FIN,
  END_RUN
};

enum ACQ_MODE{
  BEAMTEST,
  DEBUG
};

class TriggerController
{
 public:
  TriggerController(){;}
  TriggerController(std::vector< ipbus::IpbusHwController* > rdout);
  ~TriggerController(){;}
  void startrunning( uint32_t runNumber, const ACQ_MODE mode=BEAMTEST );
  bool checkState( STATES st ) const { return m_state==st; }
  void stopRun() { m_gotostop=true; std::cout << "receive stop command : gotostop = " << m_gotostop << std::endl; }
  void readoutCompleted();

  void setAcqMode( const ACQ_MODE mode ){m_acqmode=mode;}
  ACQ_MODE getAcqMode(  ) const {return m_acqmode;}
  
  uint32_t eventNumber() const {return m_evt;}
  uint32_t runNumber() const {return m_run;}

 private:
  void run();
  void runDebug();

  std::vector< ipbus::IpbusHwController* > m_rdout_orms;
  STATES m_state;
  ACQ_MODE m_acqmode;
  uint32_t m_run;
  uint32_t m_evt;
  bool m_gotostop;
  bool m_rdoutcompleted;
};

#endif
