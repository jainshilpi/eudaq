// -*- mode: c -*-

#include <TROOT.h>
#include "CrossCorrelationHistos.hh"
#include "HexagonHistos.hh"   //for the channel mapping
#include "OnlineMon.hh"
#include "TCanvas.h"
#include <cstdlib>
#include <sstream>


double MAXCLUSTERRADIUS2 = 9.0;
//some simple clustering
void CrossCorrelationHistos::findClusters(std::vector<std::pair<int, int> >& entities, std::vector<std::pair<float, float> >& clusters ) {
  std::vector<bool> used;
  for (size_t i=0; i<entities.size(); i++) used.push_back(false);

  for (size_t i=0; i<entities.size(); i++) {
    if (used[i]) continue;
    float cluster_X=entities[i].first, cluster_Y=entities[i].second;
    size_t clusterEntries=1;
    used[i]=true;
    for (size_t j=i+1; j<entities.size(); j++) {
      if (used[j]) continue;
      float d = pow(entities[j].first-cluster_X, 2) + pow(entities[j].second-cluster_Y, 2);
      if (d > MAXCLUSTERRADIUS2) continue;
      used[j] = true;
      cluster_X = (clusterEntries*cluster_X + entities[j].first);
      cluster_Y = (clusterEntries*cluster_Y + entities[j].second);
      clusterEntries++;
      cluster_X /= clusterEntries;
      cluster_Y /= clusterEntries;
    }  
  
    clusters.push_back(std::make_pair(cluster_X, cluster_Y));
  }
}



CrossCorrelationHistos::CrossCorrelationHistos(eudaq::StandardPlane p, RootMonitor *mon)
  :_sensor(p.Sensor()), _id(p.ID()), _maxX(p.XSize()),  _maxY(p.YSize()), _wait(false),
  _ChannelVsMIMOSA26_X(NULL), _ChannelVsMIMOSA26_Y(NULL){
    
  char out[1024], out2[1024];

  _mon = mon;

  // std::cout << "CrossCorrelationHistos::Sensorname: " << _sensor << " "<< _id<<
  // std::endl;

  if (_maxX != -1 && _maxY != -1) {

    sprintf(out, "%s %i channel vs. MIMOSA1 x", _sensor.c_str(), _id);
    sprintf(out2, "h_ch_vs_MIMOSA1x_%s_%i", _sensor.c_str(), _id);
    _ChannelVsMIMOSA26_X = new TH2F(out2, out, 128, 0, 128, 1153, 0, 1153);
    
    SetHistoAxisLabels(_ChannelVsMIMOSA26_X, "channel Hexaboard", "pixel X on MIMOSA 1");

    sprintf(out, "%s %i channel vs. MIMOSA1 y", _sensor.c_str(), _id);
    sprintf(out2, "h_ch_vs_MIMOSA1y_%s_%i", _sensor.c_str(), _id);
    _ChannelVsMIMOSA26_Y = new TH2F(out2, out, 128, 0, 128, 557, 0, 557);
    SetHistoAxisLabels(_ChannelVsMIMOSA26_Y, "channel Hexaboard", "pixel Y on MIMOSA 1");

    
    // make a plane array for calculating e..g hotpixels and occupancy
    plane_map_array = new int *[_maxX];

    if (plane_map_array != NULL) {
      for (int j = 0; j < _maxX; j++) {
        plane_map_array[j] = new int[_maxY];
        if (plane_map_array[j] == NULL) {
          cout << "CrossCorrelationHistos :Error in memory allocation" << endl;
          exit(-1);
        }
      }
      zero_plane_array();
    }

  } else {
    std::cout << "No max sensorsize known!" << std::endl;
  }


}

int CrossCorrelationHistos::zero_plane_array() {
  for (int i = 0; i < _maxX; i++) {
    for (int j = 0; j < _maxY; j++) {
      plane_map_array[i][j] = 0;
    }
  }
  return 0;
}


void CrossCorrelationHistos::Fill(const eudaq::StandardPlane &plane, const eudaq::StandardPlane &plMIMOSA1) {
  //MIMOSA clustering
  std::vector<double> pxl = plMIMOSA1.GetPixels<double>();
  std::vector<std::pair<int, int> > clusterEntitites;
  std::vector<std::pair<float, float> > clusters;
  int _N_hits = pxl.size();
  
  clusterEntitites.clear();
  clusters.clear();
  for (size_t ipix = 0; ipix < pxl.size(); ++ipix) {
    int pixelX = plane.GetX(ipix), pixelY = plane.GetY(ipix);
    clusterEntitites.push_back(std::make_pair(pixelX, pixelY));     
  }

  
  //find the clusters      
  findClusters(clusterEntitites, clusters);


  // std::cout<< "FILL with a plane." << std::endl;
  for (unsigned int pix = 0; pix < plane.HitPixels(); pix++) {
    const int pixel_x = plane.GetX(pix);
    const int pixel_y = plane.GetY(pix);
    const int ch  = _ski_to_ch_map.find(make_pair(pixel_x,pixel_y))->second;
    double HG_TS3 = plane.GetPixel(pix, 3+13);

    if (HG_TS3 < 30.) continue; //only fill if hexaboard hit is above 30
    //end of hexaboard loop

    for (size_t cl=0; cl<clusters.size(); cl++) {
      _ChannelVsMIMOSA26_X->Fill(ch*1., clusters[cl].first);
      _ChannelVsMIMOSA26_Y->Fill(ch*1., clusters[cl].second);
    }
  }
  
  
}

void CrossCorrelationHistos::Set_SkiToHexaboard_ChannelMap(){

  for(int c=0; c<127; c++){
    int row = c*3;
    _ski_to_ch_map.insert(make_pair(make_pair(sc_to_ch_map[row+1],sc_to_ch_map[row+2]),sc_to_ch_map[row]));
  }
}

void CrossCorrelationHistos::Reset() {

  
  _ChannelVsMIMOSA26_X->Reset();
  _ChannelVsMIMOSA26_Y->Reset();
  
  // we have to reset the aux array as well
  zero_plane_array();
}

void CrossCorrelationHistos::Calculate(const int currentEventNum) {
  _wait = true;

  _wait = false;
}

void CrossCorrelationHistos::Write() {

  _ChannelVsMIMOSA26_X->Write();
  _ChannelVsMIMOSA26_Y->Write();

}

int CrossCorrelationHistos::SetHistoAxisLabelx(TH1 *histo, string xlabel) {
  if (histo != NULL) {
    histo->GetXaxis()->SetTitle(xlabel.c_str());
  }
  return 0;
}

int CrossCorrelationHistos::SetHistoAxisLabels(TH1 *histo, string xlabel, string ylabel) {
  SetHistoAxisLabelx(histo, xlabel);
  SetHistoAxisLabely(histo, ylabel);

  return 0;
}

int CrossCorrelationHistos::SetHistoAxisLabely(TH1 *histo, string ylabel) {
  if (histo != NULL) {
    histo->GetYaxis()->SetTitle(ylabel.c_str());
  }
  return 0;
}
