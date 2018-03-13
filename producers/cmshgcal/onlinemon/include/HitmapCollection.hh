// -*- mode: c -*-
/*
 * HitmapCollection.hh
 *
 *  Created on: Jun 16, 2011
 *      Author: stanitz
 */

#ifndef HITMAPCOLLECTION_HH_
#define HITMAPCOLLECTION_HH_
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
#include "HitmapHistos.hh"
#include "BaseCollection.hh"

class HitmapCollection : public BaseCollection {
  RQ_OBJECT("HitmapCollection")
protected:
  bool isOnePlaneRegistered;
  std::map<SimpleStandardPlane, HitmapHistos *> _map;
  std::map<eudaq::StandardPlane, HitmapHistos *> _map2;
  bool isPlaneRegistered(SimpleStandardPlane p);
  bool isPlaneRegistered(eudaq::StandardPlane p);
  void fillHistograms(const SimpleStandardPlane &simpPlane); 
  void fillHistograms(const eudaq::StandardPlane &plane);
    
public:
  void registerPlane(const SimpleStandardPlane &p);
  void registerPlane(const eudaq::StandardPlane &p);
  void bookHistograms(const SimpleStandardEvent &simpev);
  void bookHistograms(const eudaq::StandardEvent &ev);
  void setRootMonitor(RootMonitor *mon) { _mon = mon; }
  HitmapCollection() : BaseCollection() {
    std::cout << " Initialising Hitmap Collection" << std::endl;
    isOnePlaneRegistered = false;
    CollectionType = HITMAP_COLLECTION_TYPE;
  }
  void Fill(const SimpleStandardEvent &simpev);
  void Fill(const eudaq::StandardEvent &ev);
  HitmapHistos *getHitmapHistos(std::string sensor, int id);
  void Reset();
  virtual void Write(TFile *file);
  virtual void Calculate(const unsigned int currentEventNumber);
};

#ifdef __CINT__
#pragma link C++ class HitmapCollection - ;
#endif

#endif /* HITMAPCOLLECTION_HH_ */
