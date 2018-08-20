// -*- mode: c -*-

#include <TROOT.h>
#include "HGCalAnalysisHistos.hh"
#include "OnlineMon.hh"
#include "TCanvas.h"
#include <cstdlib>
#include <sstream>
#include <cmath>

HGCalAnalysisHistos::HGCalAnalysisHistos(eudaq::StandardPlane p, RootMonitor *mon, int _NDWCs)
  : _sensor(p.Sensor()), _id(p.ID()), _maxX(p.XSize()),  _maxY(p.YSize()), _wait(false) ,
    h1_nhits(NULL), h1_energysum(NULL), h1_depth(NULL), h2_nhits_vs_energysum(NULL), h2_nits_vs_depth(NULL), h2_energysum_vs_depth(NULL) {


  char out[1024], out2[1024];

  //ranges of the histograms, hard coded, could be replaced by the configuration?
  float max_NHits = 250;
  float max_NEnergySum = 5000;   //MIPs
  float max_Depth = 28;   //number of Hexaboards

  _mon = mon;

  // std::cout << "HGCalAnalysisHistos::Sensorname: " << _sensor << " "<< _id<<
  // std::endl;

  sprintf(out, "Number of hits");
  sprintf(out2, "Nhits_%s_%i", _sensor.c_str(), _id);
  h1_nhits = new TH1F(out2, out, 50, 0, max_NHits);
  SetHistoAxisLabels(h1_nhits, "NHits", "N");

  sprintf(out, "Signal sum");
  sprintf(out2, "energysum_%s_%i", _sensor.c_str(), _id);
  h1_energysum = new TH1F(out2, out, 50, 0, max_NEnergySum);
  SetHistoAxisLabels(h1_energysum, "Signal sum [MIPs]", "N");

  sprintf(out, "Shower depth");
  sprintf(out2, "depth_%s_%i", _sensor.c_str(), _id);
  h1_depth = new TH1F(out2, out, 50, 0, max_Depth);
  SetHistoAxisLabels(h1_depth, "Depth [board number]", "N");


  sprintf(out, "Number of hits vs Signal sum");
  sprintf(out2, "Nhits_vs_energysum_%s_%i", _sensor.c_str(), _id);
  h2_nhits_vs_energysum = new TH2F(out2, out, 30, 0, max_NHits, 30, 0, max_NEnergySum);
  SetHistoAxisLabels(h2_nhits_vs_energysum, "NHits", "Signal sum [MIPs]");

  sprintf(out, "Number of hits vs ");
  sprintf(out2, "Nhits_vs_depth_%s_%i", _sensor.c_str(), _id);
  h2_nits_vs_depth = new TH2F(out2, out, 30, 0, max_NHits, 30, 0, max_Depth);
  SetHistoAxisLabels(h2_nits_vs_depth, "NHits", "Depth [board number]");

  sprintf(out, "Signal sum vs Shower depth");
  sprintf(out2, "energysum_vs_depth_%s_%i", _sensor.c_str(), _id);
  h2_energysum_vs_depth = new TH2F(out2, out, 30, 0, max_NEnergySum, 30, 0, max_Depth);
  SetHistoAxisLabels(h2_energysum_vs_depth, "Signal sum [MIPs]", "Depth [board number]");


  //helper objects
}

void HGCalAnalysisHistos::Fill(const eudaq::StandardPlane &plane, int eventNumber) {
  float nhits = plane.GetPixel(0, 0);
  float energysum = plane.GetPixel(1, 0);
  float depth = plane.GetPixel(2, 0);

  h1_nhits->Fill(nhits);
  h1_energysum->Fill(energysum);
  h1_depth->Fill(depth);
  h2_nhits_vs_energysum->Fill(nhits, energysum);
  h2_nits_vs_depth->Fill(nhits, depth);
  h2_energysum_vs_depth->Fill(energysum, depth);
}

void HGCalAnalysisHistos::Reset() {
  h1_nhits->Reset();
  h1_energysum->Reset();
  h1_depth->Reset();
  h2_nhits_vs_energysum->Reset();
  h2_nits_vs_depth->Reset();
  h2_energysum_vs_depth->Reset();
}

void HGCalAnalysisHistos::Calculate(const int currentEventNum) {
  _wait = true;

  _wait = false;
}

void HGCalAnalysisHistos::Write() {
  h1_nhits->Write();
  h1_energysum->Write();
  h1_depth->Write();
  h2_nhits_vs_energysum->Write();
  h2_nits_vs_depth->Write();
  h2_energysum_vs_depth->Write();

}

int HGCalAnalysisHistos::SetHistoAxisLabelx(TH2 *histo, string xlabel) {
  if (histo != NULL) {
    histo->GetXaxis()->SetTitle(xlabel.c_str());
  }
  return 0;
}

int HGCalAnalysisHistos::SetHistoAxisLabels(TH2 *histo, string xlabel, string ylabel) {
  SetHistoAxisLabelx(histo, xlabel);
  SetHistoAxisLabely(histo, ylabel);

  return 0;
}

int HGCalAnalysisHistos::SetHistoAxisLabely(TH2 *histo, string ylabel) {
  if (histo != NULL) {
    histo->GetYaxis()->SetTitle(ylabel.c_str());
  }
  return 0;
}


int HGCalAnalysisHistos::SetHistoAxisLabelx(TH1 *histo, string xlabel) {
  if (histo != NULL) {
    histo->GetXaxis()->SetTitle(xlabel.c_str());
  }
  return 0;
}

int HGCalAnalysisHistos::SetHistoAxisLabels(TH1 *histo, string xlabel, string ylabel) {
  SetHistoAxisLabelx(histo, xlabel);
  SetHistoAxisLabely(histo, ylabel);

  return 0;
}

int HGCalAnalysisHistos::SetHistoAxisLabely(TH1 *histo, string ylabel) {
  if (histo != NULL) {
    histo->GetYaxis()->SetTitle(ylabel.c_str());
  }
  return 0;
}