#include "TriggerController.h"
#include "boost/thread/thread.hpp"

#define RDOUT_DONE_MAGIC 0xABCD4321

TriggerController::TriggerController(std::vector< ipbus::IpbusHwController* > rdout,int throwFirstTrigger) : m_state(WAIT),
													     m_acqmode(BEAMTEST),
													     m_run(0),
													     m_evt(0),
													     m_rdout_orms(rdout),
													     m_gotostop(false),
													     m_rdoutcompleted(false),
													     m_throwFirstTrigger(throwFirstTrigger)
{;}

void TriggerController::readoutCompleted() 
{
  m_mutex.lock();
  m_rdoutcompleted=true;
  m_mutex.unlock();
  //    std::cout << "receive rdoutcompleted command : rdoutcompleted = " << m_rdoutcompleted << std::endl; 
}

bool TriggerController::checkState( STATES st )
{
  m_mutex.lock();
  bool val= m_state==st ? true : false;
  m_mutex.unlock();
  return val;
}

void TriggerController::stopRun()
{
  m_mutex.lock();
  m_gotostop=true;
  std::cout << "receive stop command : gotostop = " << m_gotostop << std::endl;
  m_mutex.unlock();
}

void TriggerController::startrunning( uint32_t runNumber, const ACQ_MODE mode )
{
  m_mutex.lock();
  m_run=runNumber;
  m_evt=0;
  m_state=WAIT;
  m_acqmode=mode;
  m_gotostop=false;
  m_mutex.unlock();
  
  switch( m_acqmode ){
  case BEAMTEST : run(); break;
  case DEBUG : runDebug(); break;
  }
}

void TriggerController::ReadAndThrowFirstTrigger()
{
  boost::this_thread::sleep( boost::posix_time::microseconds(2000) );
  std::cout << "here i am in throwtrigger" << std::endl;
  while(1){
    bool rdout_ready=true;
    for( std::vector<ipbus::IpbusHwController*>::iterator it=m_rdout_orms.begin(); it!=m_rdout_orms.end(); ++it )
      if( (*it)->ReadRegister("BLOCK_READY")!=1 ) 
	rdout_ready=false;
    if( !rdout_ready ){
      boost::this_thread::sleep( boost::posix_time::microseconds(1) );
      continue;
    }
    else break;
  }
  std::cout << "receive the readout rdy" << std::endl;
  for( std::vector<ipbus::IpbusHwController*>::iterator it=m_rdout_orms.begin(); it!=m_rdout_orms.end(); ++it ){
    (*it)->ReadDataBlock("FIFO");
    (*it)->ResetTheData();
    while(1){
      if( (*it)->ReadRegister("DATE_STAMP") )
	break;
      else
	boost::this_thread::sleep( boost::posix_time::microseconds(1) );     
    }
  }
  std::cout << "read fifo and throw the event" << std::endl;
  for( std::vector<ipbus::IpbusHwController*>::iterator it=m_rdout_orms.begin(); it!=m_rdout_orms.end(); ++it )
    (*it)->SetRegister("RDOUT_DONE",RDOUT_DONE_MAGIC);
  boost::this_thread::sleep( boost::posix_time::microseconds(2000) );
}

