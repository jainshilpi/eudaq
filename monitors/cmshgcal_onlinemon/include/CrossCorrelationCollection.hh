// -*- mode: c -*-

//author: Thorben Quast, thorben.quast@cern.ch
//26 March 2018

#ifndef CROSSCORRELATION_COLLECTION_
#define CROSSCORRELATION_COLLECTION_
// ROOT Includes
#include <RQ_OBJECT.h>
#include <TH2I.h>
#include <TFile.h>

// STL includes
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <iostream>

#include "eudaq/StandardEvent.hh"

// Project Includes
#include "SimpleStandardEvent.hh"
#include "CrossCorrelationHistos.hh"
#include "BaseCollection.hh"

class CrossCorrelationCollection : public BaseCollection {
  RQ_OBJECT("CrossCorrelationCollection")
protected:
  bool isOnePlaneRegistered;
  std::map<eudaq::StandardPlane, CrossCorrelationHistos *> _map;
  bool isPlaneRegistered(eudaq::StandardPlane p);
  void fillHistograms(const eudaq::StandardPlane &plane, const eudaq::StandardPlane &plMIMOSA3, const eudaq::StandardPlane &plMIMOSA4);
    
public:
  void registerPlane(const eudaq::StandardPlane &p);
  void bookHistograms(const eudaq::StandardEvent &ev);
  void setRootMonitor(RootMonitor *mon) { _mon = mon; }
  CrossCorrelationCollection() : BaseCollection() {
    std::cout << " Initialising CrossCorrelationCollection Collection" << std::endl;
    isOnePlaneRegistered = false;
    CollectionType = CROSSCORRELATION_COLLECTION_TYPE;

  }
  void Fill(const SimpleStandardEvent &simpev) { ; };
  void Fill(const eudaq::StandardEvent &ev, int evNumber=-1);
  CrossCorrelationHistos *getCrossCorrelationHistos(std::string sensor, int id);
  void Reset();
  virtual void Write(TFile *file);
  virtual void Calculate(const unsigned int currentEventNumber);

};




#ifdef __CINT__
#pragma link C++ class CrossCorrelationCollection - ;
#endif

#endif /* CROSSCORRELATION_COLLECTION_ */
