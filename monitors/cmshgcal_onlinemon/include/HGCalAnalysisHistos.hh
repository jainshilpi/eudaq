//Thorben Quast, thorben.quast@cern.ch
//July 2018

// -*- mode: c -*-

#ifndef HGCALANALYSISHISTOS_HH_
#define HGCALANALYSISHISTOS_HH_

#include <TH2F.h>
#include <TH1F.h>
#include <TFile.h>

#include <map>

#include "eudaq/StandardEvent.hh"

using namespace std;

class RootMonitor;

class HGCalAnalysisHistos {
protected:
  string _sensor;
  int _id;
  int _maxX;
  int _maxY;
  bool _wait;


  TH1F *h1_nhits;
  TH1F *h1_energysum;
  TH1F *h1_depth;

  TH2F *h2_nhits_vs_energysum;
  TH2F *h2_nits_vs_depth;
  TH2F *h2_energysum_vs_depth;



public:
  HGCalAnalysisHistos(eudaq::StandardPlane p, RootMonitor *mon, int _NDWCs);

  void Fill(const eudaq::StandardPlane &plane, int eventNumber);
  void Reset();

  void Calculate(const int currentEventNum);
  void Write();


  TH1F* getNHits() {return h1_nhits;}
  TH1F* getEnergysum() {return h1_energysum;}
  TH1F* getDepth() {return h1_depth;}

  TH2F* getNhits_vs_Energysum() {return h2_nhits_vs_energysum;}
  TH2F* getNits_vs_Depth() {return h2_nits_vs_depth;}
  TH2F* getEnergysum_vs_Depth() {return h2_energysum_vs_depth;  }

  //TH2F *getHitOccupancy() { return _hitOccupancy; }
  //TH2F *getHitProbability() { return _hitProbability; }


  void setRootMonitor(RootMonitor *mon) { _mon = mon; }

private:
  int SetHistoAxisLabelx(TH2 *histo, string xlabel);
  int SetHistoAxisLabely(TH2 *histo, string ylabel);
  int SetHistoAxisLabels(TH2 *histo, string xlabel, string ylabel);
  int SetHistoAxisLabelx(TH1 *histo, string xlabel);
  int SetHistoAxisLabely(TH1 *histo, string ylabel);
  int SetHistoAxisLabels(TH1 *histo, string xlabel, string ylabel);
  int filling_counter; // we don't need occupancy to be refreshed for every
  // single event

  RootMonitor *_mon;

};


#ifdef __CINT__
#pragma link C++ class HGCalAnalysisHistos - ;
#endif

#endif /* HGCALANALYSISHISTOS_HH_ */
