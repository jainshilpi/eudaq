// -*- mode: c -*-

#include <TROOT.h>
#include "TDCHitsHistos.hh"
#include "OnlineMon.hh"
#include "TCanvas.h"
#include <cstdlib>
#include <sstream>
#include <cmath>

TDCHitsHistos::TDCHitsHistos(eudaq::StandardPlane p, RootMonitor *mon, int _NDWCs)
  :_sensor(p.Sensor()), _id(p.ID()), _maxX(p.XSize()),  _maxY(p.YSize()), _wait(false),
  _hitOccupancy(NULL), _hitProbability(NULL), _occupancy(NULL), _hitCount(NULL), _hitSumCount(NULL){
    
  char out[1024], out2[1024];

  _mon = mon;

  // std::cout << "TDCHitsHistos::Sensorname: " << _sensor << " "<< _id<<
  // std::endl;

  NChannelsPerTDC = 16;
  NTDCs = ceil(_NDWCs*4./NChannelsPerTDC);
  
  std::cout<<"Number of TDCS: "<<NTDCs<<std::endl;
  sprintf(out, "%s %i hit occupancy", _sensor.c_str(), _id);
  sprintf(out2, "hitOccupancy_%s_%i", _sensor.c_str(), _id);
  _hitOccupancy = new TH2F(out2, out, NChannelsPerTDC, 0, NChannelsPerTDC, NTDCs, 0, NTDCs);
  SetHistoAxisLabels(_hitOccupancy, "channel", "TDC");


  sprintf(out, "%s %i hit probability", _sensor.c_str(), _id);
  sprintf(out2, "hitProbability_%s_%i", _sensor.c_str(), _id);
  _hitProbability = new TH2F(out2, out, NChannelsPerTDC, 0, NChannelsPerTDC, NTDCs, 0, NTDCs);
  SetHistoAxisLabels(_hitProbability, "channel", "TDC");  


  sprintf(out, "%s %i _occupancy", _sensor.c_str(), _id);
  sprintf(out2, "_occupancy_%s_%i", _sensor.c_str(), _id);
  _occupancy = new TH2F(out2, out, NChannelsPerTDC, 0, NChannelsPerTDC, NTDCs, 0, NTDCs);
  SetHistoAxisLabels(_occupancy, "channel", "TDC"); 

  sprintf(out, "%s %i _hitSumCount", _sensor.c_str(), _id);
  sprintf(out2, "_hitSumCount%s_%i", _sensor.c_str(), _id);
  _hitSumCount = new TH2F(out2, out, NChannelsPerTDC, 0, NChannelsPerTDC, NTDCs, 0, NTDCs);
  SetHistoAxisLabels(_hitSumCount, "channel", "TDC"); 

  sprintf(out, "%s %i _hitCount", _sensor.c_str(), _id);
  sprintf(out2, "_hitCount%s_%i", _sensor.c_str(), _id);
  _hitCount = new TH2F(out2, out, NChannelsPerTDC, 0, NChannelsPerTDC, NTDCs, 0, NTDCs);
  SetHistoAxisLabels(_hitCount, "channel", "TDC"); 

  //helper objects
}

void TDCHitsHistos::Fill(const eudaq::StandardPlane &plane) {
  // std::cout<< "FILL with a plane." << std::endl;

  for (unsigned tdc_index=0; tdc_index<NTDCs; tdc_index++)for (int channel=0; channel<NChannelsPerTDC; channel++) {
    _hitSumCount->Fill(1.*channel, 1.*tdc_index, 1.*plane.GetPixel(tdc_index*16+channel, 0));
    _hitCount->Fill(1.*channel, 1.*tdc_index, int(plane.GetPixel(tdc_index*16+channel, 0)>0));
    _occupancy->Fill(1.*channel, 1.*tdc_index);
  }

  
  for (size_t nbinx=0; nbinx<_hitSumCount->GetNbinsX(); nbinx++) for (size_t nbiny=0; nbiny<_hitSumCount->GetNbinsY(); nbiny++) {
    _hitOccupancy->SetBinContent(nbinx, nbiny, _hitSumCount->GetBinContent(nbinx, nbiny));
    _hitProbability->SetBinContent(nbinx, nbiny, _hitCount->GetBinContent(nbinx, nbiny));
  }

  _hitSumCount->Copy(*_hitOccupancy);
  _hitOccupancy->Divide(_occupancy);  

  _hitCount->Copy(*_hitProbability);
  _hitProbability->Divide(_occupancy);  
    
}

void TDCHitsHistos::Reset() {

  _hitOccupancy->Reset();
    
}

void TDCHitsHistos::Calculate(const int currentEventNum) {
  _wait = true;

  _wait = false;
}

void TDCHitsHistos::Write() {

  _hitOccupancy->Write();

}

int TDCHitsHistos::SetHistoAxisLabelx(TH2 *histo, string xlabel) {
  if (histo != NULL) {
    histo->GetXaxis()->SetTitle(xlabel.c_str());
  }
  return 0;
}

int TDCHitsHistos::SetHistoAxisLabels(TH2 *histo, string xlabel, string ylabel) {
  SetHistoAxisLabelx(histo, xlabel);
  SetHistoAxisLabely(histo, ylabel);

  return 0;
}

int TDCHitsHistos::SetHistoAxisLabely(TH2 *histo, string ylabel) {
  if (histo != NULL) {
    histo->GetYaxis()->SetTitle(ylabel.c_str());
  }
  return 0;
}
