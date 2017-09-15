#include "Unpacker.h"

//#define DEBUG_UNPACKER

#include <string>

//implementation adapted from H4DQM: https://github.com/cmsromadaq/H4DQM/blob/master/src/CAEN_V1290.cpp
//CAEN v1290 manual found in: http://www.tunl.duke.edu/documents/public/electronics/CAEN/caen_v1290.pdf
int Unpacker::Unpack (std::vector<WORD> Words) {
  for (size_t i = 0; i<Words.size(); i++) {
    uint32_t currentWord = Words[i];
    
    #ifdef DEBUG_UNPACKER 
      std::cout << "TDC WORD: " << currentWord << std::endl;
      std::cout << "TDC first 4 bits: " << (currentWord>>28) << std::endl;
    #endif
    
    if (currentWord>>28 == 10 ) { //TDC BOE
      unsigned int tdcEvent= (currentWord) & 0xFFFFFF; 
      currentData.event = tdcEvent;
      #ifdef DEBUG_UNPACKER 
        std::cout << "[CAEN_V12490][Unpack] | TDC 1190 BOE: event " << tdcEvent+1 << std::endl;
      #endif
      continue;
    }
    
    else if (currentWord>>28 == 0) {//TDC DATUM
      unsigned int channel = (currentWord>>21) & 0x1f;   //looks at the bits 22 - 27
      unsigned int measurement = currentWord & 0x1fffff;  //looks at bits 1 - 21

      #ifdef DEBUG_UNPACKER
        std::cout << "[CAEN_V12490][Unpack] | tdc 1190 board channel " << channel +" measurement " << measurement <<std::endl;
      #endif

      //check if channel exists, if not create it
      if (timeStamps.find(channel) == timeStamps.end()) {
        timeStamps[channel] = std::vector<unsigned int>();
      }

      timeStamps[channel].push_back(measurement);
      continue;
    }
    
    else if (currentWord>>28 == 8) { //TDC EOE 
      #ifdef DEBUG_UNPACKER
        std::cout << "[CAEN_V12490][Unpack] | TDC 1190 BOE: end of event " << std::endl;
      #endif
      break;
    }
   
    else if (currentWord>>28 == 4){
      unsigned int errorFlag = currentWord & 0x7FFF;
      std::cout << "TDC Error has occured with error flag: " << errorFlag << std::endl;
      break;
    } else {
      std::cout<<"NOTHING meaningful ... "<<std::endl;
      break;
    }
  }

  return 0 ;
}


tdcData Unpacker::ConvertTDCData(std::vector<WORD> Words) {

  for(std::map<unsigned int, std::vector<unsigned int> >::iterator channelTimeStamps = timeStamps.begin(); channelTimeStamps != timeStamps.end(); ++channelTimeStamps) {
    channelTimeStamps->second.clear();
  }
  timeStamps.clear();

  //unpack the stream
  Unpack(Words);
  
  for (int ch=0; ch<N_channels; ch++) {
    currentData.timeOfArrivals[ch] = -1;  //fill all 16 channels with a value indicating that no hit has been registered
    currentData.hits[ch].clear();
  } 

  for(std::map<unsigned int, std::vector<unsigned int> >::iterator channelTimeStamps = timeStamps.begin(); channelTimeStamps != timeStamps.end(); ++channelTimeStamps) {
    if (channelTimeStamps->second.size() == 0) continue;
    
    unsigned int this_channel = channelTimeStamps->first; 
    currentData.timeOfArrivals[this_channel] = *min_element(channelTimeStamps->second.begin(), channelTimeStamps->second.end());
    currentData.hits[this_channel] = channelTimeStamps->second;
    #ifdef DEBUG_UNPACKER
      std::cout<<"Number of hits in channel: "<<this_channel<<" :  "<<currentData.hits[this_channel].size()<<std::endl;
    #endif
  }

  return currentData;
}
