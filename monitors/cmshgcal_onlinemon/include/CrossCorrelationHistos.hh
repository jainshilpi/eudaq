// -*- mode: c -*-

#ifndef CROSSCORRELATIONHISTOS_HH_
#define CROSSCORRELATIONHISTOS_HH_

#include <TH2F.h>
#include <TH1F.h>
#include <TH2Poly.h>
#include <TFile.h>

#include <map>
#include <cstdlib>

#include "eudaq/StandardEvent.hh"

using namespace std;

class RootMonitor;


class CrossCorrelationHistos {
protected:
  string _sensor;
  int _id;
  int _maxX;
  int _maxY;
  bool _wait;

  map < pair < int,int >, int > _ski_to_ch_map; //channel mapping
  
  TH2F *_ChannelVsMIMOSA26_X;
  TH2F *_ChannelVsMIMOSA26_Y;

  
public:
  CrossCorrelationHistos(eudaq::StandardPlane p, RootMonitor *mon);

  void findClusters(std::vector<std::pair<int, int> >& entities, std::vector<std::pair<float, float> >& clusters );

  void Fill(const eudaq::StandardPlane &plane, const eudaq::StandardPlane &plMIMOSA1);
  void Set_SkiToHexaboard_ChannelMap();
  void Reset();

  void Calculate(const int currentEventNum);
  void Write();

  TH2F *getChannelVsMIMOSA26_XHisto() { return _ChannelVsMIMOSA26_X; }
  TH2F *getChannelVsMIMOSA26_YHisto() { return _ChannelVsMIMOSA26_Y; }

  
  void setRootMonitor(RootMonitor *mon) { _mon = mon; }

private:
  int **plane_map_array; // store an array representing the map
  int zero_plane_array(); // fill array with zeros;
  int SetHistoAxisLabelx(TH1 *histo, string xlabel);
  int SetHistoAxisLabely(TH1 *histo, string ylabel);
  int SetHistoAxisLabels(TH1 *histo, string xlabel, string ylabel);
  int filling_counter; // we don't need occupancy to be refreshed for every
                       // single event
  
  RootMonitor *_mon;

  bool is_WIRECHAMBER;
};


#ifdef __CINT__
#pragma link C++ class CrossCorrelationHistos - ;
#endif

#endif /* CROSSCORRELATIONHISTOS_HH_ */
