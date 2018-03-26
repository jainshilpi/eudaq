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
  _HGCalXVsMIMOSA26_X(NULL), _HGCalYVsMIMOSA26_Y(NULL), _HGCalXVsMIMOSA26_X_rot30(NULL), _HGCalYVsMIMOSA26_Y_rot30(NULL){
    
  char out[1024], out2[1024];

  _mon = mon;

  // std::cout << "CrossCorrelationHistos::Sensorname: " << _sensor << " "<< _id<<
  // std::endl;

  if (_maxX != -1 && _maxY != -1) {

    sprintf(out, "%s %i pixel X vs. MIMOSA1 x", _sensor.c_str(), _id);
    sprintf(out2, "h_hgcal_x_rot0_vs_MIMOSA1x_%s_%i", _sensor.c_str(), _id);
    _HGCalXVsMIMOSA26_X = new TH2F(out2, out, 100, -6.5, 6.5, 1153, 0, 1153);
    _HGCalXVsMIMOSA26_X->SetOption("COLZ");
    _HGCalXVsMIMOSA26_X->SetStats(false);
    SetHistoAxisLabels(_HGCalXVsMIMOSA26_X, "Hexaboard x [cm]", "pixel X on MIMOSA 3");

    sprintf(out, "%s %i pixel X vs. MIMOSA1 y", _sensor.c_str(), _id);
    sprintf(out2, "h_hgcal_y_rot0_vs_MIMOSA1y_%s_%i", _sensor.c_str(), _id);
    _HGCalYVsMIMOSA26_Y = new TH2F(out2, out, 100, -6.5, 6.5, 557, 0, 557);
    _HGCalYVsMIMOSA26_Y->SetOption("COLZ");
    _HGCalYVsMIMOSA26_Y->SetStats(false);
    SetHistoAxisLabels(_HGCalYVsMIMOSA26_Y, "Hexaboard y [cm]", "pixel Y on MIMOSA 3");

    sprintf(out, "%s %i pixel X (rot 30) vs. MIMOSA1 x", _sensor.c_str(), _id);
    sprintf(out2, "h_hgcal_x_rot30_vs_MIMOSA1x_%s_%i", _sensor.c_str(), _id);
    _HGCalXVsMIMOSA26_X_rot30 = new TH2F(out2, out, 100, -6.5, 6.5, 1153, 0, 1153);
    _HGCalXVsMIMOSA26_X_rot30->SetOption("COLZ");
    _HGCalXVsMIMOSA26_X_rot30->SetStats(false);
    SetHistoAxisLabels(_HGCalXVsMIMOSA26_X_rot30, "Hexaboard x, rot30 [cm]", "pixel X on MIMOSA 3");

    sprintf(out, "%s %i pixel X (rot 30) vs. MIMOSA1 y", _sensor.c_str(), _id);
    sprintf(out2, "h_hgcal_y_rot30_vs_MIMOSA1y_%s_%i", _sensor.c_str(), _id);
    _HGCalYVsMIMOSA26_Y_rot30 = new TH2F(out2, out, 100, -6.5, 6.5, 557, 0, 557);
    _HGCalYVsMIMOSA26_Y_rot30->SetOption("COLZ");
    _HGCalYVsMIMOSA26_Y_rot30->SetStats(false);
    SetHistoAxisLabels(_HGCalYVsMIMOSA26_Y_rot30, "Hexaboard y, rot30 [cm]", "pixel Y on MIMOSA 3");

    sprintf(out, "%s %i pixel X (rot 60) vs. MIMOSA1 x", _sensor.c_str(), _id);
    sprintf(out2, "h_hgcal_x_rot60_vs_MIMOSA1x_%s_%i", _sensor.c_str(), _id);
    _HGCalXVsMIMOSA26_X_rot60 = new TH2F(out2, out, 100, -6.5, 6.5, 1153, 0, 1153);
    _HGCalXVsMIMOSA26_X_rot60->SetOption("COLZ");
    _HGCalXVsMIMOSA26_X_rot60->SetStats(false);
    SetHistoAxisLabels(_HGCalXVsMIMOSA26_X_rot60, "Hexaboard x, rot60 [cm]", "pixel X on MIMOSA 3");

    sprintf(out, "%s %i pixel X (rot 60) vs. MIMOSA1 y", _sensor.c_str(), _id);
    sprintf(out2, "h_hgcal_y_rot60_vs_MIMOSA1y_%s_%i", _sensor.c_str(), _id);
    _HGCalYVsMIMOSA26_Y_rot60 = new TH2F(out2, out, 100, -6.5, 6.5, 557, 0, 557);
    _HGCalYVsMIMOSA26_Y_rot60->SetOption("COLZ");
    _HGCalYVsMIMOSA26_Y_rot60->SetStats(false);
    SetHistoAxisLabels(_HGCalYVsMIMOSA26_Y_rot60, "Hexaboard y, rot60 [cm]", "pixel Y on MIMOSA 3");

    sprintf(out, "%s %i pixel X (rot 90) vs. MIMOSA1 x", _sensor.c_str(), _id);
    sprintf(out2, "h_hgcal_x_rot90_vs_MIMOSA1x_%s_%i", _sensor.c_str(), _id);
    _HGCalXVsMIMOSA26_X_rot90 = new TH2F(out2, out, 100, -6.5, 6.5, 1153, 0, 1153);
    _HGCalXVsMIMOSA26_X_rot90->SetOption("COLZ");
    _HGCalXVsMIMOSA26_X_rot90->SetStats(false);
    SetHistoAxisLabels(_HGCalXVsMIMOSA26_X_rot90, "Hexaboard x, rot90 [cm]", "pixel X on MIMOSA 3");

    sprintf(out, "%s %i pixel X (rot 90) vs. MIMOSA1 y", _sensor.c_str(), _id);
    sprintf(out2, "h_hgcal_y_rot90_vs_MIMOSA1y_%s_%i", _sensor.c_str(), _id);
    _HGCalYVsMIMOSA26_Y_rot90 = new TH2F(out2, out, 100, -6.5, 6.5, 557, 0, 557);
    _HGCalYVsMIMOSA26_Y_rot90->SetOption("COLZ");
    _HGCalYVsMIMOSA26_Y_rot90->SetStats(false);
    SetHistoAxisLabels(_HGCalYVsMIMOSA26_Y_rot90, "Hexaboard y, rot90 [cm]", "pixel Y on MIMOSA 3");
    
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

  Set_SkiToHexaboard_ChannelMap();
  Set_UV_FromChannelMap();
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
    int pixelX = plMIMOSA1.GetX(ipix), pixelY = plMIMOSA1.GetY(ipix);
    clusterEntitites.push_back(std::make_pair(pixelX, pixelY));     
  }

  //energy weighted sum to infer the impact position on the HGCal modules
  float x_sum = 0, y_sum = 0;
  float E_sum = 0;
  // std::cout<< "FILL with a plane." << std::endl;
  for (unsigned int pix = 0; pix < plane.HitPixels(); pix++) {
    const int pixel_x = plane.GetX(pix);    //corresponds to the skiroc index
    const int pixel_y = plane.GetY(pix);    //corresponds to the channel id
    
    int u = electronicsMap[std::make_pair(pixel_x, pixel_y)].first;
    int v = electronicsMap[std::make_pair(pixel_x, pixel_y)].second;
    float x = 1.12583 * (u + v/2.); //cm, cell side x (sqrt 3 x u + (sqrt 3) / 2 x v)
    float y = 0.975 * v;    //cm, 1.5 x cell side x v

    const int ch  = _ski_to_ch_map.find(make_pair(pixel_x,pixel_y))->second;
    double energyHG_estimate = plane.GetPixel(pix, 3+13) - plane.GetPixel(pix, 0+13);

    if (energyHG_estimate < 10.) continue; 
    
    x_sum += energyHG_estimate * x;
    y_sum += energyHG_estimate * y;
    E_sum += energyHG_estimate;

  }

  float x = x_sum/E_sum;
  float y = y_sum/E_sum;
  float x_rot30 = 0.8660254038 * x + 0.5 * y;   //HGCAL sensors are rotated virtually
  float y_rot30 = -0.5 * x + 0.8660254038 * y;
  float x_rot60 = 0.5 * x + 0.8660254038 * y;
  float y_rot60 = -0.8660254038 * x + 0.5 * y;
  float x_rot90 = y;
  float y_rot90 = -x;

  //find the clusters      
  findClusters(clusterEntitites, clusters);
  for (size_t cl=0; cl<clusters.size(); cl++) {
    _HGCalXVsMIMOSA26_X->Fill(x, clusters[cl].first);
    _HGCalYVsMIMOSA26_Y->Fill(y, clusters[cl].second);
    _HGCalXVsMIMOSA26_X_rot30->Fill(x_rot30, clusters[cl].first);
    _HGCalYVsMIMOSA26_Y_rot30->Fill(y_rot30, clusters[cl].second);
    _HGCalXVsMIMOSA26_X_rot60->Fill(x_rot60, clusters[cl].first);
    _HGCalYVsMIMOSA26_Y_rot60->Fill(y_rot60, clusters[cl].second);
    _HGCalXVsMIMOSA26_X_rot90->Fill(x_rot90, clusters[cl].first);
    _HGCalYVsMIMOSA26_Y_rot90->Fill(y_rot90, clusters[cl].second);
  }
  
  
}

