#include "CAEN_v1742.h"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <bitset>


//#define DEBUG_CAENV1742

using namespace std ;

int CAEN_V1742::Init ()
{
  if (_isInitialized) return _isInitialized;

  char* software_version = new char[100];
  CAENVME_SWRelease(software_version);
  std::cout<<"CAENVME library version: "<<software_version<<std::endl;
  
  if (!_isConfigured) {
    std::cout<<"[CAEN_V1742] Configuration has not been loaded. Initialisation is interrupted."<<std::endl;
    return _isInitialized;
  }
  
  ostringstream s;

  s.str(""); s << "[CAEN_V1742]::[INFO]::++++++ CAEN V1742 INIT ++++++";
  std::cout<<s.str()<<std::endl;
  
  CAEN_DGTZ_OpenDigitizer(GetConfiguration()->LinkType, GetConfiguration()->LinkNum, GetConfiguration()->ConetNode, GetConfiguration()->BaseAddress, &digitizerHandle_);

  int ret = CAEN_DGTZ_Success ;
  ERROR_CODES ErrCode = ERR_NONE ;


  ret = CAEN_DGTZ_GetInfo (digitizerHandle_, &boardInfo_) ;

  if (ret) 
    {
      s.str(""); s << "[CAEN_V1742]::[ERROR]::cannot get information of the board "<< endl ;
      std::cout<<s.str()<<std::endl;
      ErrCode = ERR_BOARD_INFO_READ ;
      return ErrCode ;
    }

  s.str(""); s << "[CAEN_V1742]::[INFO]::Connected to CAEN Digitizer Model "<< boardInfo_.ModelName       << endl ;
  s << "[CAEN_V1742]::[INFO]::ROC FPGA Release is "              << boardInfo_.ROC_FirmwareRel << endl ;
  s << "[CAEN_V1742]::[INFO]::AMC FPGA Release is "              << boardInfo_.AMC_FirmwareRel;
  
  std::cout<<s.str()<<std::endl;
  

  // Check firmware rivision (DPP firmwares cannot be used with WaveDump */
  int MajorNumber ;
  sscanf (boardInfo_.AMC_FirmwareRel, "%d", &MajorNumber) ;
  if (MajorNumber >= 128) 
    {
      s.str(""); s << "[CAEN_V1742]::[ERROR]::This digitizer has a DPP firmware";
      #ifdef DEBUG_CAENV1742
      std::cout<<s.str()<<std::endl;
      #endif
      ErrCode = ERR_INVALID_BOARD_TYPE ;
      return ErrCode ;
    }
  
  //Wait for "initialization of the board"
  eudaq::mSleep(100);
  // get num of channels, num of bit, num of group of the board */
  ret = (CAEN_DGTZ_ErrorCode) getMoreBoardInfo () ;
  if (ret) 
    {
      ErrCode = ERR_INVALID_BOARD_TYPE ;
      return ErrCode ;
    }

  _isInitialized=1;

  return _isInitialized;
}

int CAEN_V1742::SetupModule () {
  int ret = CAEN_DGTZ_Success ;
  ERROR_CODES ErrCode = ERR_NONE ;
  ostringstream s;

  /* *************************************************************************************** */
  /* program the digitizer                                                                   */
  /* *************************************************************************************** */
  // mask the channels not available for this model
  if ( (boardInfo_.FamilyCode != CAEN_DGTZ_XX740_FAMILY_CODE) && 
       (boardInfo_.FamilyCode != CAEN_DGTZ_XX742_FAMILY_CODE))
    {
      digitizerConfiguration_.EnableMask &= (1<<digitizerConfiguration_.Nch)-1 ;
    } else {
      digitizerConfiguration_.EnableMask &= (1<< (digitizerConfiguration_.Nch/9))-1 ;
    }
  if ( (boardInfo_.FamilyCode == CAEN_DGTZ_XX751_FAMILY_CODE) && 
       digitizerConfiguration_.DesMode) 
    {
      digitizerConfiguration_.EnableMask &= 0xAA ;
    }
  if ( (boardInfo_.FamilyCode == CAEN_DGTZ_XX731_FAMILY_CODE) && 
       digitizerConfiguration_.DesMode) 
    {
      digitizerConfiguration_.EnableMask &= 0x55 ;
    }
  ret = (CAEN_DGTZ_ErrorCode) programDigitizer () ;
  if (ret) 
    {
      ErrCode = ERR_DGZ_PROGRAM ;
      s.str(""); s << "[CAEN_V1742]::[ERROR]::++++++ Error in setup: ERR_DGZ_PROGRAM++++++";
      std::cout<<s.str()<<std::endl;
      return ErrCode ;
    }
  ret = CAEN_DGTZ_GetInfo (digitizerHandle_, &boardInfo_) ;
  if (ret) 
    {
      ErrCode = ERR_BOARD_INFO_READ ;
      s.str(""); s << "[CAEN_V1742]::[ERROR]::++++++ Error in setup: ERR_BOARD_INFO_READ++++++";
      std::cout<<s.str()<<std::endl;
      return ErrCode ;
    }

  ret = (CAEN_DGTZ_ErrorCode) getMoreBoardInfo () ;
  if (ret) 
    {
      ErrCode = ERR_INVALID_BOARD_TYPE ;
      s.str(""); s << "[CAEN_V1742]::[ERROR]::++++++ Error in setup: ERR_INVALID_BOARD_TYPE++++++";
      std::cout<<s.str()<<std::endl;
      return ErrCode ;
    }
  
  if ( digitizerConfiguration_.useCorrections != -1 ) { // only automatic corrections supported for the moment
    ret |= CAEN_DGTZ_LoadDRS4CorrectionData (digitizerHandle_,digitizerConfiguration_.DRS4Frequency) ;
    ret |= CAEN_DGTZ_EnableDRS4Correction (digitizerHandle_) ;
    if (ret) 
      {
        ErrCode = ERR_DGZ_PROGRAM ;
        s.str(""); s << "[CAEN_V1742]::[ERROR]::++++++ Error in setup: ERR_DGZ_PROGRAM++++++";
        std::cout<<s.str()<<std::endl;
        return ErrCode ;
      }
  }
  //now allocate the eventBuffers
  uint32_t AllocatedSize=0;
  
  ret = CAEN_DGTZ_AllocateEvent(digitizerHandle_, (void**)&event_);

  if (ret != CAEN_DGTZ_Success) {
    ErrCode = ERR_MALLOC;
    s.str(""); s << "[CAEN_V1742]::[ERROR]::++++++ Error in setup: ERR_MALLOC++++++";
    std::cout<<s.str()<<std::endl;
    return ErrCode;
  }

  ret = CAEN_DGTZ_MallocReadoutBuffer(digitizerHandle_, &buffer_, &AllocatedSize); /* WARNING: This malloc must be done after the digitizer programming */

  if (ret || AllocatedSize==0) {
    ErrCode = ERR_MALLOC;
    s.str(""); s << "[CAEN_V1742]::[ERROR]::++++++ Error in setup: ERR_MALLOC++++++";
    std::cout<<s.str()<<std::endl;
    return ErrCode;
  }

  s.str(""); s << "[CAEN_V1742]::[INFO]::++++++ CAEN V1742 END Setup ++++++";
  std::cout<<s.str()<<std::endl;
  
  
  return 0 ;
} ;



