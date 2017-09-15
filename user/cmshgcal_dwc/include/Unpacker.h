#ifndef UNPACKER_H
#define UNPACKER_H

#include <fstream>
#include <iostream>
#include <string>
#include <cstdlib>
#include <algorithm>
#include <map>
#include <vector>
#include <algorithm> 

#include "eudaq/Logger.hh"
#include "CAEN_v1290.h"

#define WORDSIZE 4

struct tdcData {
  int event;
  int ID;
  std::map<unsigned int, unsigned int> timeOfArrivals;  //like in September 2016 test-beam: minimum of all read timestamps 
  std::map<unsigned int, std::vector<unsigned int> >hits;

} ;

#define DEBUG_BOARD 0

class Unpacker {
  public: 
    Unpacker(int N) {N_channels = N;}
    tdcData ConvertTDCData(std::vector<WORD>);
    
  private:
    int Unpack (std::vector<WORD>) ;
    std::map<unsigned int, std::vector<unsigned int> > timeStamps;   
    tdcData currentData;
    int N_channels;
};

#endif