void TriggerController::run()
{
  m_mutex.lock();
  for( std::vector<ipbus::IpbusHwController*>::iterator it=m_rdout_orms.begin(); it!=m_rdout_orms.end(); ++it )
    (*it)->SetRegister("RDOUT_DONE",RDOUT_DONE_MAGIC);
  if(m_throwFirstTrigger)
    this->ReadAndThrowFirstTrigger();
  m_mutex.unlock();
  while(1){
    if( m_state==END_RUN ) break;
    m_mutex.lock();
    while(1){
      bool rdout_ready=true;
      for( std::vector<ipbus::IpbusHwController*>::iterator it=m_rdout_orms.begin(); it!=m_rdout_orms.end(); ++it )
	if( (*it)->ReadRegister("BLOCK_READY")!=1 ) 
	  rdout_ready=false;
      std::cout << "TriggerController waits for block ready ; state = " << m_state << " ... " << std::endl;
      if( !rdout_ready ){
	boost::this_thread::sleep( boost::posix_time::microseconds(1000) );
	if( m_gotostop ) m_state=END_RUN;
	continue;
      }
      else break;
    }
    std::cout << "... TriggerController receives block ready ; state = " << m_state  << std::endl;
    m_rdoutcompleted=false;
    m_evt=m_rdout_orms[0]->ReadRegister("TRIG_COUNT");
    m_state=RDOUT_RDY;
    std::cout << "TriggerController waits for readout completed  ; state = " << m_state  << " ... " << std::endl;
    m_mutex.unlock();
    while(1){
      if( m_rdoutcompleted==true ){
	m_mutex.lock();
	std::cout << "TriggerController receives readout completed ; state = " << m_state << " ... " << std::endl;
	m_state=RDOUT_FIN;
	for( std::vector<ipbus::IpbusHwController*>::iterator it=m_rdout_orms.begin(); it!=m_rdout_orms.end(); ++it )
	  (*it)->SetRegister("RDOUT_DONE",RDOUT_DONE_MAGIC); 
	m_mutex.unlock();
	break;
      }
      boost::this_thread::sleep( boost::posix_time::microseconds(1000) );
    }
    m_mutex.lock();
    if( m_gotostop ) m_state=END_RUN;
    else m_state=WAIT;
    m_mutex.unlock();
  }
  return;
}

void TriggerController::runDebug()
{
  for( std::vector<ipbus::IpbusHwController*>::iterator it=m_rdout_orms.begin(); it!=m_rdout_orms.end(); ++it ){

    std::cout << "   Before setting RDOUT_DONE register"<<(*it)->getInterface()->id() << "\t BLOCK_READY = " << (*it)->ReadRegister("BLOCK_READY")
	      <<"  RDOUT_DONE: "<< (*it)->ReadRegister("RDOUT_DONE")<<std::endl;
    
    (*it)->SetRegister("RDOUT_DONE",RDOUT_DONE_MAGIC);
  }
  while(1){
    if( m_state==END_RUN ) break;
    bool rdout_ready=true;
    int iter=0;
    for( std::vector<ipbus::IpbusHwController*>::iterator it=m_rdout_orms.begin(); it!=m_rdout_orms.end(); ++it ){
      std::cout << (*it)->getInterface()->id() << "\t BLOCK_READY = " << (*it)->ReadRegister("BLOCK_READY")
		<<"  RDOUT_DONE: "<< (*it)->ReadRegister("RDOUT_DONE")<< "   iter="<<iter<<std::endl;
      if( (*it)->ReadRegister("BLOCK_READY")!=1 ) 
	rdout_ready=false;
    }
    if( !rdout_ready ){
      std::cout << "trigger for BLOCK_READY not received yet -> wait for 1 seconds" << std::endl;
      boost::this_thread::sleep( boost::posix_time::milliseconds(1000) );
      if( m_gotostop ) m_state=END_RUN;
      continue;
    }
    else
      std::cout << "BLOCK_READY ok for each boards -> try to read fifos" << std::endl;
    m_rdoutcompleted=false;
    m_evt=m_rdout_orms[0]->ReadRegister("TRIG_COUNT");
    m_state=RDOUT_RDY;
    while(1){
      if( m_rdoutcompleted==true ){
	m_state=RDOUT_FIN;
	break;
      }
      boost::this_thread::sleep( boost::posix_time::microseconds(1) );
    }
    std::cout << "finish to read fifos" << std::endl;
    boost::this_thread::sleep( boost::posix_time::milliseconds(1000) );
    for( std::vector<ipbus::IpbusHwController*>::iterator it=m_rdout_orms.begin(); it!=m_rdout_orms.end(); ++it )
      (*it)->SetRegister("RDOUT_DONE",RDOUT_DONE_MAGIC);
    std::cout << "finish to send RDOUT_DONE to each board -> wait for 1 seconds" << std::endl;
    boost::this_thread::sleep( boost::posix_time::milliseconds(1000) );
    if( m_gotostop ) m_state=END_RUN;
    else m_state=WAIT;
  }
  return;
}