int CAEN_V1742::StartAcquisition (){
  int ret = 0 ;
  ostringstream s;
  /* reset the digitizer */
  ret |= CAEN_DGTZ_SWStartAcquisition (digitizerHandle_) ;

  if (ret != 0) {
    s.str(""); s << "[CAEN_V1742]::[ERROR]::Unable to start the data acquisition of digitizer.";
    std::cout<<s.str()<<std::endl;
    return ERR_RESTART ;
  }

  return 0 ;
};

int CAEN_V1742::StopAcquisition (){
  int ret = 0 ;
  ostringstream s;
  /* reset the digitizer */
  ret |= CAEN_DGTZ_SWStopAcquisition (digitizerHandle_) ;

  if (ret != 0) {
    s.str(""); s << "[CAEN_V1742]::[ERROR]::Unable to stop the data acquisition of digitizer.";
    std::cout<<s.str()<<std::endl;
    return ERR_RESTART ;
  }

  return 0 ;
};


// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----


int CAEN_V1742::Clear (){
  int ret = 0 ;
  ostringstream s;
  /* reset the digitizer */
  ret |= CAEN_DGTZ_Reset (digitizerHandle_) ;

  if (ret != 0) {
    s.str(""); s << "[CAEN_V1742]::[ERROR]::Unable to reset digitizer.Please reset digitizer manually then restart the program";
    #ifdef DEBUG_CAENV1742
    std::cout<<s.str()<<std::endl;
    #endif
    return ERR_RESTART ;
  }

  return 0 ;
} ;


// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----


int CAEN_V1742::BufferClear (){
  int ret = 0 ;
  ostringstream s;
  /* clear the digitizer buffers */

  ret |= CAEN_DGTZ_ClearData(digitizerHandle_); 

  if (ret != 0) {
    s.str(""); s << "[CAEN_V1742]::[ERROR]::Unable to clear buffers";
    #ifdef DEBUG_CAENV1742
    std::cout<<s.str()<<std::endl;
    #endif
    return ERR_CLEARBUFFER ;
  }
  eudaq::mSleep(100);
  return 0;
} ;


// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----


