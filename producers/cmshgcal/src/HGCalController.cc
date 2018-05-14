#include <boost/crc.hpp>
#include <boost/timer/timer.hpp>
#include <boost/thread/thread.hpp>

#include "HGCalController.h"

#define FORMAT_VERSION 1 //WARNING : be careful when changing this value
#define MAX_NUMBER_OF_ORM 16
#define RDOUT_DONE_MAGIC 0xABCD4321

void HGCalController::readFIFOThread( ipbus::IpbusHwController* orm)
{
  orm->ReadDataBlock("FIFO");
}

void HGCalController::init()
{
  m_mutex.lock();
  m_state=INIT;
  m_mutex.unlock();
}

void HGCalController::configure(HGCalConfig config)
{
  m_mutex.lock();
  m_config=config;

  //check if it is needed to empty the ipbus::IpbusHwController vector
  if( m_state==CONFED ){
    m_state==INIT;
    for( std::vector<ipbus::IpbusHwController *>::iterator it=m_rdout_orms.begin(); it!=m_rdout_orms.end(); ++it )
      delete (*it);
    m_rdout_orms.clear();
  }

  std::ostringstream os( std::ostringstream::ate );
  unsigned int bit=1;
  for( int iorm=0; iorm<MAX_NUMBER_OF_ORM; iorm++ ){
    std::cout << "iorm = " << iorm << "..." << std::endl;
    bool activeSlot=(m_config.rdoutMask&bit);
    bit = bit << 1;
    if(!activeSlot) continue;
    std::cout << "... found in the rdout mask " << std::endl;
    os.str(""); os << "RDOUT_ORM" << iorm;
    ipbus::IpbusHwController *orm = new ipbus::IpbusHwController(m_config.connectionFile,os.str(),m_config.blockSize);
    m_rdout_orms.push_back( orm );
  }

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
    const uint32_t cst0=(*it)->ReadRegister("CONSTANT0");
    const uint32_t cst1=(*it)->ReadRegister("CONSTANT1");
    os.str("");
    os << "ORM " << ormId << "\t SKIROC_MASK = " <<std::setw(8)<< std::hex<<mask << "\n";
    os << "ORM " << ormId << "\t CONST0 = " << std::hex << cst0 << "\n";
    os << "ORM " << ormId << "\t CONST1 = " << std::hex << cst1 ;
    std::cout << os.str() << std::endl;
    boost::this_thread::sleep( boost::posix_time::milliseconds(1000) );
  }    

  m_state=CONFED;
  m_mutex.unlock();
}

void HGCalController::startrun(int runId)
{
  m_mutex.lock();
  m_runId=runId;
  m_evtId=0;
  m_deqData.clear();

  char rfname[256];
  sprintf(rfname, "data_root/time.root"); // The path is relative to eudaq/bin; data_root could be a symbolic link
  m_rootfile = new TFile(rfname,"RECREATE");
  m_roottree = new TTree("daq_timing","HGCal daq timing root tree");
  m_roottree->Branch("daqrate",&m_daqrate);
  m_roottree->Branch("eventtime",&m_eventtime);
  m_roottree->Branch("rdoutreadytime",&m_rdoutreadytime);
  m_roottree->Branch("readouttime",&m_readouttime);
  m_roottree->Branch("datestamptime",&m_datestamptime);
  m_roottree->Branch("rdoutdonetime",&m_rdoutdonetime);

  if( m_config.saveRawData ){
    char name[256];
    sprintf(name, "raw_data/rawdata.raw"); // The path is relative to eudaq/bin; raw_data could be a symbolic link
    m_rawFile.open(name, std::ios::binary);
    uint32_t header[3];
    header[0]=time(0);
    header[1]=m_rdout_orms.size();
    header[2]=FORMAT_VERSION;
    m_rawFile.write(reinterpret_cast<const char*>(&header[0]), sizeof(header));
  }

  //wait for the date_stamp registers
  for( std::vector<ipbus::IpbusHwController*>::iterator it=m_rdout_orms.begin(); it!=m_rdout_orms.end(); ++it ){
    (*it)->ResetTheData();
    while(1){
      if( (*it)->ReadRegister("DATE_STAMP") )
	break;
      else
	boost::this_thread::sleep( boost::posix_time::microseconds(10) );
    }
  }
  
  for( std::vector<ipbus::IpbusHwController*>::iterator it=m_rdout_orms.begin(); it!=m_rdout_orms.end(); ++it )
    (*it)->SetRegister("RDOUT_DONE",RDOUT_DONE_MAGIC);
  if(m_config.throwFirstTrigger){
    this->readAndThrowFirstTrigger();
  }

  m_state=RUNNING;
  m_mutex.unlock();
  this->run();
}

void HGCalController::stoprun()
{
  m_mutex.lock();
  m_state=CONFED;
  m_mutex.unlock();
  boost::this_thread::sleep( boost::posix_time::microseconds(m_config.timeToWaitAtEndOfRun) ); //wait sometime until run thread is done
  m_mutex.lock();
  m_rootfile->Write();
  m_rootfile->Close();
  m_mutex.unlock();
}

