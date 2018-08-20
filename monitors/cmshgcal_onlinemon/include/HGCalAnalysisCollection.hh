//Thorben Quast, thorben.quast@cern.ch
//June 2017

// -*- mode: c -*-

#ifndef HGCALANALYSISCOLLECTION_HH_
#define HGCALANALYSISCOLLECTION_HH_
// ROOT Includes
#include <RQ_OBJECT.h>
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
#include "HGCalAnalysisHistos.hh"
#include "BaseCollection.hh"

class HGCalAnalysisCollection : public BaseCollection {
  RQ_OBJECT("HGCalAnalysisCollection")
protected:
  bool isOnePlaneRegistered;
  std::map<eudaq::StandardPlane, HGCalAnalysisHistos*> _map;
  bool isPlaneRegistered(eudaq::StandardPlane p);
  void fillHistograms(const eudaq::StandardEvent &ev, const eudaq::StandardPlane &plane, int evNumber);
  int NumberOfDWCPlanes;

public:
  void registerPlane(const eudaq::StandardPlane &p);
  void bookHistograms(const eudaq::StandardEvent &ev);
  void setRootMonitor(RootMonitor *mon) { _mon = mon; }
  HGCalAnalysisCollection() : BaseCollection() {
    std::cout << " Initialising HGCal Analysis Collection" << std::endl;
    isOnePlaneRegistered = false;
    CollectionType = HGCALANALYSIS_COLLECTION_TYPE;
    NumberOfDWCPlanes = -1;
  }
  void Fill(const SimpleStandardEvent &simpev) { ; };
  void Fill(const eudaq::StandardEvent &ev, int evNumber = -1);
  HGCalAnalysisHistos *getHGCalAnalysisHistos(std::string sensor, int id);
  void Reset();
  virtual void Write(TFile *file);
  virtual void Calculate(const unsigned int currentEventNumber);
};

#ifdef __CINT__
#pragma link C++ class HGCalAnalysisCollection - ;
#endif

#endif /* HGCALANALYSISCOLLECTION_HH_ */