int CAEN_V1742::Config(CAEN_V1742::CAEN_V1742_Config_t &_config) {
  GetConfiguration()->BaseAddress = _config.BaseAddress ;
  GetConfiguration()->LinkType = _config.LinkType ;      //hard coded value
  GetConfiguration()->LinkNum = _config.LinkNum ;
  GetConfiguration()->ConetNode = _config.ConetNode ;
  GetConfiguration()->Nch = _config.Nch ;
  GetConfiguration()->Nbit = _config.Nbit ;
  GetConfiguration()->Ts = _config.Ts ;
  GetConfiguration()->RecordLength = _config.RecordLength ;
  CAEN_DGTZ_DRS4Frequency_t DRS4Frequency;
  GetConfiguration()->PostTrigger = _config.PostTrigger ;
  GetConfiguration()->NumEvents = _config.NumEvents ;
  GetConfiguration()->InterruptNumEvents  = _config.InterruptNumEvents; 
  GetConfiguration()->TestPattern = _config.TestPattern ;
  GetConfiguration()->DesMode = 0 ;   //not applicable for the V1742 
  GetConfiguration()->TriggerEdge = _config.TriggerEdge ;
  GetConfiguration()->FPIOtype = _config.FPIOtype ;
  GetConfiguration()->ExtTriggerMode = _config.ExtTriggerMode ;
  GetConfiguration()->EnableMask = _config.EnableMask ;
  CAEN_DGTZ_TriggerMode_t ChannelTriggerMode[CAEN_V1742_MAXSET];
  GetConfiguration()->FastTriggerMode = _config.FastTriggerMode ;
  GetConfiguration()->FastTriggerEnabled = _config.FastTriggerEnabled ;
  GetConfiguration()->useCorrections = _config.useCorrections ;

  //one value supported so far, 01 August 2018
  for (int i=0; i<CAEN_V1742_MAXSET; i++) {
    GetConfiguration()->DCoffset[i] = _config.DCoffset[i];
    GetConfiguration()->Threshold[i] = _config.Threshold[i];
    GetConfiguration()->GroupTrgEnableMask[i] = _config.GroupTrgEnableMask[i];
    GetConfiguration()->FTDCoffset[i] = _config.FTDCoffset[i];
    GetConfiguration()->FTThreshold[i] = _config.FTThreshold[i];
  
    for (int j=0; j<CAEN_V1742_MAXCH; j++) GetConfiguration()->DCoffsetGrpCh[i][j] = _config.DCoffsetGrpCh[i][j];
  }
  
  //no generic write commands
  GetConfiguration()->GWn = 0 ;
  for (int i=0; i<CAEN_V1742_MAXGW; i++) {
    GetConfiguration()->GWaddr[i] = _config.GWaddr[i];
    GetConfiguration()->GWdata[i] = _config.GWdata[i];
  }


  _isConfigured = true; 

  return 0;
}


// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

int CAEN_V1742::ClearBusy (){  
  //  return BufferClear();
  return 0;
} ;


// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----


int CAEN_V1742::Read (vector<WORD> &v) {

  CAEN_DGTZ_ErrorCode ret=CAEN_DGTZ_Success ;
  ERROR_CODES ErrCode= ERR_NONE ;
  
  
  

  uint32_t BufferSize, NumEvents;
  CAEN_DGTZ_EventInfo_t       EventInfo ;
  ostringstream s;

  NumEvents = 0 ;
  int itry=0;
  int TIMEOUT=10;

  while (1 > NumEvents && itry<TIMEOUT) {
    ++itry;      
    BufferSize=0;
    NumEvents=0;
    ret = CAEN_DGTZ_ReadData (digitizerHandle_, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, buffer_, &BufferSize) ;
    if (ret) {
      s.str(""); s << "[CAEN_V1742]::[ERROR]::READOUT ERROR (ReadData)!!!";
      std::cout<<s.str()<<std::endl;
      ErrCode = ERR_READOUT ;
      return ErrCode ;
    }

    if (BufferSize != 0) {
      ret = CAEN_DGTZ_GetNumEvents (digitizerHandle_, buffer_, BufferSize, &NumEvents) ;
      if (ret) {
        s.str(""); s << "[CAEN_V1742]::[ERROR]::READOUT ERROR (NumEvents)!!!";
        std::cout<<s.str()<<std::endl;
        ErrCode = ERR_READOUT ;
        return ErrCode ;
      }
      if (NumEvents == 0) {
        s.str(""); s << "[CAEN_V1742]::[WARNING]::NO EVENTS BUT BUFFERSIZE !=0";
        std::cout<<s.str()<<std::endl;
      }
    }
  }

  //nothing to read out in the buffer
  if (itry == TIMEOUT) {
    #ifdef DEBUG_CAENV1742
      s.str(""); s << "[CAEN_V1742]::[ERROR]::READ TIMEOUT!!!";
      std::cout<<s.str()<<std::endl;
    #endif
    return 0;
  }

  //For the moment empty the buffers one by one
  if (NumEvents != 1) {
    s.str(""); s << "[CAEN_V1742]::[ERROR]::MISMATCHED EVENTS!!!" << NumEvents;
    std::cout<<s.str()<<std::endl;
    ErrCode = ERR_MISMATCH_EVENTS ;
    return ErrCode ;
  }


  // Building the event
  
  /* Get one event from the readout buffer */
  ret = CAEN_DGTZ_GetEventInfo (digitizerHandle_, buffer_, BufferSize, 0, &EventInfo, &eventPtr_) ;
  if (ret) {
    s.str(""); s << "[CAEN_V1742]::[ERROR]::EVENT BUILD ERROR!!!";
    std::cout<<s.str()<<std::endl;
    ErrCode = ERR_EVENT_BUILD ;
    return ErrCode ;
  }
  ret = CAEN_DGTZ_DecodeEvent (digitizerHandle_, eventPtr_, (void**)&event_) ;
  if (ret) {
    s.str(""); s << "[CAEN_V1742]::[ERROR]::EVENT BUILD ERROR!!!";  
    std::cout<<s.str()<<std::endl;
    ErrCode = ERR_EVENT_BUILD ;
    return ErrCode ;
  }    
  // if (digitizerConfiguration_.useCorrections != -1) { // if manual corrections
  //     ApplyDataCorrection ( 0, digitizerConfiguration_.useCorrections, digitizerConfiguration_.DRS4Frequency, & (event_->DataGroup[0]), &Table_gr0) ;
  //     ApplyDataCorrection ( 1, digitizerConfiguration_.useCorrections, digitizerConfiguration_.DRS4Frequency, & (event_->DataGroup[1]), &Table_gr1) ;
  //       }
  
  ret = (CAEN_DGTZ_ErrorCode) writeEventToOutputBuffer (v,&EventInfo,event_) ;
  if (ret) {
    s.str(""); s << "[CAEN_V1742]::[ERROR]::EVENT BUILD ERROR!!!";
    std::cout<<s.str()<<std::endl;
    ErrCode = ERR_EVENT_BUILD ;
    return ErrCode ;
  }    
  // } //close cycle over events    
  return 0 ;

} ;


// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----


int CAEN_V1742::getMoreBoardInfo ()
{
  CAEN_DGTZ_DRS4Frequency_t freq ;
  int ret ;
  switch (boardInfo_.FamilyCode) {
  case CAEN_DGTZ_XX724_FAMILY_CODE: digitizerConfiguration_.Nbit = 14 ; digitizerConfiguration_.Ts = 10.0 ; break ;
  case CAEN_DGTZ_XX720_FAMILY_CODE: digitizerConfiguration_.Nbit = 12 ; digitizerConfiguration_.Ts = 4.0 ;  break ;
  case CAEN_DGTZ_XX721_FAMILY_CODE: digitizerConfiguration_.Nbit =  8 ; digitizerConfiguration_.Ts = 2.0 ;  break ;
  case CAEN_DGTZ_XX731_FAMILY_CODE: digitizerConfiguration_.Nbit =  8 ; digitizerConfiguration_.Ts = 2.0 ;  break ;
  case CAEN_DGTZ_XX751_FAMILY_CODE: digitizerConfiguration_.Nbit = 10 ; digitizerConfiguration_.Ts = 1.0 ;  break ;
  case CAEN_DGTZ_XX761_FAMILY_CODE: digitizerConfiguration_.Nbit = 10 ; digitizerConfiguration_.Ts = 0.25 ;  break ;
  case CAEN_DGTZ_XX740_FAMILY_CODE: digitizerConfiguration_.Nbit = 12 ; digitizerConfiguration_.Ts = 16.0 ; break ;
  case CAEN_DGTZ_XX742_FAMILY_CODE: 
    digitizerConfiguration_.Nbit = 12 ; 
    if ( (ret = CAEN_DGTZ_GetDRS4SamplingFrequency (digitizerHandle_, &freq)) != CAEN_DGTZ_Success) return (int)CAEN_DGTZ_CommError ;
    switch (freq) {
    case CAEN_DGTZ_DRS4_1GHz:
      digitizerConfiguration_.Ts = 1.0 ;
      break ;
    case CAEN_DGTZ_DRS4_2_5GHz:
      digitizerConfiguration_.Ts = (float)0.4 ;
      break ;
    case CAEN_DGTZ_DRS4_5GHz:
      digitizerConfiguration_.Ts = (float)0.2 ;
      break ;
    }
    break ;
  default: return -1 ;
  }
  if ( ( (boardInfo_.FamilyCode == CAEN_DGTZ_XX751_FAMILY_CODE) ||
       (boardInfo_.FamilyCode == CAEN_DGTZ_XX731_FAMILY_CODE) ) && digitizerConfiguration_.DesMode)
    digitizerConfiguration_.Ts /= 2 ;
    
  switch (boardInfo_.FamilyCode) {
  case CAEN_DGTZ_XX724_FAMILY_CODE:
  case CAEN_DGTZ_XX720_FAMILY_CODE:
  case CAEN_DGTZ_XX721_FAMILY_CODE:
  case CAEN_DGTZ_XX751_FAMILY_CODE:
  case CAEN_DGTZ_XX761_FAMILY_CODE:
  case CAEN_DGTZ_XX731_FAMILY_CODE:
    switch (boardInfo_.FormFactor) {
    case CAEN_DGTZ_VME64_FORM_FACTOR:
    case CAEN_DGTZ_VME64X_FORM_FACTOR:
      digitizerConfiguration_.Nch = 8 ;
      break ;
    case CAEN_DGTZ_DESKTOP_FORM_FACTOR:
    case CAEN_DGTZ_NIM_FORM_FACTOR:
      digitizerConfiguration_.Nch = 4 ;
      break ;
    }
    break ;
  case CAEN_DGTZ_XX740_FAMILY_CODE:
    switch ( boardInfo_.FormFactor) {
    case CAEN_DGTZ_VME64_FORM_FACTOR:
    case CAEN_DGTZ_VME64X_FORM_FACTOR:
      digitizerConfiguration_.Nch = 64 ;
      break ;
    case CAEN_DGTZ_DESKTOP_FORM_FACTOR:
    case CAEN_DGTZ_NIM_FORM_FACTOR:
      digitizerConfiguration_.Nch = 32 ;
      break ;
    }
    break ;
  case CAEN_DGTZ_XX742_FAMILY_CODE:
    switch ( boardInfo_.FormFactor) {
    case CAEN_DGTZ_VME64_FORM_FACTOR:
    case CAEN_DGTZ_VME64X_FORM_FACTOR:
      digitizerConfiguration_.Nch = 36 ;
      break ;
    case CAEN_DGTZ_DESKTOP_FORM_FACTOR:
    case CAEN_DGTZ_NIM_FORM_FACTOR:
      digitizerConfiguration_.Nch = 16 ;
      break ;
    }
    break ;
  default:
    return -1 ;
  }
  return 0 ;
} ;


// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----


int CAEN_V1742::programDigitizer ()
{
  int i,j, ret = 0 ;
  ostringstream s;
  /* reset the digitizer */
  ret |= CAEN_DGTZ_Reset (digitizerHandle_) ;
  if (ret != 0) {
    s.str(""); s << "[CAEN_V1742]::[ERROR]::Unable to reset digitizer.Please reset digitizer manually then restart the program";
    std::cout<<s.str()<<std::endl;
    return ERR_RESTART ;
  }

  /* execute generic write commands */
  for (i=0 ; i<digitizerConfiguration_.GWn ; i++)
    ret |= CAEN_DGTZ_WriteRegister (digitizerHandle_, digitizerConfiguration_.GWaddr[i], digitizerConfiguration_.GWdata[i]) ;

  // Set the waveform test bit for debugging
  if (digitizerConfiguration_.TestPattern) {
    ret |= CAEN_DGTZ_WriteRegister (digitizerHandle_, CAEN_DGTZ_BROAD_CH_CONFIGBIT_SET_ADD, 1<<3) ;
    s.str(""); s << "[CAEN_V1742]::[INFO]::Setup test pattern";
    std::cout<<s.str()<<std::endl;
  }
  // custom setting for X742 boards
  if (boardInfo_.FamilyCode == CAEN_DGTZ_XX742_FAMILY_CODE) {
    ret |= CAEN_DGTZ_SetFastTriggerDigitizing (digitizerHandle_, (CAEN_DGTZ_EnaDis_t)digitizerConfiguration_.FastTriggerEnabled) ;
    ret |= CAEN_DGTZ_SetFastTriggerMode (digitizerHandle_, (CAEN_DGTZ_TriggerMode_t)digitizerConfiguration_.FastTriggerMode) ;
    s.str(""); s << "[CAEN_V1742]::[INFO]::Configuring fast trigger: "<<digitizerConfiguration_.FastTriggerEnabled<<","<<digitizerConfiguration_.FastTriggerMode;
    std::cout<<s.str()<<std::endl;    
  }
  if ( (boardInfo_.FamilyCode == CAEN_DGTZ_XX751_FAMILY_CODE) || (boardInfo_.FamilyCode == CAEN_DGTZ_XX731_FAMILY_CODE)) {
    ret |= CAEN_DGTZ_SetDESMode (digitizerHandle_, ( CAEN_DGTZ_EnaDis_t) digitizerConfiguration_.DesMode) ;
    s.str(""); s << "[CAEN_V1742]::[INFO]::Setting DES Mode: "<<digitizerConfiguration_.DesMode;
    std::cout<<s.str()<<std::endl;        
  }
  ret |= CAEN_DGTZ_SetRecordLength (digitizerHandle_, digitizerConfiguration_.RecordLength) ;
  s.str(""); s << "[CAEN_V1742]::[INFO]::Setting record length: "<<digitizerConfiguration_.RecordLength;
  std::cout<<s.str()<<std::endl;    
  ret |= CAEN_DGTZ_SetPostTriggerSize (digitizerHandle_, (uint32_t) digitizerConfiguration_.PostTrigger) ;
  s.str(""); s << "[CAEN_V1742]::[INFO]::Setting post trigger size: "<<digitizerConfiguration_.PostTrigger;
  std::cout<<s.str()<<std::endl;      
  if (boardInfo_.FamilyCode != CAEN_DGTZ_XX742_FAMILY_CODE)
    ret |= CAEN_DGTZ_GetPostTriggerSize (digitizerHandle_, &digitizerConfiguration_.PostTrigger) ;
  ret |= CAEN_DGTZ_SetIOLevel (digitizerHandle_, (CAEN_DGTZ_IOLevel_t) digitizerConfiguration_.FPIOtype) ;
  s.str(""); s << "[CAEN_V1742]::[INFO]::Setting FPIO type: "<<digitizerConfiguration_.FPIOtype;
  std::cout<<s.str()<<std::endl;     
  if ( digitizerConfiguration_.InterruptNumEvents > 0) {
      // Interrupt handling
      if ( ret |= CAEN_DGTZ_SetInterruptConfig ( digitizerHandle_, CAEN_DGTZ_ENABLE,
             CAEN_V1742_VME_INTERRUPT_LEVEL, CAEN_V1742_VME_INTERRUPT_STATUS_ID,
             digitizerConfiguration_.InterruptNumEvents, CAEN_V1742_INTERRUPT_MODE)!= CAEN_DGTZ_Success) {
      s.str(""); s << "[CAEN_V1742]::[ERROR]::Error configuring interrupts. Interrupts disabled";
      std::cout<<s.str()<<std::endl;
      digitizerConfiguration_.InterruptNumEvents = 0 ;
      }
      s.str(""); s << "[CAEN_V1742]::[INFO]::Interrupts enabled";
      std::cout<<s.str()<<std::endl;
      
  }

  ret |= CAEN_DGTZ_SetMaxNumEventsBLT (digitizerHandle_, digitizerConfiguration_.NumEvents) ;
  s.str(""); s << "[CAEN_V1742]::[INFO]::Setting maximum of events blt: "<<digitizerConfiguration_.NumEvents;
  std::cout<<s.str()<<std::endl;  
  ret |= CAEN_DGTZ_SetAcquisitionMode (digitizerHandle_, CAEN_DGTZ_SW_CONTROLLED) ;   //?
  s.str(""); s << "[CAEN_V1742]::[INFO]::Setting acquisition mode: "<<CAEN_DGTZ_SW_CONTROLLED;
  std::cout<<s.str()<<std::endl;   
  ret |= CAEN_DGTZ_SetExtTriggerInputMode (digitizerHandle_, digitizerConfiguration_.ExtTriggerMode) ;
  s.str(""); s << "[CAEN_V1742]::[INFO]::Setting external trigger input mode: "<<digitizerConfiguration_.ExtTriggerMode;
  std::cout<<s.str()<<std::endl;   


  if ( (boardInfo_.FamilyCode == CAEN_DGTZ_XX740_FAMILY_CODE) || (boardInfo_.FamilyCode == CAEN_DGTZ_XX742_FAMILY_CODE)){
    ret |= CAEN_DGTZ_SetGroupEnableMask (digitizerHandle_, digitizerConfiguration_.EnableMask) ;
    for (i=0 ; i< (digitizerConfiguration_.Nch/9) ; i++) {
      if (digitizerConfiguration_.EnableMask & (1<<i)) {
  if (boardInfo_.FamilyCode == CAEN_DGTZ_XX742_FAMILY_CODE) {
    for (j=0 ; j<8 ; j++) {
      if (digitizerConfiguration_.DCoffsetGrpCh[i][j] != -1)
        ret |= CAEN_DGTZ_SetChannelDCOffset (digitizerHandle_, (i*8)+j, digitizerConfiguration_.DCoffsetGrpCh[i][j]) ;
      else
        ret |= CAEN_DGTZ_SetChannelDCOffset (digitizerHandle_, (i*8)+j, digitizerConfiguration_.DCoffset[i]) ;
    }
  }
  else {
    ret |= CAEN_DGTZ_SetGroupDCOffset (digitizerHandle_, i, digitizerConfiguration_.DCoffset[i]) ;
    ret |= CAEN_DGTZ_SetGroupSelfTrigger (digitizerHandle_, digitizerConfiguration_.ChannelTriggerMode[i], (1<<i)) ;
    ret |= CAEN_DGTZ_SetGroupTriggerThreshold (digitizerHandle_, i, digitizerConfiguration_.Threshold[i]) ;
    ret |= CAEN_DGTZ_SetChannelGroupMask (digitizerHandle_, i, digitizerConfiguration_.GroupTrgEnableMask[i]) ;
  } 
  ret |= CAEN_DGTZ_SetTriggerPolarity (digitizerHandle_, i, (CAEN_DGTZ_TriggerPolarity_t) digitizerConfiguration_.TriggerEdge) ;
  s.str(""); s << "[CAEN_V1742]::[INFO]::Setting trigger polarity: "<<digitizerConfiguration_.TriggerEdge;
  std::cout<<s.str()<<std::endl;                
      }
    }
  } else {
    ret |= CAEN_DGTZ_SetChannelEnableMask (digitizerHandle_, digitizerConfiguration_.EnableMask) ;
    s.str(""); s << "[CAEN_V1742]::[INFO]::Setting channel enable mask: "<<digitizerConfiguration_.EnableMask;
    std::cout<<s.str()<<std::endl;  
    for (i=0 ; i<digitizerConfiguration_.Nch ; i++) {
      if (digitizerConfiguration_.EnableMask & (1<<i)) {
        ret |= CAEN_DGTZ_SetChannelDCOffset (digitizerHandle_, i, digitizerConfiguration_.DCoffset[i]) ;
        ret |= CAEN_DGTZ_SetChannelSelfTrigger (digitizerHandle_, digitizerConfiguration_.ChannelTriggerMode[i], (1<<i)) ;
        ret |= CAEN_DGTZ_SetChannelTriggerThreshold (digitizerHandle_, i, digitizerConfiguration_.Threshold[i]) ;
        ret |= CAEN_DGTZ_SetTriggerPolarity (digitizerHandle_, i, (CAEN_DGTZ_TriggerPolarity_t) digitizerConfiguration_.TriggerEdge) ;
      }
    }
  }
  if (boardInfo_.FamilyCode == CAEN_DGTZ_XX742_FAMILY_CODE) {
    for (i=0 ; i< (digitizerConfiguration_.Nch/9) ; i++) {
      s.str(""); s << "[CAEN_V1742]::[INFO]::Setting DRS4_FREQUENCY_CONFIG for Chip#" << i << " to " << digitizerConfiguration_.DRS4Frequency;
      ret |= CAEN_DGTZ_SetDRS4SamplingFrequency(digitizerHandle_, digitizerConfiguration_.DRS4Frequency);
      s << " " << ret;
      ret |= CAEN_DGTZ_SetGroupFastTriggerDCOffset (digitizerHandle_,i,digitizerConfiguration_.FTDCoffset[i]) ;
      s << " " << ret;
      ret |= CAEN_DGTZ_SetGroupFastTriggerThreshold (digitizerHandle_,i,digitizerConfiguration_.FTThreshold[i]) ;
      s << " " << ret;
      std::cout<<s.str()<<std::endl;
    }
  }
    
  
  if (ret)
    {
      s.str(""); s << "[CAEN_V1742]::[ERROR]::Error in Configuration";
      std::cout<<s.str()<<std::endl;
      return ERR_DGZ_PROGRAM ;
    }

  return 0 ;
} ;


// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----


int CAEN_V1742::writeEventToOutputBuffer (vector<WORD>& CAEN_V1742_eventBuffer, CAEN_DGTZ_EventInfo_t* eventInfo, CAEN_DGTZ_X742_EVENT_t* event)
{
  int gr,ch ;
  
  //          ====================================================
  //          |           V1742 Raw Event Data Format            |
  //          ====================================================

  //                       31  -  28 27  -  16 15   -   0
  //            Word[0] = [ 1010  ] [Event Tot #Words  ] //Event Header (5 words)
  //            Word[1] = [     Board Id    ] [ Pattern]  
  //            Word[2] = [      #channels readout     ]
  //            Word[3] = [        Event counter       ]
  //            Word[4] = [      Trigger Time Tag      ]
  //            Word[5] = [ 1000  ] [    Ch0   #Words  ] // Ch0 Data (2 + #samples words)
  //            Word[6] = [    Ch0  #Gr    ] [ Ch0 #Ch ] 
  //            Word[7] = [ Ch0 Corr. samples  (float) ]
  //                ..  = [ Ch0 Corr. samples  (float) ]
  // Word[5+Ch0 #Words] = [ 1000  ] [    Ch1   #Words  ] // Ch1 Data (2 + #samples words)
  // Word[6+Ch0 #Words] = [    Ch1  #Gr    ] [ Ch1 #Ch ]
  // Word[7+Ch0 #Words] = [ Ch1 Corr. samples  (float) ]
  //               ...   = [          .....             ]

  //CAEN_V1742_eventBuffer.clear () ;
  CAEN_V1742_eventBuffer.reserve(5 + digitizerConfiguration_.Nch*(digitizerConfiguration_.RecordLength+2)) ; //allocate once for all channels in a board
  CAEN_V1742_eventBuffer.resize (5) ;
  (CAEN_V1742_eventBuffer)[0]=0xA0000005 ; 
  (CAEN_V1742_eventBuffer)[1]=( (eventInfo->BoardId)<<26) +eventInfo->Pattern ;
  (CAEN_V1742_eventBuffer)[2]=0 ;
  (CAEN_V1742_eventBuffer)[3]=eventInfo->EventCounter ;
  (CAEN_V1742_eventBuffer)[4]=eventInfo->TriggerTimeTag ;

  //  printf ("EVENT 1742 %d %d",eventInfo->EventCounter,eventInfo->TriggerTimeTag) ;
  for (gr=0 ;gr< (digitizerConfiguration_.Nch/9) ;gr++) {
    if (event->GrPresent[gr]) {
      for (ch=0 ; ch<9 ; ch++) {
    int Size = event->DataGroup[gr].ChSize[ch] ;
    if (Size <= 0) {
      continue ;
    }

    // Channel Header for this event
     uint32_t ChHeader[2] ;
     ChHeader[0] = (8<<28) + ( (digitizerConfiguration_.DRS4Frequency & 0x3) << 26 ) + ( (2 + Size) & 0x3FFFFFF) ; //Number of words written for this channel
     ChHeader[1] = (gr<<16)+ch ;

    //Starting pointer
    int start_ptr=CAEN_V1742_eventBuffer.size () ;

    //Allocating necessary space for this channel
    CAEN_V1742_eventBuffer.resize (start_ptr + 2 + Size) ;
    std::memcpy (& ( (CAEN_V1742_eventBuffer)[start_ptr]), &ChHeader[0], 2 * sizeof (unsigned int)) ;

    //Beware the datas are float (because they are corrected...) but copying them here bit by bit. Should remember this for reading them out
    std::memcpy (& ( (CAEN_V1742_eventBuffer)[start_ptr+2]), event->DataGroup[gr].DataChannel[ch], Size * sizeof (unsigned int)) ;

    //Update event size and #channels
    (CAEN_V1742_eventBuffer)[0]+= (Size+2) ;
    (CAEN_V1742_eventBuffer)[2]++ ;
      }
    }
  }
  
  return 0 ;

}


// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----


int CAEN_V1742::setDefaults ()
{
//  char str[1000], str1[1000] ;
//  int i,j, ch=-1, val, Off=0, tr = -1 ;

  /* Default settings */
  digitizerConfiguration_.RecordLength = (1024*16) ;
  digitizerConfiguration_.PostTrigger = 80 ;
  digitizerConfiguration_.NumEvents = 1023 ;
  digitizerConfiguration_.EnableMask = 0xFF ;
  digitizerConfiguration_.GWn = 0 ;
  digitizerConfiguration_.ExtTriggerMode = CAEN_DGTZ_TRGMODE_ACQ_ONLY ;
  digitizerConfiguration_.InterruptNumEvents = 0 ;
  digitizerConfiguration_.TestPattern = 0 ;
  digitizerConfiguration_.TriggerEdge = 0 ;
  digitizerConfiguration_.DesMode = 0 ;
  digitizerConfiguration_.FastTriggerMode = CAEN_DGTZ_TRGMODE_DISABLED ;
  digitizerConfiguration_.FastTriggerEnabled = 0 ; 
  digitizerConfiguration_.FPIOtype = 0 ;
  /* strcpy (digitizerConfiguration_.GnuPlotPath, GNUPLOT_DEFAULT_PATH) ; */
  for (int i = 0 ; i < CAEN_V1742_MAXSET ; ++i) 
    {
      digitizerConfiguration_.DCoffset[i] = 0 ;
      digitizerConfiguration_.Threshold[i] = 0 ;
      digitizerConfiguration_.ChannelTriggerMode[i] = CAEN_DGTZ_TRGMODE_DISABLED ;
      digitizerConfiguration_.GroupTrgEnableMask[i] = 0 ;
      for (int j = 0 ; j < CAEN_V1742_MAXCH ; ++j) digitizerConfiguration_.DCoffsetGrpCh[i][j] = -1 ;
      digitizerConfiguration_.FTThreshold[i] = 0 ;
      digitizerConfiguration_.FTDCoffset[i] = 0 ;
    }

  digitizerConfiguration_.useCorrections = -1 ;
  
  return 0 ;
}


// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----


int CAEN_V1742::Print (int full)
{
  cout << " ---- ---- ---- ---- ---- ---- ---- \n" ;
  cout << " BaseAddress        " << digitizerConfiguration_.BaseAddress        << "\n" ;                                                               
  cout << " LinkType           " << digitizerConfiguration_.LinkType           << "\n" ;                                                               
  cout << " LinkNum            " << digitizerConfiguration_.LinkNum            << "\n" ;                                                               
  cout << " ConetNode          " << digitizerConfiguration_.ConetNode          << "\n" ;                                                               
  cout << " Nch                " << digitizerConfiguration_.Nch                << "\n" ;                                                               
  cout << " Nbit               " << digitizerConfiguration_.Nbit               << "\n" ;                                                               
  cout << " Ts                 " << digitizerConfiguration_.Ts                 << "\n" ;                                                               
  cout << " RecordLength       " << digitizerConfiguration_.RecordLength       << "\n" ;                                                               
  cout << " PostTrigger        " << digitizerConfiguration_.PostTrigger        << "\n" ;                                                               
  cout << " NumEvents          " << digitizerConfiguration_.NumEvents          << "\n" ;                                                               
  cout << " InterruptNumEvents " << digitizerConfiguration_.InterruptNumEvents << "\n" ;                                                               
  cout << " TestPattern        " << digitizerConfiguration_.TestPattern        << "\n" ;                                                                                                                       
  cout << " TriggerEdge        " << digitizerConfiguration_.TriggerEdge        << "\n" ;                                                               
  cout << " FPIOtype           " << digitizerConfiguration_.FPIOtype           << "\n" ;                                                               
  cout << " ExtTriggerMode     " << digitizerConfiguration_.ExtTriggerMode     << "\n" ;                                                               
  cout << " EnableMask         " << bitset<8> (digitizerConfiguration_.EnableMask)  << "\n" ;  
  cout << "    channel         " << "----3210"  << "\n" ;  
  cout << " FastTriggerMode    " << digitizerConfiguration_.FastTriggerMode    << "\n" ;                                                               
  cout << " FastTriggerEnabled " << digitizerConfiguration_.FastTriggerEnabled << "\n" ;                                                                 
  cout << " GWn                " << digitizerConfiguration_.GWn                << "\n" ;                                                               
  cout << " useCorrections     " << digitizerConfiguration_.useCorrections     << "\n" ;                                                               

  for (int i = 0 ; i < CAEN_V1742_MAXSET ; ++i) 
    {
      cout << " ---- channel " << i << " ---- ---- ---- ---- ---- \n" ;
      cout << " ChannelTriggerMode[" << i << "] : " << digitizerConfiguration_.ChannelTriggerMode[i] << "\n" ;  
      cout << " DCoffset[" << i << "]           : " << digitizerConfiguration_.DCoffset[i]           << "\n" ;  
      cout << " Threshold[" << i << "]          : " << digitizerConfiguration_.Threshold[i]          << "\n" ;  
      cout << " GroupTrgEnableMask[" << i << "] : " << bitset<8> (digitizerConfiguration_.GroupTrgEnableMask[i]) << "\n" ;  
      cout << " FTDCoffset[" << i << "]         : " << digitizerConfiguration_.FTDCoffset[i]         << "\n" ;  
      cout << " FTThreshold[" << i << "]        : " << digitizerConfiguration_.FTThreshold[i]        << "\n" ;  
      cout << "   ---- ---- ---- ---- \n" ;
      for (int j = 0 ; j < CAEN_V1742_MAXCH ; ++j) 
        cout << "   DCoffsetGrpCh[" << i << "][" << j << "] : " << digitizerConfiguration_.DCoffsetGrpCh[i][j] << "\n" ; 
    }

  if (full)
    {
      cout << " ---- ---- ---- ---- ---- ---- ---- \n" ;
      for (int i = 0 ; i < CAEN_V1742_MAXGW ; ++i) 
        {
          cout << " GWaddr[" << i << "] : " << digitizerConfiguration_.GWaddr[i] << "\n" ;                                                           
          cout << " GWdata[" << i << "] : " << digitizerConfiguration_.GWdata[i] << "\n" ;                                                           
        }
    }

  cout << " ---- ---- ---- ---- ---- ---- ---- \n" ;

}