void HGCalController::terminate()
{
  for( std::vector<ipbus::IpbusHwController *>::iterator it=m_rdout_orms.begin(); it!=m_rdout_orms.end(); ++it )
    delete (*it);
  m_rdout_orms.clear();
}

void HGCalController::run()
{
  while(1){

    m_mutex.lock();
    
    if( m_state!=RUNNING ) {m_mutex.unlock(); break;}

    boost::timer::cpu_timer eventTimer;
    boost::timer::cpu_timer rdoutreadyTimer;
    boost::timer::cpu_timer readoutTimer;
    boost::timer::cpu_timer datestampTimer;    
    boost::timer::cpu_timer rdoutDoneTimer;

    std::cout << "\t HGCalController waits for block ready" << std::endl;
    eventTimer.start();
    rdoutreadyTimer.start();
    while(1){
      bool rdout_ready=true;
      for( std::vector<ipbus::IpbusHwController*>::iterator it=m_rdout_orms.begin(); it!=m_rdout_orms.end(); ++it )
	if( (*it)->ReadRegister("BLOCK_READY")!=1 ) 
	  rdout_ready=false;
      if( !rdout_ready ){
	boost::this_thread::sleep( boost::posix_time::microseconds(10) );
	continue;
      }
      else break;
    }
    rdoutreadyTimer.stop();
    std::cout << "\t HGCalController received block ready" << std::endl;

    uint32_t trid=m_rdout_orms[0]->ReadRegister("TRIG_COUNT");
    if( trid<m_triggerId ){
      std::cout << "\t THIS IS A MAJOR ISSUE FROM NON INCREMENTED TRIGGER ID -> PAUSE IN THE DAQ" << std::endl;
      getchar();
    }
    else
      m_triggerId=trid;
    m_evtId++;
    
    readoutTimer.start();
    std::cout << "\t HGCalController starts reading the data" << std::endl;	
    boost::thread threadVec[m_rdout_orms.size()];
    for( int i=0; i<(int)m_rdout_orms.size(); i++)
      threadVec[i]=boost::thread(readFIFOThread,m_rdout_orms[i]);
    for( int i=0; i<(int)m_rdout_orms.size(); i++){
      threadVec[i].join();
    }
    HGCalDataBlock datablk;
    for( int i=0; i<(int)m_rdout_orms.size(); i++){
      datablk.addORMBlock(m_rdout_orms[i]->getData());
      uint32_t trailer=i;//8 bits for orm id
      trailer|=m_triggerId<<8;//24 bits for trigger number
      datablk.addTrailer(trailer);
      datablk.addTrailer( m_rdout_orms[i]->ReadRegister("CLK_COUNT0") );
      datablk.addTrailer( m_rdout_orms[i]->ReadRegister("CLK_COUNT1") );
    }
    std::cout << "\t HGCalController finished reading the data" << std::endl;	
    readoutTimer.stop();

    datestampTimer.start();
    std::cout << "\t HGCalController waits the date_stamp" << std::endl;	
    for( std::vector<ipbus::IpbusHwController*>::iterator it=m_rdout_orms.begin(); it!=m_rdout_orms.end(); ++it ){
      (*it)->ResetTheData();
      while(1){
	if( (*it)->ReadRegister("DATE_STAMP") )
	  break;
	else
	  boost::this_thread::sleep( boost::posix_time::microseconds(10) );
      }
    }
    std::cout << "\t HGCalController received the date_stamp" << std::endl;	
    datestampTimer.stop();
    
    rdoutDoneTimer.start();
    std::cout << "\t HGCalController sends readout done magic" << std::endl;	
    for( std::vector<ipbus::IpbusHwController*>::iterator it=m_rdout_orms.begin(); it!=m_rdout_orms.end(); ++it )
      (*it)->SetRegister("RDOUT_DONE",RDOUT_DONE_MAGIC); 
    std::cout << "\t HGCalController finished readout done magic" << std::endl;	
    rdoutDoneTimer.stop();
    
    eventTimer.stop();

    m_eventtime = eventTimer.elapsed().wall/1e9;
    m_rdoutreadytime = rdoutreadyTimer.elapsed().wall/1e9;
    m_readouttime = readoutTimer.elapsed().wall/1e9;
    m_datestamptime = datestampTimer.elapsed().wall/1e9;
    m_rdoutdonetime = rdoutDoneTimer.elapsed().wall/1e9;

    m_daqrate = 1.0/m_eventtime;

    m_roottree->Fill();
    m_mutex.unlock();

    //rdout done magic already sent, HW and FW shoud be able to deal with new event
    m_deqData.push_back(datablk);

    if( m_config.saveRawData )
      m_rawFile.write(reinterpret_cast<const char*>(&datablk.getData()[0]), datablk.getData().size()*sizeof(uint32_t));
    
    
  }
}

void HGCalController::readAndThrowFirstTrigger()
{
  boost::this_thread::sleep( boost::posix_time::microseconds(2000) );
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
  for( std::vector<ipbus::IpbusHwController*>::iterator it=m_rdout_orms.begin(); it!=m_rdout_orms.end(); ++it )
    (*it)->SetRegister("RDOUT_DONE",RDOUT_DONE_MAGIC);
  boost::this_thread::sleep( boost::posix_time::microseconds(2000) );
}