void CrossCorrelationHistos::Set_UV_FromChannelMap() {
  electronicsMap[std::make_pair(1,0)]=std::make_pair(-2,5);
  electronicsMap[std::make_pair(1,2)]=std::make_pair(-2,4);
  electronicsMap[std::make_pair(1,4)]=std::make_pair(-1,4);
  electronicsMap[std::make_pair(1,6)]=std::make_pair(-2,6);
  electronicsMap[std::make_pair(1,8)]=std::make_pair(0,3);
  electronicsMap[std::make_pair(1,10)]=std::make_pair(-1,6);
  electronicsMap[std::make_pair(1,12)]=std::make_pair(-1,5);
  electronicsMap[std::make_pair(1,14)]=std::make_pair(0,2);
  electronicsMap[std::make_pair(1,16)]=std::make_pair(-1,3);
  electronicsMap[std::make_pair(1,18)]=std::make_pair(-1,2);
  electronicsMap[std::make_pair(1,20)]=std::make_pair(-2,3);
  electronicsMap[std::make_pair(1,22)]=std::make_pair(-2,2);
  electronicsMap[std::make_pair(1,24)]=std::make_pair(-1,1);
  electronicsMap[std::make_pair(1,26)]=std::make_pair(-2,1);
  electronicsMap[std::make_pair(1,28)]=std::make_pair(-3,2);
  electronicsMap[std::make_pair(1,30)]=std::make_pair(-4,2);
  electronicsMap[std::make_pair(1,32)]=std::make_pair(-5,3);
  electronicsMap[std::make_pair(1,34)]=std::make_pair(-4,3);
  electronicsMap[std::make_pair(1,36)]=std::make_pair(-5,4);
  electronicsMap[std::make_pair(1,38)]=std::make_pair(-4,4);
  electronicsMap[std::make_pair(1,40)]=std::make_pair(-3,3);
  electronicsMap[std::make_pair(1,42)]=std::make_pair(-3,4);
  electronicsMap[std::make_pair(1,44)]=std::make_pair(-6,3);
  electronicsMap[std::make_pair(1,46)]=std::make_pair(-3,7);
  electronicsMap[std::make_pair(1,48)]=std::make_pair(-6,4);
  electronicsMap[std::make_pair(1,50)]=std::make_pair(-6,5);
  electronicsMap[std::make_pair(1,52)]=std::make_pair(-5,5);
  electronicsMap[std::make_pair(1,54)]=std::make_pair(-4,5);
  electronicsMap[std::make_pair(1,56)]=std::make_pair(-5,6);
  electronicsMap[std::make_pair(1,58)]=std::make_pair(-3,6);
  electronicsMap[std::make_pair(1,60)]=std::make_pair(-3,5);
  electronicsMap[std::make_pair(1,62)]=std::make_pair(-4,6);
  electronicsMap[std::make_pair(2,0)]=std::make_pair(-3,-2);
  electronicsMap[std::make_pair(2,2)]=std::make_pair(-4,-2);
  electronicsMap[std::make_pair(2,4)]=std::make_pair(-3,-3);
  electronicsMap[std::make_pair(2,6)]=std::make_pair(-4,-1);
  electronicsMap[std::make_pair(2,8)]=std::make_pair(-5,0);
  electronicsMap[std::make_pair(2,10)]=std::make_pair(-5,-1);
  electronicsMap[std::make_pair(2,12)]=std::make_pair(-6,1);
  electronicsMap[std::make_pair(2,14)]=std::make_pair(-4,0);
  electronicsMap[std::make_pair(2,16)]=std::make_pair(-3,-1);
  electronicsMap[std::make_pair(2,18)]=std::make_pair(-3,0);
  electronicsMap[std::make_pair(2,20)]=std::make_pair(-7,3);
  electronicsMap[std::make_pair(2,22)]=std::make_pair(-6,2);
  electronicsMap[std::make_pair(2,24)]=std::make_pair(-5,1);
  electronicsMap[std::make_pair(2,26)]=std::make_pair(-4,1);
  electronicsMap[std::make_pair(2,28)]=std::make_pair(-5,2);
  electronicsMap[std::make_pair(2,30)]=std::make_pair(-3,1);
  electronicsMap[std::make_pair(2,32)]=std::make_pair(-2,0);
  electronicsMap[std::make_pair(2,34)]=std::make_pair(-1,0);
  electronicsMap[std::make_pair(2,36)]=std::make_pair(-1,0);
  electronicsMap[std::make_pair(2,38)]=std::make_pair(-1,-1);
  electronicsMap[std::make_pair(2,40)]=std::make_pair(0,-4);
  electronicsMap[std::make_pair(2,42)]=std::make_pair(0,-3);
  electronicsMap[std::make_pair(2,44)]=std::make_pair(-1,-3);
  electronicsMap[std::make_pair(2,46)]=std::make_pair(0,-5);
  electronicsMap[std::make_pair(2,48)]=std::make_pair(-1,-5);
  electronicsMap[std::make_pair(2,50)]=std::make_pair(-1,-4);
  electronicsMap[std::make_pair(2,52)]=std::make_pair(-1,-2);
  electronicsMap[std::make_pair(2,54)]=std::make_pair(-2,-1);
  electronicsMap[std::make_pair(2,56)]=std::make_pair(-2,-2);
  electronicsMap[std::make_pair(2,58)]=std::make_pair(-2,-3);
  electronicsMap[std::make_pair(2,60)]=std::make_pair(-3,-4);
  electronicsMap[std::make_pair(2,62)]=std::make_pair(-2,-4);
  electronicsMap[std::make_pair(3,0)]=std::make_pair(3,-4);
  electronicsMap[std::make_pair(3,2)]=std::make_pair(2,-4);
  electronicsMap[std::make_pair(3,4)]=std::make_pair(3,-5);
  electronicsMap[std::make_pair(3,6)]=std::make_pair(4,-6);
  electronicsMap[std::make_pair(3,8)]=std::make_pair(1,-5);
  electronicsMap[std::make_pair(3,10)]=std::make_pair(3,-6);
  electronicsMap[std::make_pair(3,12)]=std::make_pair(4,-7);
  electronicsMap[std::make_pair(3,14)]=std::make_pair(2,-6);
  electronicsMap[std::make_pair(3,16)]=std::make_pair(1,-6);
  electronicsMap[std::make_pair(3,18)]=std::make_pair(2,-5);
  electronicsMap[std::make_pair(3,20)]=std::make_pair(1,-4);
  electronicsMap[std::make_pair(3,22)]=std::make_pair(1,-3);
  electronicsMap[std::make_pair(3,24)]=std::make_pair(2,-3);
  electronicsMap[std::make_pair(3,26)]=std::make_pair(1,-2);
  electronicsMap[std::make_pair(3,28)]=std::make_pair(0,-2);
  electronicsMap[std::make_pair(3,30)]=std::make_pair(3,-3);
  electronicsMap[std::make_pair(3,32)]=std::make_pair(3,-2);
  electronicsMap[std::make_pair(3,34)]=std::make_pair(2,-2);
  electronicsMap[std::make_pair(3,36)]=std::make_pair(0,0);
  electronicsMap[std::make_pair(3,38)]=std::make_pair(1,-1);
  electronicsMap[std::make_pair(3,40)]=std::make_pair(2,-1);
  electronicsMap[std::make_pair(3,42)]=std::make_pair(4,-2);
  electronicsMap[std::make_pair(3,44)]=std::make_pair(4,-3);
  electronicsMap[std::make_pair(3,46)]=std::make_pair(5,-3);
  electronicsMap[std::make_pair(3,48)]=std::make_pair(6,-4);
  electronicsMap[std::make_pair(3,50)]=std::make_pair(5,-4);
  electronicsMap[std::make_pair(3,52)]=std::make_pair(6,-5);
  electronicsMap[std::make_pair(3,54)]=std::make_pair(4,-5);
  electronicsMap[std::make_pair(3,56)]=std::make_pair(5,-5);
  electronicsMap[std::make_pair(3,58)]=std::make_pair(4,-4);
  electronicsMap[std::make_pair(3,62)]=std::make_pair(5,-6);
  electronicsMap[std::make_pair(4,0)]=std::make_pair(2,2);
  electronicsMap[std::make_pair(4,2)]=std::make_pair(3,1);
  electronicsMap[std::make_pair(4,4)]=std::make_pair(3,2);
  electronicsMap[std::make_pair(4,6)]=std::make_pair(4,0);
  electronicsMap[std::make_pair(4,8)]=std::make_pair(3,3);
  electronicsMap[std::make_pair(4,10)]=std::make_pair(4,-1);
  electronicsMap[std::make_pair(4,12)]=std::make_pair(5,-1);
  electronicsMap[std::make_pair(4,14)]=std::make_pair(2,4);
  electronicsMap[std::make_pair(4,16)]=std::make_pair(3,4);
  electronicsMap[std::make_pair(4,18)]=std::make_pair(4,2);
  electronicsMap[std::make_pair(4,20)]=std::make_pair(4,1);
  electronicsMap[std::make_pair(4,22)]=std::make_pair(5,1);
  electronicsMap[std::make_pair(4,24)]=std::make_pair(5,0);
  electronicsMap[std::make_pair(4,26)]=std::make_pair(6,-1);
  electronicsMap[std::make_pair(4,28)]=std::make_pair(6,-2);
  electronicsMap[std::make_pair(4,30)]=std::make_pair(5,-2);
  electronicsMap[std::make_pair(4,32)]=std::make_pair(6,-3);
  electronicsMap[std::make_pair(4,34)]=std::make_pair(7,-4);
  electronicsMap[std::make_pair(4,36)]=std::make_pair(1,0);
  electronicsMap[std::make_pair(4,38)]=std::make_pair(2,0);
  electronicsMap[std::make_pair(4,40)]=std::make_pair(3,-1);
  electronicsMap[std::make_pair(4,42)]=std::make_pair(3,0);
  electronicsMap[std::make_pair(4,44)]=std::make_pair(0,1);
  electronicsMap[std::make_pair(4,46)]=std::make_pair(1,3);
  electronicsMap[std::make_pair(4,48)]=std::make_pair(0,4);
  electronicsMap[std::make_pair(4,50)]=std::make_pair(0,5);
  electronicsMap[std::make_pair(4,52)]=std::make_pair(1,2);
  electronicsMap[std::make_pair(4,54)]=std::make_pair(1,1);
  electronicsMap[std::make_pair(4,56)]=std::make_pair(2,1);
  electronicsMap[std::make_pair(4,58)]=std::make_pair(2,3);
  electronicsMap[std::make_pair(4,60)]=std::make_pair(1,5);
  electronicsMap[std::make_pair(4,62)]=std::make_pair(1,4);  
}

