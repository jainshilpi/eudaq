// -*- mode: c -*-

#include <TROOT.h>
#include "CrossCorrelationHistos.hh"
#include "HexagonHistos.hh"   //for the channel mapping
#include "OnlineMon.hh"
#include "TCanvas.h"
#include <cstdlib>
#include <sstream>


double MAXCLUSTERRADIUS2 = 9.0;
float pixelGap_MIMOSA26 = 0.0184; //mm

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
  :_sensor(p.Sensor()), _id(p.ID()), _maxX(p.XSize()),  _maxY(p.YSize()), _wait(false){
    
  char out[1024], out2[1024];

  _mon = mon;

  // std::cout << "CrossCorrelationHistos::Sensorname: " << _sensor << " "<< _id<<
  // std::endl;

  if (_maxX != -1 && _maxY != -1) {

    for (int ch=0; ch<128; ch++) {
      sprintf(out, "%s %i, channel %i projected on MIMOSA26 plane 3", _sensor.c_str(), _id, ch);
      sprintf(out2, "h_hgcal_ch%i_vs_MIMOSA26_3_%s_%i", ch, _sensor.c_str(), _id);
      _MIMOSA_map_ForChannel[ch] = new TH2F(out2, out, 400, 0, 1153*pixelGap_MIMOSA26, 200, 0, 557*pixelGap_MIMOSA26);
      _MIMOSA_map_ForChannel[ch]->SetOption("COLZ");
      _MIMOSA_map_ForChannel[ch]->SetStats(false);
      SetHistoAxisLabels(_MIMOSA_map_ForChannel[ch], "MIMOSA26, plane 3, x [mm]", "MIMOSA26, plane 3, y [mm]");      
    }

    
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


void CrossCorrelationHistos::Fill(const eudaq::StandardPlane &plane, const eudaq::StandardPlane &plMIMOSA2) {
  //MIMOSA clustering
  std::vector<double> pxl = plMIMOSA2.GetPixels<double>();
  std::vector<std::pair<int, int> > clusterEntitites;
  std::vector<std::pair<float, float> > clusters;
  int _N_hits = pxl.size();
  
  clusterEntitites.clear();
  clusters.clear();
  for (size_t ipix = 0; ipix < pxl.size(); ++ipix) {
    int pixelX = plMIMOSA2.GetX(ipix), pixelY = plMIMOSA2.GetY(ipix);
    clusterEntitites.push_back(std::make_pair(pixelX, pixelY));     
  }

  //find the clusters      
  findClusters(clusterEntitites, clusters);

  for (unsigned int pix = 0; pix < plane.HitPixels(); pix++) {
    const int pixel_x = plane.GetX(pix);    //corresponds to the skiroc index
    const int pixel_y = plane.GetY(pix);    //corresponds to the channel id
    
    double energyHG_estimate = plane.GetPixel(pix, 3+13) - plane.GetPixel(pix, 0+13);
    if (energyHG_estimate < 20.) continue; 
    
    
    const int ch  = _ski_to_ch_map.find(make_pair(pixel_x,pixel_y))->second;
    for (size_t cl=0; cl<clusters.size(); cl++) {
      //_MIMOSA_map_ForChannel[ch]->Fill(clusters[cl].first, clusters[cl].second);   //binary entries
      _MIMOSA_map_ForChannel[ch]->Fill(clusters[cl].first*pixelGap_MIMOSA26, clusters[cl].second*pixelGap_MIMOSA26, energyHG_estimate);   //energy weighted entries
    }    
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

  for (int ch=0; ch<128; ch++)
    _MIMOSA_map_ForChannel[ch]->Reset();
  
  // we have to reset the aux array as well
  zero_plane_array();
}

void CrossCorrelationHistos::Calculate(const int currentEventNum) {
  _wait = true;

  _wait = false;
}

void CrossCorrelationHistos::Write() {
  for (int ch=0; ch<128; ch++)
    _MIMOSA_map_ForChannel[ch]->Write();

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