void CrossCorrelationHistos::Set_SkiToHexaboard_ChannelMap(){

  for(int c=0; c<127; c++){
    int row = c*3;
    _ski_to_ch_map.insert(make_pair(make_pair(sc_to_ch_map[row+1],sc_to_ch_map[row+2]),sc_to_ch_map[row]));
  }
}

void CrossCorrelationHistos::Reset() {

  
  _HGCalXVsMIMOSA26_X->Reset();
  _HGCalYVsMIMOSA26_Y->Reset();
  _HGCalXVsMIMOSA26_X_rot30->Reset();
  _HGCalYVsMIMOSA26_Y_rot30->Reset();
  _HGCalXVsMIMOSA26_X_rot60->Reset();
  _HGCalYVsMIMOSA26_Y_rot60->Reset();
  _HGCalXVsMIMOSA26_X_rot90->Reset();
  _HGCalYVsMIMOSA26_Y_rot90->Reset();
  
  // we have to reset the aux array as well
  zero_plane_array();
}

void CrossCorrelationHistos::Calculate(const int currentEventNum) {
  _wait = true;

  _wait = false;
}

void CrossCorrelationHistos::Write() {
  _HGCalXVsMIMOSA26_X->Write();
  _HGCalYVsMIMOSA26_Y->Write();
  _HGCalXVsMIMOSA26_X_rot30->Write();
  _HGCalYVsMIMOSA26_Y_rot30->Write();
  _HGCalXVsMIMOSA26_X_rot60->Write();
  _HGCalYVsMIMOSA26_Y_rot60->Write();
  _HGCalXVsMIMOSA26_X_rot90->Write();
  _HGCalYVsMIMOSA26_Y_rot90->Write();
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
