// -*- mode: c -*-
/*
 * HitmapHistos.cc
 *
 *  Created on: Jun 16, 2011
 *      Author: stanitz
 */

#include <TROOT.h>
#include "HitmapHistos.hh"
#include "OnlineMon.hh"
#include "TCanvas.h"
#include "TGraph.h"
#include <cstdlib>

HitmapHistos::HitmapHistos(SimpleStandardPlane p, RootMonitor *mon)
    : _sensor(p.getName()), _id(p.getID()), _maxX(p.getMaxX()),
      _maxY(p.getMaxY()), _wait(false),
      _hexagons_occ_adc(NULL), _hexagons_charge(NULL), _hexagons_occ_tot(NULL), _hexagons_occ_toa(NULL), 
      _hitmap(NULL), _hitXmap(NULL),
      _hitYmap(NULL), _clusterMap(NULL), _lvl1Distr(NULL), _lvl1Width(NULL),
      _lvl1Cluster(NULL), _totSingle(NULL), _totCluster(NULL), _hitOcc(NULL),
      _nClusters(NULL), _nHits(NULL), _clusterXWidth(NULL),
      _clusterYWidth(NULL), _nbadHits(NULL), _nHotPixels(NULL),
      _hitmapSections(NULL), is_MIMOSA26(false), is_APIX(false),
      is_USBPIX(false), is_USBPIXI4(false), is_HEXABOARD(false){
  char out[1024], out2[1024];

  _mon = mon;
  mimosa26_max_section = _mon->mon_configdata.getMimosa26_max_sections();
  if (_sensor == std::string("MIMOSA26")) {
    is_MIMOSA26 = true;
  } else if (_sensor == std::string("APIX")) {
    is_APIX = true;
  } else if (_sensor == std::string("USBPIX")) {
    is_USBPIX = true;
  } else if (_sensor == std::string("USBPIXI4")) {
    is_USBPIXI4 = true;
  } else if (_sensor == std::string("USBPIXI4B")) {
    is_USBPIXI4 = true;
  } else if (_sensor == std::string("HexaBoard")) {
    is_HEXABOARD = true;
  }
  is_DEPFET = p.is_DEPFET;

  // std::cout << "HitmapHistos::Sensorname: " << _sensor << " "<< _id<<
  // std::endl;

  if (_maxX != -1 && _maxY != -1) {
    sprintf(out, "%s %i  ADC HG Occupancy", _sensor.c_str(), _id);
    sprintf(out2, "h_hexagons_occ_adc_%s_%i", _sensor.c_str(), _id);
    _hexagons_occ_adc = get_th2poly(out2,out);  
    sprintf(out, "%s %i  ADC HG Charge", _sensor.c_str(), _id);
    sprintf(out2, "h_hexagons_charge_%s_%i", _sensor.c_str(), _id);
    _hexagons_charge = get_th2poly(out2,out); 
    sprintf(out, "%s %i  TOT Occupancy", _sensor.c_str(), _id);
    sprintf(out2, "h_hexagons_occ_tot_%s_%i", _sensor.c_str(), _id);
    _hexagons_occ_tot = get_th2poly(out2,out); 
    sprintf(out, "%s %i  TOA Occupancy", _sensor.c_str(), _id);
    sprintf(out2, "h_hexagons_occ_toa_%s_%i", _sensor.c_str(), _id);
    _hexagons_occ_toa = get_th2poly(out2,out);

    
    sprintf(out, "%s %i Raw Hitmap", _sensor.c_str(), _id);
    sprintf(out2, "h_hitmap_%s_%i", _sensor.c_str(), _id);
    _hitmap = new TH2I(out2, out, _maxX + 1, 0, _maxX, _maxY + 1, 0, _maxY);
    SetHistoAxisLabels(_hitmap, "SkiRoc ID", "Channel");
    // std::cout << "Created Histogram " << out2 << std::endl;

    sprintf(out, "%s %i Raw Hitmap X-Projection", _sensor.c_str(), _id);
    sprintf(out2, "h_hitXmap_%s_%i", _sensor.c_str(), _id);
    _hitXmap = new TH1I(out2, out, _maxX + 1, 0, _maxX);
    SetHistoAxisLabelx(_hitXmap, "SkiRoc ID");

    sprintf(out, "%s %i Raw Hitmap Y-Projection", _sensor.c_str(), _id);
    sprintf(out2, "h_hitYmap_%s_%i", _sensor.c_str(), _id);
    _hitYmap = new TH1I(out2, out, _maxY + 1, 0, _maxY);
    SetHistoAxisLabelx(_hitYmap, "Channel ID");

    sprintf(out, "%s %i Cluster Hitmap", _sensor.c_str(), _id);
    sprintf(out2, "h_clustermap_%s_%i", _sensor.c_str(), _id);
    _clusterMap = new TH2I(out2, out, _maxX + 1, 0, _maxX, _maxY + 1, 0, _maxY);
    SetHistoAxisLabels(_clusterMap, "X", "Y");
    // std::cout << "Created Histogram " << out2 << std::endl;

    sprintf(out, "%s %i hot Pixel Map", _sensor.c_str(), _id);
    sprintf(out2, "h_hotpixelmap_%s_%i", _sensor.c_str(), _id);
    _HotPixelMap =
        new TH2D(out2, out, _maxX + 1, 0, _maxX, _maxY + 1, 0, _maxY);
    SetHistoAxisLabels(_HotPixelMap, "X", "Y");

    sprintf(out, "%s %i LVL1 Pixel Distribution", _sensor.c_str(), _id);
    sprintf(out2, "h_lvl1_%s_%i", _sensor.c_str(), _id);
    _lvl1Distr = new TH1I(out2, out, 16, 0, 16);

    sprintf(out, "%s %i LVL1 Cluster Distribution", _sensor.c_str(), _id);
    sprintf(out2, "h_lvl1cluster_%s_%i", _sensor.c_str(), _id);
    _lvl1Cluster = new TH1I(out2, out, 16, 0, 16);

    sprintf(out, "%s %i LVL1 Clusterwidth", _sensor.c_str(), _id);
    sprintf(out2, "h_lvl1width_%s_%i", _sensor.c_str(), _id);
    _lvl1Width = new TH1I(out2, out, 16, 0, 16);

    sprintf(out, "%s %i TOT Single Pixels", _sensor.c_str(), _id);
    sprintf(out2, "h_totsingle_%s_%i", _sensor.c_str(), _id);
    if (p.is_USBPIXI4) {
      _totSingle = new TH1I(out2, out, 16, 0, 15);
    } else if (p.is_DEPFET) {
      _totSingle = new TH1I(out2, out, 255, -127, 127);
    } else {
      _totSingle = new TH1I(out2, out, 256, 0, 255);
#ifdef EUDAQ_LIB_ROOT6
      _totSingle->SetCanExtend(TH1::kAllAxes);
#else
      _totSingle->SetBit(TH1::kCanRebin);
#endif
    }

    sprintf(out, "%s %i TOT Clusters", _sensor.c_str(), _id);
    sprintf(out2, "h_totcluster_%s_%i", _sensor.c_str(), _id);
    if (p.is_USBPIXI4)
      _totCluster = new TH1I(out2, out, 80, 0, 79);
    else
      _totCluster = new TH1I(out2, out, 256, 0, 255);

    sprintf(out, "%s %i Hitoccupancy", _sensor.c_str(), _id);
    sprintf(out2, "h_hitocc%s_%i", _sensor.c_str(), _id);

    _hitOcc = new TH1F(out2, out, 250, 0.01, 1);
    SetHistoAxisLabelx(_hitOcc, "Frequency");

    sprintf(out, "%s %i Clustersize", _sensor.c_str(), _id);
    sprintf(out2, "h_clustersize_%s_%i", _sensor.c_str(), _id);
    _clusterSize = new TH1I(out2, out, 10, 0, 10);
    SetHistoAxisLabelx(_clusterSize, "Cluster Size");

    sprintf(out, "%s %i Number of Hits", _sensor.c_str(), _id);
    sprintf(out2, "h_raw_nHits_%s_%i", _sensor.c_str(), _id);
    _nHits = new TH1I(out2, out, 20, 0, 20);
    SetHistoAxisLabelx(_nHits, "Number of Hits above ZS");
    //_nHits->SetStats(1);

    sprintf(out, "%s %i Number of Invalid Hits", _sensor.c_str(), _id);
    sprintf(out2, "h_nbadHits_%s_%i", _sensor.c_str(), _id);
    _nbadHits = new TH1I(out2, out, 50, 0, 50);
    SetHistoAxisLabelx(_nbadHits, "n_{BadHits}");

    sprintf(out, "%s %i Number of Hot Pixels", _sensor.c_str(), _id);
    sprintf(out2, "h_nhotpixels_%s_%i", _sensor.c_str(), _id);
    _nHotPixels = new TH1I(out2, out, 50, 0, 50);
    SetHistoAxisLabelx(_nHotPixels, "n_{HotPixels}");

    sprintf(out, "%s %i Number of Clusters", _sensor.c_str(), _id);
    sprintf(out2, "h_nClusters_%s_%i", _sensor.c_str(), _id);
    _nClusters = new TH1I(out2, out, 500, 0, 99);
    SetHistoAxisLabelx(_nClusters, "n_{Clusters}");

    sprintf(out, "%s %i Clustersize in X", _sensor.c_str(), _id);
    sprintf(out2, "h_clustersizeX_%s_%i", _sensor.c_str(), _id);
    _clusterXWidth = new TH1I(out2, out, 20, 0, 19);

    sprintf(out, "%s %i Clustersize in Y", _sensor.c_str(), _id);
    sprintf(out2, "h_clustersizeY_%s_%i", _sensor.c_str(), _id);
    _clusterYWidth = new TH1I(out2, out, 20, 0, 19);

    sprintf(out, "%s %i Hitmap Sections", _sensor.c_str(), _id);
    sprintf(out2, "h_hitmapSections_%s_%i", _sensor.c_str(), _id);
    _hitmapSections = new TH1I(out2, out, mimosa26_max_section, _id, _id + 1);

    sprintf(out, "%s %i Pivot Pixel Usage", _sensor.c_str(), _id);
    sprintf(out2, "h_pivotpixel_%s_%i", _sensor.c_str(), _id);
    _nPivotPixel = new TH1I(out2, out, 930, 0, 9300);

    for (unsigned int section = 0; section < mimosa26_max_section; section++) {
      sprintf(out, "%i%c", _id, (section + 65));
      _hitmapSections->GetXaxis()->SetBinLabel(section + 1, out);
    }

    _nHits_section = new TH1I *[mimosa26_max_section];
    _nClusters_section = new TH1I *[mimosa26_max_section];
    _nClustersize_section = new TH1I *[mimosa26_max_section];
    _nHotPixels_section = new TH1I *[mimosa26_max_section];

    for (unsigned int section = 0; section < mimosa26_max_section; section++) {
      sprintf(out, "%s %i Number of Hits in Section %i ", _sensor.c_str(), _id,
              section);
      sprintf(out2, "h_hits_section%i_%s_%i", section, _sensor.c_str(), _id);
      _nHits_section[section] = new TH1I(out2, out, 50, 0, 50);
      if (_nHits_section[section] == NULL) {
        cout << "Error allocating Histogram" << out << endl;
        exit(-1);
      } else {
        _nHits_section[section]->GetXaxis()->SetTitle("Hits");
      }

      sprintf(out, "%s %i Number of Clusters in Section %i ", _sensor.c_str(),
              _id, section);
      sprintf(out2, "h_clusters_section%i_%s_%i", section, _sensor.c_str(),
              _id);
      _nClusters_section[section] = new TH1I(out2, out, 50, 0, 50);
      if (_nClusters_section[section] == NULL) {
        cout << "Error allocating Histogram" << out << endl;
        exit(-1);
      } else {
        _nClusters_section[section]->GetXaxis()->SetTitle("Clusters");
      }

      sprintf(out, "%s %i Cluster size in Section %i ", _sensor.c_str(), _id,
              section);
      sprintf(out2, "h_clustersize_section%i_%s_%i", section, _sensor.c_str(),
              _id);
      _nClustersize_section[section] = new TH1I(out2, out, 10, 0, 10);
      if (_nClustersize_section[section] == NULL) {
        cout << "Error allocating Histogram" << out << endl;
        exit(-1);
      } else {
        _nClustersize_section[section]->GetXaxis()->SetTitle("Cluster Size");
      }

      sprintf(out, "%s %i Number of Hot Pixels in Section %i ", _sensor.c_str(),
              _id, section);
      sprintf(out2, "h_hotpixels_section%i_%s_%i", section, _sensor.c_str(),
              _id);
      _nHotPixels_section[section] = new TH1I(out2, out, 50, 0, 50);
      if (_nHotPixels_section[section] == NULL) {
        cout << "Error allocating Histogram" << out << endl;
        exit(-1);
      } else {
        _nHotPixels_section[section]->GetXaxis()->SetTitle("Hot Pixels");
      }
    }
    // make a plane array for calculating e..g hotpixels and occupancy

    plane_map_array = new int *[_maxX];
    if (plane_map_array != NULL) {
      for (int j = 0; j < _maxX; j++) {
        plane_map_array[j] = new int[_maxY];
        if (plane_map_array[j] == NULL) {
          cout << "HitmapHistos :Error in memory allocation" << endl;
          exit(-1);
        }
      }
      zero_plane_array();
    }

  } else {
    std::cout << "No max sensorsize known!" << std::endl;
  }
  
  Set_SkiToHexaboard_ChannelMap();
}

int HitmapHistos::zero_plane_array() {
  for (int i = 0; i < _maxX; i++) {
    for (int j = 0; j < _maxY; j++) {
      plane_map_array[i][j] = 0;
    }
  }
  return 0;
}

void HitmapHistos::Fill(const SimpleStandardHit &hit) {
  int pixel_x = hit.getX();
  int pixel_y = hit.getY();

  bool pixelIsHot = false;
  if (_HotPixelMap->GetBinContent(pixel_x + 1, pixel_y + 1) >
      _mon->mon_configdata.getHotpixelcut())
    pixelIsHot = true;

  if (_hexagons_occ_adc != NULL && _hexagons_occ_tot != NULL && _hexagons_occ_toa != NULL && !pixelIsHot) {
    int ch  = _ski_to_ch_map.find(make_pair(pixel_x,pixel_y))->second;
    
    if (ch < 0 || ch > 127){
      std::cout<<" There is a problem with channel number\n pixel_x = "
	       <<pixel_x <<"  pixel_y="<<pixel_y<<"  channel = "<<ch<<std::endl;      
      ch=999;
    }
    
    else {
      //if(bin<-1 || bin > 133){      
      //std::cout<<" There is a problem with bin number\n pixel_x = "
      //	 <<" pixel_x = "<<pixel_x <<"  pixel_y="<<pixel_y<<"  channel = "<<ch<<"  bin="<<bin<<std::endl;

      // Loop over the bins and Fill the one matched to our channel 
      for (int icell = 0; icell < 133 ; icell++) {
	int bin = ch_to_bin_map[icell];
	if (bin == ch){
	  //std::cout<<" pixel_x = "<<pixel_x <<"  pixel_y="<<pixel_y<<"  channel = "<<ch<<"  icell="<<icell<<std::endl;
	  
	  char buffer_bin[3]; sprintf(buffer_bin,"%d", (char)(icell+1));
	  string bin_name = "Sensor_"+string(buffer_bin);

	  if (hit.getAMP() > 100)
	    _hexagons_occ_adc->Fill(bin_name.c_str(), 1);


	  if (hit.getTOT()!=4)
	    _hexagons_occ_tot->Fill(bin_name.c_str(), 1);
	  
	  if (hit.getLVL1()!=4)
	    _hexagons_occ_toa->Fill(bin_name.c_str(), 1);
	  

	}
      }
    }
  }

  if (_hitmap != NULL && !pixelIsHot)
    _hitmap->Fill(pixel_x, pixel_y);
  if (_hitXmap != NULL && !pixelIsHot)
    _hitXmap->Fill(pixel_x);
  if (_hitYmap != NULL && !pixelIsHot)
    _hitYmap->Fill(pixel_y);
  if ((is_MIMOSA26) && (_hitmapSections != NULL) && (!pixelIsHot))
  //&& _hitOcc->GetEntries()>0) // only fill histogram when occupancies and
  //hotpixels have been determined
  {
    char sectionid[3];
    sprintf(sectionid, "%i%c", _id,
            (pixel_x / _mon->mon_configdata.getMimosa26_section_boundary()) +
                65); // determine section label
    _hitmapSections->Fill(sectionid,
                          1); // add one hit to the corresponding section bin
    // tab[pixel_x/_mon->mon_configdata.getMimosa26_section_boundary()]++;
  }
  /*else
    {
    bool MIMOSA, PIXEL, HITMAP;
    MIMOSA = true;
    PIXEL = true;
    HITMAP = true;

    if (!is_MIMOSA26)
    cout << "MIMOSA IS FALSE" << endl;
    if (_hitmapSections == NULL)
    cout << "_hitmapSections IS NULL" << endl;
    if (pixelIsHot)
    cout << "pixelIsHot IS TRUE" << endl;

    }*/

  if ((pixel_x < _maxX) && (pixel_y < _maxY)) {
    plane_map_array[pixel_x][pixel_y] = plane_map_array[pixel_x][pixel_y] + 1;
  }
  // if (_sensor == std::string("APIX")) {
  // if (_sensor == std::string("APIX") || _sensor == std::string("USBPIX") ||
  // _sensor == std::string("USBPIXI4") ) {
  if ((is_APIX) || (is_USBPIX) || (is_USBPIXI4) || is_DEPFET) {
    if (_totSingle != NULL)
      _totSingle->Fill(hit.getTOT());
    if (_lvl1Distr != NULL)
      _lvl1Distr->Fill(hit.getLVL1());
  }
}

void HitmapHistos::Fill(const SimpleStandardPlane &plane) {
  // std::cout<< "FILL with a plane." << std::endl;
  if (_nHits != NULL)
    _nHits->Fill(plane.getNHits());
  if ((_nbadHits != NULL) && (plane.getNBadHits() > 0)) {
    _nbadHits->Fill(plane.getNBadHits());
  }
  if (_nClusters != NULL)
    _nClusters->Fill(plane.getNClusters());


  
  if (is_HEXABOARD){

    /*
    if (_hexagons_charge != NULL  && plane.getNHits()>1) {
      for (int icell = 0; icell < 133 ; icell++) {
	_hexagons_charge->SetBinContent(icell+1, 0); 
	
	const int bin = ch_to_bin_map[icell];
	
	for (int hitpix = 0; hitpix < plane.getNHits(); hitpix++) {
	  const SimpleStandardHit &hit = plane.getHit(hitpix);
	  
	  const int pixel_x = hit.getX();
	  const int pixel_y = hit.getY();
	  const int ch  = _ski_to_ch_map.find(make_pair(pixel_x,pixel_y))->second;
	  
	  if (bin == ch)
	    _hexagons_charge->SetBinContent(icell+1, hit.getAMP()); 
	}
      }
    }
    */
  }
  
  // we fill the information for the individual mimosa sections, and do a
  // zero-suppression,in case not all sections have hits/clusters
  if (is_MIMOSA26) {
    _nPivotPixel->Fill(plane.getPivotPixel());
    for (unsigned int section = 0; section < mimosa26_max_section; section++) {
      if (_nHits_section[section] != NULL) {
        if (plane.getNSectionHits(section) > 0) {
          _nHits_section[section]->Fill(plane.getNSectionHits(section));
          // std::cout<< "Section " << section << " filling with " <<
          // plane.getNSectionHits(section) << std::endl;
        }
      }
      if (_nClusters_section[section] != NULL) {
        if (plane.getNSectionClusters(section) > 0) {
          _nClusters_section[section]->Fill(plane.getNSectionClusters(section));
        }
      }
    }
  }
}

void HitmapHistos::Fill(const SimpleStandardCluster &cluster) {
  if (_clusterMap != NULL)
    _clusterMap->Fill(cluster.getX(), cluster.getY());
  if (_clusterSize != NULL)
    _clusterSize->Fill(cluster.getNPixel());
  if (is_MIMOSA26) {
    unsigned int nsection =
        cluster.getX() /
        _mon->mon_configdata.getMimosa26_section_boundary(); // get to which
                                                             // section in
                                                             // Mimosa the
                                                             // cluster belongs
    if ((nsection < mimosa26_max_section) &&
        (_nClustersize_section[nsection] != NULL)) // check if valid address
    {
      if (cluster.getNPixel() > 0) {
        _nClustersize_section[nsection]->Fill(cluster.getNPixel());
      }
    }
  }

  if ((is_APIX) || (is_USBPIX) || (is_USBPIXI4)) {
    if (_lvl1Width != NULL)
      _lvl1Width->Fill(cluster.getLVL1Width());
    if (_totCluster != NULL)
      _totCluster->Fill(cluster.getTOT());
    if (_lvl1Cluster != NULL)
      _lvl1Cluster->Fill(cluster.getFirstLVL1());
    if (_clusterXWidth != NULL)
      _clusterXWidth->Fill(cluster.getWidthX());
    if (_clusterYWidth != NULL)
      _clusterYWidth->Fill(cluster.getWidthY());
  }
}

void HitmapHistos::Reset() {
  _hexagons_occ_adc->Reset("");
  _hexagons_charge->Reset("");
  _hexagons_occ_tot->Reset("");
  _hexagons_occ_toa->Reset("");
  _hitmap->Reset();
  _hitXmap->Reset();
  _hitYmap->Reset();
  _totSingle->Reset();
  _lvl1Distr->Reset();
  _clusterMap->Reset();
  _totCluster->Reset();
  _lvl1Cluster->Reset();
  _lvl1Width->Reset();
  _hitOcc->Reset();
  _clusterSize->Reset();
  _nClusters->Reset();
  _nHits->Reset();
  _nbadHits->Reset();
  _nHotPixels->Reset();
  _HotPixelMap->Reset();
  _clusterYWidth->Reset();
  _clusterXWidth->Reset();
  _hitmapSections->Reset();
  _nPivotPixel->Reset();
  for (unsigned int section = 0; section < mimosa26_max_section; section++) {
    _nClusters_section[section]->Reset();
    _nHits_section[section]->Reset();
    _nClustersize_section[section]->Reset();
    _nHotPixels_section[section]->Reset();
  }
  // we have to reset the aux array as well
  zero_plane_array();
}

void HitmapHistos::Calculate(const int currentEventNum) {
  _wait = true;
  _hitOcc->SetBins(currentEventNum / 10, 0, 1);
  _hitOcc->Reset();

  int nHotpixels = 0;
  std::vector<unsigned int> nHotpixels_section;
  double Hotpixelcut = _mon->mon_configdata.getHotpixelcut();
  double bin = 0;
  double occupancy = 0;
  if (is_MIMOSA26) // probalbly initialize vector
  {
    nHotpixels_section.reserve(_mon->mon_configdata.getMimosa26_max_sections());
    for (unsigned int i = 0;
         i < _mon->mon_configdata.getMimosa26_max_sections(); i++) {
      nHotpixels_section[i] = 0;
    }
  }

  for (int x = 0; x < _maxX; ++x) {
    for (int y = 0; y < _maxY; ++y) {

      bin = plane_map_array[x][y];

      if (bin != 0) {
        occupancy =
            bin /
            (double)currentEventNum; // FIXME it's not occupancy, it's frequency
        _hitOcc->Fill(occupancy);
        // only count as hotpixel if occupancy larger than minimal occupancy for
        // a single hit
        if (occupancy > Hotpixelcut && ((1. / (double)(currentEventNum)) <
                                        _mon->mon_configdata.getHotpixelcut()))
        // if (occupancy>Hotpixelcut && )
        {
          nHotpixels++;
          _HotPixelMap->SetBinContent(x + 1, y + 1,
                                      occupancy); // ROOT start from 1
          if (is_MIMOSA26) {
            nHotpixels_section
                [x / _mon->mon_configdata.getMimosa26_section_boundary()]++;
          }
        }
      }
    }
  }
  if (nHotpixels > 0) {
    _nHotPixels->Fill(nHotpixels);
    if (is_MIMOSA26) {
      for (unsigned int section = 0;
           section < _mon->mon_configdata.getMimosa26_max_sections();
           section++) {
        if ((nHotpixels_section[section] > 0)) {
          _nHotPixels_section[section]->Fill(nHotpixels_section[section]);
          // cout<<"nHotPixels is being filled for plane " << _id << " with " <<
          // ++counter_nhotpixels_being_filled << " time in section " <<
          //      section << endl;
        }
      }
    }
  }

  _wait = false;
}

void HitmapHistos::Write() {
  _hexagons_occ_adc->Write();
  _hexagons_occ_tot->Write();
  _hexagons_occ_toa->Write();
  _hexagons_charge->Write();
  _hitmap->Write();
  _hitXmap->Write();
  _hitYmap->Write();
  //_totSingle->Write();
  //_lvl1Distr->Write();
  //_clusterMap->Write();
  //_totCluster->Write();
  //_lvl1Cluster->Write();
  //_lvl1Width->Write();
  //_hitOcc->Write();
  //_clusterSize->Write();
  //_nClusters->Write();
  _nHits->Write();
  //_nbadHits->Write();
  //_HotPixelMap->Write();
  //_nHotPixels->Write();
  //_clusterXWidth->Write();
  //_clusterYWidth->Write();
  //_hitmapSections->Write();
  //_nPivotPixel->Write();

  std::cout<<"Doing HitmapHistos::Write() before canvas drawing"<<std::endl;

  /*
  gSystem->Sleep(100);
  
  gROOT->SetBatch(kTRUE);
  TCanvas *tmpcan = new TCanvas("tmpcan","Canvas for PNGs",600,600);
  tmpcan->cd();
  
  _hexagons_occ_adc->Draw("COLZ TEXT");
  tmpcan->SaveAs("../snapshots/Occupancy_ADC_HG.png");

  _hexagons_occ_tot->Draw("COLZ TEXT");
  tmpcan->SaveAs("../snapshots/Occupancy_TOT.png");
   
  _hexagons_occ_toa->Draw("COLZ TEXT");
  tmpcan->SaveAs("../snapshots/Occupancy_TOA.png");

  _nHits->Draw("hist");
  tmpcan->SaveAs("../snapshots/nHits.png");

  tmpcan->Close();
  gROOT->SetBatch(kFALSE); 

  */
  
  std::cout<<"Doing HitmapHistos::Write() after canvas drawing"<<std::endl;

}

int HitmapHistos::SetHistoAxisLabelx(TH1 *histo, string xlabel) {
  if (histo != NULL) {
    histo->GetXaxis()->SetTitle(xlabel.c_str());
  }
  return 0;
}

int HitmapHistos::SetHistoAxisLabels(TH1 *histo, string xlabel, string ylabel) {
  SetHistoAxisLabelx(histo, xlabel);
  SetHistoAxisLabely(histo, ylabel);

  return 0;
}

int HitmapHistos::SetHistoAxisLabely(TH1 *histo, string ylabel) {
  if (histo != NULL) {
    histo->GetYaxis()->SetTitle(ylabel.c_str());
  }
  return 0;
}

void HitmapHistos::Set_SkiToHexaboard_ChannelMap(){

  for(int c=0; c<127; c++){
    int row = c*3;
    _ski_to_ch_map.insert(make_pair(make_pair(sc_to_ch_map[row+1],sc_to_ch_map[row+2]),sc_to_ch_map[row]));
  }
}

TH2Poly* HitmapHistos::get_th2poly(string name, string title) {
   //hitmapoly = new TH2Poly(out2, out, _maxX + 1, 0, _maxX, _maxY + 1, 0, _maxY);
   TH2Poly* hitmapoly = new TH2Poly(name.c_str(), title.c_str(), 25, -7.14598, 7.14598, 25, -6.1886, 6.188);
   SetHistoAxisLabels(hitmapoly, "X [cm]", "Y [cm]");
   Double_t Graph_fx1[4] = {
   6.171528,
   6.496345,
   7.14598,
   6.496345};
   Double_t Graph_fy1[4] = {
   0.5626,
   1.166027e-08,
   1.282629e-08,
   1.1252};
   TGraph *graph = new TGraph(4,Graph_fx1,Graph_fy1);
   graph->SetName("Sensor_1");
   graph->SetTitle("Sensor_1");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx2[4] = {
   6.171528,
   6.496345,
   7.14598,
   6.496345};
   Double_t Graph_fy2[4] = {
   -0.5626,
   -1.1252,
   1.282629e-08,
   1.166027e-08};
   graph = new TGraph(4,Graph_fx2,Graph_fy2);
   graph->SetName("Sensor_2");
   graph->SetTitle("Sensor_2");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx3[4] = {
   5.197076,
   5.521893,
   6.171528,
   5.521893};
   Double_t Graph_fy3[4] = {
   2.2504,
   1.6878,
   1.6878,
   2.813};
   graph = new TGraph(4,Graph_fx3,Graph_fy3);
   graph->SetName("Sensor_3");
   graph->SetTitle("Sensor_3");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx4[6] = {
   5.197076,
   5.521893,
   6.171528,
   6.496345,
   6.171528,
   5.521893};
   Double_t Graph_fy4[6] = {
   1.1252,
   0.5626,
   0.5626,
   1.1252,
   1.6878,
   1.6878};
   graph = new TGraph(6,Graph_fx4,Graph_fy4);
   graph->SetName("Sensor_4");
   graph->SetTitle("Sensor_4");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx5[6] = {
   5.197076,
   5.521893,
   6.171528,
   6.496345,
   6.171528,
   5.521893};
   Double_t Graph_fy5[6] = {
   9.328214e-09,
   -0.5626,
   -0.5626,
   1.166027e-08,
   0.5626,
   0.5626};
   graph = new TGraph(6,Graph_fx5,Graph_fy5);
   graph->SetName("Sensor_5");
   graph->SetTitle("Sensor_5");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx6[6] = {
   5.197076,
   5.521893,
   6.171528,
   6.496345,
   6.171528,
   5.521893};
   Double_t Graph_fy6[6] = {
   -1.1252,
   -1.6878,
   -1.6878,
   -1.1252,
   -0.5626,
   -0.5626};
   graph = new TGraph(6,Graph_fx6,Graph_fy6);
   graph->SetName("Sensor_6");
   graph->SetTitle("Sensor_6");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx7[4] = {
   5.197076,
   5.521893,
   6.171528,
   5.521893};
   Double_t Graph_fy7[4] = {
   -2.2504,
   -2.813,
   -1.6878,
   -1.6878};
   graph = new TGraph(4,Graph_fx7,Graph_fy7);
   graph->SetName("Sensor_7");
   graph->SetTitle("Sensor_7");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx8[4] = {
   4.222624,
   4.547442,
   5.197076,
   4.547442};
   Double_t Graph_fy8[4] = {
   3.9382,
   3.3756,
   3.3756,
   4.5008};
   graph = new TGraph(4,Graph_fx8,Graph_fy8);
   graph->SetName("Sensor_8");
   graph->SetTitle("Sensor_8");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx9[6] = {
   4.222624,
   4.547442,
   5.197076,
   5.521893,
   5.197076,
   4.547442};
   Double_t Graph_fy9[6] = {
   2.813,
   2.2504,
   2.2504,
   2.813,
   3.3756,
   3.3756};
   graph = new TGraph(6,Graph_fx9,Graph_fy9);
   graph->SetName("Sensor_9");
   graph->SetTitle("Sensor_9");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx10[6] = {
   4.222624,
   4.547442,
   5.197076,
   5.521893,
   5.197076,
   4.547442};
   Double_t Graph_fy10[6] = {
   1.6878,
   1.1252,
   1.1252,
   1.6878,
   2.2504,
   2.2504};
   graph = new TGraph(6,Graph_fx10,Graph_fy10);
   graph->SetName("Sensor_10");
   graph->SetTitle("Sensor_10");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx11[6] = {
   4.222624,
   4.547442,
   5.197076,
   5.521893,
   5.197076,
   4.547442};
   Double_t Graph_fy11[6] = {
   0.5626,
   8.162187e-09,
   9.328214e-09,
   0.5626,
   1.1252,
   1.1252};
   graph = new TGraph(6,Graph_fx11,Graph_fy11);
   graph->SetName("Sensor_11");
   graph->SetTitle("Sensor_11");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx12[6] = {
   4.222624,
   4.547441,
   5.197076,
   5.521893,
   5.197076,
   4.547442};
   Double_t Graph_fy12[6] = {
   -0.5626,
   -1.1252,
   -1.1252,
   -0.5626,
   9.328214e-09,
   8.162187e-09};
   graph = new TGraph(6,Graph_fx12,Graph_fy12);
   graph->SetName("Sensor_12");
   graph->SetTitle("Sensor_12");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx13[6] = {
   4.222624,
   4.547441,
   5.197076,
   5.521893,
   5.197076,
   4.547441};
   Double_t Graph_fy13[6] = {
   -1.6878,
   -2.2504,
   -2.2504,
   -1.6878,
   -1.1252,
   -1.1252};
   graph = new TGraph(6,Graph_fx13,Graph_fy13);
   graph->SetName("Sensor_13");
   graph->SetTitle("Sensor_13");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx14[6] = {
   4.222624,
   4.547441,
   5.197076,
   5.521893,
   5.197076,
   4.547441};
   Double_t Graph_fy14[6] = {
   -2.813,
   -3.3756,
   -3.3756,
   -2.813,
   -2.2504,
   -2.2504};
   graph = new TGraph(6,Graph_fx14,Graph_fy14);
   graph->SetName("Sensor_14");
   graph->SetTitle("Sensor_14");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx15[4] = {
   4.222624,
   4.547441,
   5.197076,
   4.547441};
   Double_t Graph_fy15[4] = {
   -3.9382,
   -4.5008,
   -3.3756,
   -3.3756};
   graph = new TGraph(4,Graph_fx15,Graph_fy15);
   graph->SetName("Sensor_15");
   graph->SetTitle("Sensor_15");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx16[4] = {
   3.248173,
   3.57299,
   4.222624,
   3.57299};
   Double_t Graph_fy16[4] = {
   5.626,
   5.0634,
   5.0634,
   6.1886};
   graph = new TGraph(4,Graph_fx16,Graph_fy16);
   graph->SetName("Sensor_16");
   graph->SetTitle("Sensor_16");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx17[6] = {
   3.248173,
   3.57299,
   4.222624,
   4.547442,
   4.222624,
   3.57299};
   Double_t Graph_fy17[6] = {
   4.5008,
   3.9382,
   3.9382,
   4.5008,
   5.0634,
   5.0634};
   graph = new TGraph(6,Graph_fx17,Graph_fy17);
   graph->SetName("Sensor_17");
   graph->SetTitle("Sensor_17");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx18[6] = {
   3.248173,
   3.57299,
   4.222624,
   4.547442,
   4.222624,
   3.57299};
   Double_t Graph_fy18[6] = {
   3.3756,
   2.813,
   2.813,
   3.3756,
   3.9382,
   3.9382};
   graph = new TGraph(6,Graph_fx18,Graph_fy18);
   graph->SetName("Sensor_18");
   graph->SetTitle("Sensor_18");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx19[6] = {
   3.248173,
   3.57299,
   4.222624,
   4.547442,
   4.222624,
   3.57299};
   Double_t Graph_fy19[6] = {
   2.2504,
   1.6878,
   1.6878,
   2.2504,
   2.813,
   2.813};
   graph = new TGraph(6,Graph_fx19,Graph_fy19);
   graph->SetName("Sensor_19");
   graph->SetTitle("Sensor_19");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx20[6] = {
   3.248173,
   3.57299,
   4.222624,
   4.547442,
   4.222624,
   3.57299};
   Double_t Graph_fy20[6] = {
   1.1252,
   0.5626,
   0.5626,
   1.1252,
   1.6878,
   1.6878};
   graph = new TGraph(6,Graph_fx20,Graph_fy20);
   graph->SetName("Sensor_20");
   graph->SetTitle("Sensor_20");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx21[6] = {
   3.248173,
   3.57299,
   4.222624,
   4.547442,
   4.222624,
   3.57299};
   Double_t Graph_fy21[6] = {
   5.830134e-09,
   -0.5626,
   -0.5626,
   8.162187e-09,
   0.5626,
   0.5626};
   graph = new TGraph(6,Graph_fx21,Graph_fy21);
   graph->SetName("Sensor_21");
   graph->SetTitle("Sensor_21");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx22[6] = {
   3.248172,
   3.57299,
   4.222624,
   4.547441,
   4.222624,
   3.57299};
   Double_t Graph_fy22[6] = {
   -1.1252,
   -1.6878,
   -1.6878,
   -1.1252,
   -0.5626,
   -0.5626};
   graph = new TGraph(6,Graph_fx22,Graph_fy22);
   graph->SetName("Sensor_22");
   graph->SetTitle("Sensor_22");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx23[6] = {
   3.248172,
   3.57299,
   4.222624,
   4.547441,
   4.222624,
   3.57299};
   Double_t Graph_fy23[6] = {
   -2.2504,
   -2.813,
   -2.813,
   -2.2504,
   -1.6878,
   -1.6878};
   graph = new TGraph(6,Graph_fx23,Graph_fy23);
   graph->SetName("Sensor_23");
   graph->SetTitle("Sensor_23");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx24[6] = {
   3.248172,
   3.57299,
   4.222624,
   4.547441,
   4.222624,
   3.57299};
   Double_t Graph_fy24[6] = {
   -3.3756,
   -3.9382,
   -3.9382,
   -3.3756,
   -2.813,
   -2.813};
   graph = new TGraph(6,Graph_fx24,Graph_fy24);
   graph->SetName("Sensor_24");
   graph->SetTitle("Sensor_24");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx25[6] = {
   3.248172,
   3.57299,
   4.222624,
   4.547441,
   4.222624,
   3.57299};
   Double_t Graph_fy25[6] = {
   -4.5008,
   -5.0634,
   -5.0634,
   -4.5008,
   -3.9382,
   -3.9382};
   graph = new TGraph(6,Graph_fx25,Graph_fy25);
   graph->SetName("Sensor_25");
   graph->SetTitle("Sensor_25");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx26[4] = {
   3.248172,
   3.57299,
   4.222624,
   3.57299};
   Double_t Graph_fy26[4] = {
   -5.626,
   -6.1886,
   -5.0634,
   -5.0634};
   graph = new TGraph(4,Graph_fx26,Graph_fy26);
   graph->SetName("Sensor_26");
   graph->SetTitle("Sensor_26");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx27[4] = {
   2.273721,
   2.598538,
   3.248173,
   3.57299};
   Double_t Graph_fy27[4] = {
   6.1886,
   5.626,
   5.626,
   6.1886};
   graph = new TGraph(4,Graph_fx27,Graph_fy27);
   graph->SetName("Sensor_27");
   graph->SetTitle("Sensor_27");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx28[6] = {
   2.273721,
   2.598538,
   3.248173,
   3.57299,
   3.248173,
   2.598538};
   Double_t Graph_fy28[6] = {
   5.0634,
   4.5008,
   4.5008,
   5.0634,
   5.626,
   5.626};
   graph = new TGraph(6,Graph_fx28,Graph_fy28);
   graph->SetName("Sensor_28");
   graph->SetTitle("Sensor_28");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx29[6] = {
   2.273721,
   2.598538,
   3.248173,
   3.57299,
   3.248173,
   2.598538};
   Double_t Graph_fy29[6] = {
   3.9382,
   3.3756,
   3.3756,
   3.9382,
   4.5008,
   4.5008};
   graph = new TGraph(6,Graph_fx29,Graph_fy29);
   graph->SetName("Sensor_29");
   graph->SetTitle("Sensor_29");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx30[6] = {
   2.273721,
   2.598538,
   3.248173,
   3.57299,
   3.248173,
   2.598538};
   Double_t Graph_fy30[6] = {
   2.813,
   2.2504,
   2.2504,
   2.813,
   3.3756,
   3.3756};
   graph = new TGraph(6,Graph_fx30,Graph_fy30);
   graph->SetName("Sensor_30");
   graph->SetTitle("Sensor_30");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx31[6] = {
   2.273721,
   2.598538,
   3.248173,
   3.57299,
   3.248173,
   2.598538};
   Double_t Graph_fy31[6] = {
   1.6878,
   1.1252,
   1.1252,
   1.6878,
   2.2504,
   2.2504};
   graph = new TGraph(6,Graph_fx31,Graph_fy31);
   graph->SetName("Sensor_31");
   graph->SetTitle("Sensor_31");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx32[6] = {
   2.273721,
   2.598538,
   3.248173,
   3.57299,
   3.248173,
   2.598538};
   Double_t Graph_fy32[6] = {
   0.5626,
   4.664107e-09,
   5.830134e-09,
   0.5626,
   1.1252,
   1.1252};
   graph = new TGraph(6,Graph_fx32,Graph_fy32);
   graph->SetName("Sensor_32");
   graph->SetTitle("Sensor_32");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx33[6] = {
   2.273721,
   2.598538,
   3.248172,
   3.57299,
   3.248173,
   2.598538};
   Double_t Graph_fy33[6] = {
   -0.5626,
   -1.1252,
   -1.1252,
   -0.5626,
   5.830134e-09,
   4.664107e-09};
   graph = new TGraph(6,Graph_fx33,Graph_fy33);
   graph->SetName("Sensor_33");
   graph->SetTitle("Sensor_33");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx34[6] = {
   2.273721,
   2.598538,
   3.248172,
   3.57299,
   3.248172,
   2.598538};
   Double_t Graph_fy34[6] = {
   -1.6878,
   -2.2504,
   -2.2504,
   -1.6878,
   -1.1252,
   -1.1252};
   graph = new TGraph(6,Graph_fx34,Graph_fy34);
   graph->SetName("Sensor_34");
   graph->SetTitle("Sensor_34");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx35[6] = {
   2.273721,
   2.598538,
   3.248172,
   3.57299,
   3.248172,
   2.598538};
   Double_t Graph_fy35[6] = {
   -2.813,
   -3.3756,
   -3.3756,
   -2.813,
   -2.2504,
   -2.2504};
   graph = new TGraph(6,Graph_fx35,Graph_fy35);
   graph->SetName("Sensor_35");
   graph->SetTitle("Sensor_35");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx36[6] = {
   2.273721,
   2.598538,
   3.248172,
   3.57299,
   3.248172,
   2.598538};
   Double_t Graph_fy36[6] = {
   -3.9382,
   -4.5008,
   -4.5008,
   -3.9382,
   -3.3756,
   -3.3756};
   graph = new TGraph(6,Graph_fx36,Graph_fy36);
   graph->SetName("Sensor_36");
   graph->SetTitle("Sensor_36");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx37[6] = {
   2.273721,
   2.598538,
   3.248172,
   3.57299,
   3.248172,
   2.598538};
   Double_t Graph_fy37[6] = {
   -5.0634,
   -5.626,
   -5.626,
   -5.0634,
   -4.5008,
   -4.5008};
   graph = new TGraph(6,Graph_fx37,Graph_fy37);
   graph->SetName("Sensor_37");
   graph->SetTitle("Sensor_37");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx38[4] = {
   2.273721,
   3.57299,
   3.248172,
   2.598538};
   Double_t Graph_fy38[4] = {
   -6.1886,
   -6.1886,
   -5.626,
   -5.626};
   graph = new TGraph(4,Graph_fx38,Graph_fy38);
   graph->SetName("Sensor_38");
   graph->SetTitle("Sensor_38");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx39[6] = {
   1.299269,
   1.624086,
   2.273721,
   2.598538,
   2.273721,
   1.624086};
   Double_t Graph_fy39[6] = {
   5.626,
   5.0634,
   5.0634,
   5.626,
   6.1886,
   6.1886};
   graph = new TGraph(6,Graph_fx39,Graph_fy39);
   graph->SetName("Sensor_39");
   graph->SetTitle("Sensor_39");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx40[6] = {
   1.299269,
   1.624086,
   2.273721,
   2.598538,
   2.273721,
   1.624086};
   Double_t Graph_fy40[6] = {
   4.5008,
   3.9382,
   3.9382,
   4.5008,
   5.0634,
   5.0634};
   graph = new TGraph(6,Graph_fx40,Graph_fy40);
   graph->SetName("Sensor_40");
   graph->SetTitle("Sensor_40");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx41[6] = {
   1.299269,
   1.624086,
   2.273721,
   2.598538,
   2.273721,
   1.624086};
   Double_t Graph_fy41[6] = {
   3.3756,
   2.813,
   2.813,
   3.3756,
   3.9382,
   3.9382};
   graph = new TGraph(6,Graph_fx41,Graph_fy41);
   graph->SetName("Sensor_41");
   graph->SetTitle("Sensor_41");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx42[6] = {
   1.299269,
   1.624086,
   2.273721,
   2.598538,
   2.273721,
   1.624086};
   Double_t Graph_fy42[6] = {
   2.2504,
   1.6878,
   1.6878,
   2.2504,
   2.813,
   2.813};
   graph = new TGraph(6,Graph_fx42,Graph_fy42);
   graph->SetName("Sensor_42");
   graph->SetTitle("Sensor_42");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx43[6] = {
   1.299269,
   1.624086,
   2.273721,
   2.598538,
   2.273721,
   1.624086};
   Double_t Graph_fy43[6] = {
   1.1252,
   0.5626,
   0.5626,
   1.1252,
   1.6878,
   1.6878};
   graph = new TGraph(6,Graph_fx43,Graph_fy43);
   graph->SetName("Sensor_43");
   graph->SetTitle("Sensor_43");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx44[6] = {
   1.299269,
   1.624086,
   2.273721,
   2.598538,
   2.273721,
   1.624086};
   Double_t Graph_fy44[6] = {
   2.332053e-09,
   -0.5626,
   -0.5626,
   4.664107e-09,
   0.5626,
   0.5626};
   graph = new TGraph(6,Graph_fx44,Graph_fy44);
   graph->SetName("Sensor_44");
   graph->SetTitle("Sensor_44");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx45[6] = {
   1.299269,
   1.624086,
   2.273721,
   2.598538,
   2.273721,
   1.624086};
   Double_t Graph_fy45[6] = {
   -1.1252,
   -1.6878,
   -1.6878,
   -1.1252,
   -0.5626,
   -0.5626};
   graph = new TGraph(6,Graph_fx45,Graph_fy45);
   graph->SetName("Sensor_45");
   graph->SetTitle("Sensor_45");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx46[6] = {
   1.299269,
   1.624086,
   2.273721,
   2.598538,
   2.273721,
   1.624086};
   Double_t Graph_fy46[6] = {
   -2.2504,
   -2.813,
   -2.813,
   -2.2504,
   -1.6878,
   -1.6878};
   graph = new TGraph(6,Graph_fx46,Graph_fy46);
   graph->SetName("Sensor_46");
   graph->SetTitle("Sensor_46");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx47[6] = {
   1.299269,
   1.624086,
   2.273721,
   2.598538,
   2.273721,
   1.624086};
   Double_t Graph_fy47[6] = {
   -3.3756,
   -3.9382,
   -3.9382,
   -3.3756,
   -2.813,
   -2.813};
   graph = new TGraph(6,Graph_fx47,Graph_fy47);
   graph->SetName("Sensor_47");
   graph->SetTitle("Sensor_47");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx48[6] = {
   1.299269,
   1.624086,
   2.273721,
   2.598538,
   2.273721,
   1.624086};
   Double_t Graph_fy48[6] = {
   -4.5008,
   -5.0634,
   -5.0634,
   -4.5008,
   -3.9382,
   -3.9382};
   graph = new TGraph(6,Graph_fx48,Graph_fy48);
   graph->SetName("Sensor_48");
   graph->SetTitle("Sensor_48");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx49[6] = {
   1.299269,
   1.624086,
   2.273721,
   2.598538,
   2.273721,
   1.624086};
   Double_t Graph_fy49[6] = {
   -5.626,
   -6.1886,
   -6.1886,
   -5.626,
   -5.0634,
   -5.0634};
   graph = new TGraph(6,Graph_fx49,Graph_fy49);
   graph->SetName("Sensor_49");
   graph->SetTitle("Sensor_49");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx50[4] = {
   0.3248173,
   0.6496345,
   1.299269,
   1.624086};
   Double_t Graph_fy50[4] = {
   6.1886,
   5.626,
   5.626,
   6.1886};
   graph = new TGraph(4,Graph_fx50,Graph_fy50);
   graph->SetName("Sensor_50");
   graph->SetTitle("Sensor_50");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx51[6] = {
   0.3248173,
   0.6496345,
   1.299269,
   1.624086,
   1.299269,
   0.6496345};
   Double_t Graph_fy51[6] = {
   5.0634,
   4.5008,
   4.5008,
   5.0634,
   5.626,
   5.626};
   graph = new TGraph(6,Graph_fx51,Graph_fy51);
   graph->SetName("Sensor_51");
   graph->SetTitle("Sensor_51");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx52[6] = {
   0.3248173,
   0.6496345,
   1.299269,
   1.624086,
   1.299269,
   0.6496345};
   Double_t Graph_fy52[6] = {
   3.9382,
   3.3756,
   3.3756,
   3.9382,
   4.5008,
   4.5008};
   graph = new TGraph(6,Graph_fx52,Graph_fy52);
   graph->SetName("Sensor_52");
   graph->SetTitle("Sensor_52");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx53[6] = {
   0.3248173,
   0.6496345,
   1.299269,
   1.624086,
   1.299269,
   0.6496345};
   Double_t Graph_fy53[6] = {
   2.813,
   2.2504,
   2.2504,
   2.813,
   3.3756,
   3.3756};
   graph = new TGraph(6,Graph_fx53,Graph_fy53);
   graph->SetName("Sensor_53");
   graph->SetTitle("Sensor_53");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx54[6] = {
   0.3248173,
   0.6496345,
   1.299269,
   1.624086,
   1.299269,
   0.6496345};
   Double_t Graph_fy54[6] = {
   1.6878,
   1.1252,
   1.1252,
   1.6878,
   2.2504,
   2.2504};
   graph = new TGraph(6,Graph_fx54,Graph_fy54);
   graph->SetName("Sensor_54");
   graph->SetTitle("Sensor_54");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx55[6] = {
   0.3248173,
   0.6496345,
   1.299269,
   1.624086,
   1.299269,
   0.6496345};
   Double_t Graph_fy55[6] = {
   0.5626,
   1.166027e-09,
   2.332053e-09,
   0.5626,
   1.1252,
   1.1252};
   graph = new TGraph(6,Graph_fx55,Graph_fy55);
   graph->SetName("Sensor_55");
   graph->SetTitle("Sensor_55");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx56[6] = {
   0.3248172,
   0.6496345,
   1.299269,
   1.624086,
   1.299269,
   0.6496345};
   Double_t Graph_fy56[6] = {
   -0.5626,
   -1.1252,
   -1.1252,
   -0.5626,
   2.332053e-09,
   1.166027e-09};
   graph = new TGraph(6,Graph_fx56,Graph_fy56);
   graph->SetName("Sensor_56");
   graph->SetTitle("Sensor_56");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx57[6] = {
   0.3248172,
   0.6496345,
   1.299269,
   1.624086,
   1.299269,
   0.6496345};
   Double_t Graph_fy57[6] = {
   -1.6878,
   -2.2504,
   -2.2504,
   -1.6878,
   -1.1252,
   -1.1252};
   graph = new TGraph(6,Graph_fx57,Graph_fy57);
   graph->SetName("Sensor_57");
   graph->SetTitle("Sensor_57");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx58[6] = {
   0.3248172,
   0.6496345,
   1.299269,
   1.624086,
   1.299269,
   0.6496345};
   Double_t Graph_fy58[6] = {
   -2.813,
   -3.3756,
   -3.3756,
   -2.813,
   -2.2504,
   -2.2504};
   graph = new TGraph(6,Graph_fx58,Graph_fy58);
   graph->SetName("Sensor_58");
   graph->SetTitle("Sensor_58");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx59[6] = {
   0.3248172,
   0.6496345,
   1.299269,
   1.624086,
   1.299269,
   0.6496345};
   Double_t Graph_fy59[6] = {
   -3.9382,
   -4.5008,
   -4.5008,
   -3.9382,
   -3.3756,
   -3.3756};
   graph = new TGraph(6,Graph_fx59,Graph_fy59);
   graph->SetName("Sensor_59");
   graph->SetTitle("Sensor_59");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx60[6] = {
   0.3248172,
   0.6496345,
   1.299269,
   1.624086,
   1.299269,
   0.6496345};
   Double_t Graph_fy60[6] = {
   -5.0634,
   -5.626,
   -5.626,
   -5.0634,
   -4.5008,
   -4.5008};
   graph = new TGraph(6,Graph_fx60,Graph_fy60);
   graph->SetName("Sensor_60");
   graph->SetTitle("Sensor_60");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx61[4] = {
   0.3248172,
   1.624086,
   1.299269,
   0.6496345};
   Double_t Graph_fy61[4] = {
   -6.1886,
   -6.1886,
   -5.626,
   -5.626};
   graph = new TGraph(4,Graph_fx61,Graph_fy61);
   graph->SetName("Sensor_61");
   graph->SetTitle("Sensor_61");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx62[6] = {
   -0.6496345,
   -0.3248172,
   0.3248173,
   0.6496345,
   0.3248173,
   -0.3248172};
   Double_t Graph_fy62[6] = {
   5.626,
   5.0634,
   5.0634,
   5.626,
   6.1886,
   6.1886};
   graph = new TGraph(6,Graph_fx62,Graph_fy62);
   graph->SetName("Sensor_62");
   graph->SetTitle("Sensor_62");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx63[6] = {
   -0.6496345,
   -0.3248172,
   0.3248173,
   0.6496345,
   0.3248173,
   -0.3248172};
   Double_t Graph_fy63[6] = {
   4.5008,
   3.9382,
   3.9382,
   4.5008,
   5.0634,
   5.0634};
   graph = new TGraph(6,Graph_fx63,Graph_fy63);
   graph->SetName("Sensor_63");
   graph->SetTitle("Sensor_63");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx64[6] = {
   -0.6496345,
   -0.3248172,
   0.3248173,
   0.6496345,
   0.3248173,
   -0.3248172};
   Double_t Graph_fy64[6] = {
   3.3756,
   2.813,
   2.813,
   3.3756,
   3.9382,
   3.9382};
   graph = new TGraph(6,Graph_fx64,Graph_fy64);
   graph->SetName("Sensor_64");
   graph->SetTitle("Sensor_64");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx65[6] = {
   -0.6496345,
   -0.3248172,
   0.3248173,
   0.6496345,
   0.3248173,
   -0.3248172};
   Double_t Graph_fy65[6] = {
   2.2504,
   1.6878,
   1.6878,
   2.2504,
   2.813,
   2.813};
   graph = new TGraph(6,Graph_fx65,Graph_fy65);
   graph->SetName("Sensor_65");
   graph->SetTitle("Sensor_65");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx66[6] = {
   -0.6496345,
   -0.3248172,
   0.3248173,
   0.6496345,
   0.3248173,
   -0.3248172};
   Double_t Graph_fy66[6] = {
   1.1252,
   0.5626,
   0.5626,
   1.1252,
   1.6878,
   1.6878};
   graph = new TGraph(6,Graph_fx66,Graph_fy66);
   graph->SetName("Sensor_66");
   graph->SetTitle("Sensor_66");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx67[6] = {
   -0.6496345,
   -0.3248173,
   0.3248172,
   0.6496345,
   0.3248173,
   -0.3248172};
   Double_t Graph_fy67[6] = {
   -1.166027e-09,
   -0.5626,
   -0.5626,
   1.166027e-09,
   0.5626,
   0.5626};
   graph = new TGraph(6,Graph_fx67,Graph_fy67);
   graph->SetName("Sensor_67");
   graph->SetTitle("Sensor_67");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx68[6] = {
   -0.6496345,
   -0.3248173,
   0.3248172,
   0.6496345,
   0.3248172,
   -0.3248173};
   Double_t Graph_fy68[6] = {
   -1.1252,
   -1.6878,
   -1.6878,
   -1.1252,
   -0.5626,
   -0.5626};
   graph = new TGraph(6,Graph_fx68,Graph_fy68);
   graph->SetName("Sensor_68");
   graph->SetTitle("Sensor_68");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx69[6] = {
   -0.6496345,
   -0.3248173,
   0.3248172,
   0.6496345,
   0.3248172,
   -0.3248173};
   Double_t Graph_fy69[6] = {
   -2.2504,
   -2.813,
   -2.813,
   -2.2504,
   -1.6878,
   -1.6878};
   graph = new TGraph(6,Graph_fx69,Graph_fy69);
   graph->SetName("Sensor_69");
   graph->SetTitle("Sensor_69");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx70[6] = {
   -0.6496345,
   -0.3248173,
   0.3248172,
   0.6496345,
   0.3248172,
   -0.3248173};
   Double_t Graph_fy70[6] = {
   -3.3756,
   -3.9382,
   -3.9382,
   -3.3756,
   -2.813,
   -2.813};
   graph = new TGraph(6,Graph_fx70,Graph_fy70);
   graph->SetName("Sensor_70");
   graph->SetTitle("Sensor_70");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx71[6] = {
   -0.6496345,
   -0.3248173,
   0.3248172,
   0.6496345,
   0.3248172,
   -0.3248173};
   Double_t Graph_fy71[6] = {
   -4.5008,
   -5.0634,
   -5.0634,
   -4.5008,
   -3.9382,
   -3.9382};
   graph = new TGraph(6,Graph_fx71,Graph_fy71);
   graph->SetName("Sensor_71");
   graph->SetTitle("Sensor_71");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx72[6] = {
   -0.6496345,
   -0.3248173,
   0.3248172,
   0.6496345,
   0.3248172,
   -0.3248173};
   Double_t Graph_fy72[6] = {
   -5.626,
   -6.1886,
   -6.1886,
   -5.626,
   -5.0634,
   -5.0634};
   graph = new TGraph(6,Graph_fx72,Graph_fy72);
   graph->SetName("Sensor_72");
   graph->SetTitle("Sensor_72");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx73[4] = {
   -1.624086,
   -1.299269,
   -0.6496345,
   -0.3248172};
   Double_t Graph_fy73[4] = {
   6.1886,
   5.626,
   5.626,
   6.1886};
   graph = new TGraph(4,Graph_fx73,Graph_fy73);
   graph->SetName("Sensor_73");
   graph->SetTitle("Sensor_73");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx74[6] = {
   -1.624086,
   -1.299269,
   -0.6496345,
   -0.3248172,
   -0.6496345,
   -1.299269};
   Double_t Graph_fy74[6] = {
   5.0634,
   4.5008,
   4.5008,
   5.0634,
   5.626,
   5.626};
   graph = new TGraph(6,Graph_fx74,Graph_fy74);
   graph->SetName("Sensor_74");
   graph->SetTitle("Sensor_74");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx75[6] = {
   -1.624086,
   -1.299269,
   -0.6496345,
   -0.3248172,
   -0.6496345,
   -1.299269};
   Double_t Graph_fy75[6] = {
   3.9382,
   3.3756,
   3.3756,
   3.9382,
   4.5008,
   4.5008};
   graph = new TGraph(6,Graph_fx75,Graph_fy75);
   graph->SetName("Sensor_75");
   graph->SetTitle("Sensor_75");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx76[6] = {
   -1.624086,
   -1.299269,
   -0.6496345,
   -0.3248172,
   -0.6496345,
   -1.299269};
   Double_t Graph_fy76[6] = {
   2.813,
   2.2504,
   2.2504,
   2.813,
   3.3756,
   3.3756};
   graph = new TGraph(6,Graph_fx76,Graph_fy76);
   graph->SetName("Sensor_76");
   graph->SetTitle("Sensor_76");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx77[6] = {
   -1.624086,
   -1.299269,
   -0.6496345,
   -0.3248172,
   -0.6496345,
   -1.299269};
   Double_t Graph_fy77[6] = {
   1.6878,
   1.1252,
   1.1252,
   1.6878,
   2.2504,
   2.2504};
   graph = new TGraph(6,Graph_fx77,Graph_fy77);
   graph->SetName("Sensor_77");
   graph->SetTitle("Sensor_77");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx78[6] = {
   -1.624086,
   -1.299269,
   -0.6496345,
   -0.3248172,
   -0.6496345,
   -1.299269};
   Double_t Graph_fy78[6] = {
   0.5626,
   -2.332053e-09,
   -1.166027e-09,
   0.5626,
   1.1252,
   1.1252};
   graph = new TGraph(6,Graph_fx78,Graph_fy78);
   graph->SetName("Sensor_78");
   graph->SetTitle("Sensor_78");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx79[6] = {
   -1.624086,
   -1.299269,
   -0.6496345,
   -0.3248173,
   -0.6496345,
   -1.299269};
   Double_t Graph_fy79[6] = {
   -0.5626,
   -1.1252,
   -1.1252,
   -0.5626,
   -1.166027e-09,
   -2.332053e-09};
   graph = new TGraph(6,Graph_fx79,Graph_fy79);
   graph->SetName("Sensor_79");
   graph->SetTitle("Sensor_79");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx80[6] = {
   -1.624086,
   -1.299269,
   -0.6496345,
   -0.3248173,
   -0.6496345,
   -1.299269};
   Double_t Graph_fy80[6] = {
   -1.6878,
   -2.2504,
   -2.2504,
   -1.6878,
   -1.1252,
   -1.1252};
   graph = new TGraph(6,Graph_fx80,Graph_fy80);
   graph->SetName("Sensor_80");
   graph->SetTitle("Sensor_80");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx81[6] = {
   -1.624086,
   -1.299269,
   -0.6496345,
   -0.3248173,
   -0.6496345,
   -1.299269};
   Double_t Graph_fy81[6] = {
   -2.813,
   -3.3756,
   -3.3756,
   -2.813,
   -2.2504,
   -2.2504};
   graph = new TGraph(6,Graph_fx81,Graph_fy81);
   graph->SetName("Sensor_81");
   graph->SetTitle("Sensor_81");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx82[6] = {
   -1.624086,
   -1.299269,
   -0.6496345,
   -0.3248173,
   -0.6496345,
   -1.299269};
   Double_t Graph_fy82[6] = {
   -3.9382,
   -4.5008,
   -4.5008,
   -3.9382,
   -3.3756,
   -3.3756};
   graph = new TGraph(6,Graph_fx82,Graph_fy82);
   graph->SetName("Sensor_82");
   graph->SetTitle("Sensor_82");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx83[6] = {
   -1.624086,
   -1.299269,
   -0.6496345,
   -0.3248173,
   -0.6496345,
   -1.299269};
   Double_t Graph_fy83[6] = {
   -5.0634,
   -5.626,
   -5.626,
   -5.0634,
   -4.5008,
   -4.5008};
   graph = new TGraph(6,Graph_fx83,Graph_fy83);
   graph->SetName("Sensor_83");
   graph->SetTitle("Sensor_83");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx84[4] = {
   -1.624086,
   -0.3248173,
   -0.6496345,
   -1.299269};
   Double_t Graph_fy84[4] = {
   -6.1886,
   -6.1886,
   -5.626,
   -5.626};
   graph = new TGraph(4,Graph_fx84,Graph_fy84);
   graph->SetName("Sensor_84");
   graph->SetTitle("Sensor_84");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx85[6] = {
   -2.598538,
   -2.273721,
   -1.624086,
   -1.299269,
   -1.624086,
   -2.273721};
   Double_t Graph_fy85[6] = {
   5.626,
   5.0634,
   5.0634,
   5.626,
   6.1886,
   6.1886};
   graph = new TGraph(6,Graph_fx85,Graph_fy85);
   graph->SetName("Sensor_85");
   graph->SetTitle("Sensor_85");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx86[6] = {
   -2.598538,
   -2.273721,
   -1.624086,
   -1.299269,
   -1.624086,
   -2.273721};
   Double_t Graph_fy86[6] = {
   4.5008,
   3.9382,
   3.9382,
   4.5008,
   5.0634,
   5.0634};
   graph = new TGraph(6,Graph_fx86,Graph_fy86);
   graph->SetName("Sensor_86");
   graph->SetTitle("Sensor_86");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx87[6] = {
   -2.598538,
   -2.273721,
   -1.624086,
   -1.299269,
   -1.624086,
   -2.273721};
   Double_t Graph_fy87[6] = {
   3.3756,
   2.813,
   2.813,
   3.3756,
   3.9382,
   3.9382};
   graph = new TGraph(6,Graph_fx87,Graph_fy87);
   graph->SetName("Sensor_87");
   graph->SetTitle("Sensor_87");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx88[6] = {
   -2.598538,
   -2.273721,
   -1.624086,
   -1.299269,
   -1.624086,
   -2.273721};
   Double_t Graph_fy88[6] = {
   2.2504,
   1.6878,
   1.6878,
   2.2504,
   2.813,
   2.813};
   graph = new TGraph(6,Graph_fx88,Graph_fy88);
   graph->SetName("Sensor_88");
   graph->SetTitle("Sensor_88");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx89[6] = {
   -2.598538,
   -2.273721,
   -1.624086,
   -1.299269,
   -1.624086,
   -2.273721};
   Double_t Graph_fy89[6] = {
   1.1252,
   0.5626,
   0.5626,
   1.1252,
   1.6878,
   1.6878};
   graph = new TGraph(6,Graph_fx89,Graph_fy89);
   graph->SetName("Sensor_89");
   graph->SetTitle("Sensor_89");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx90[6] = {
   -2.598538,
   -2.273721,
   -1.624086,
   -1.299269,
   -1.624086,
   -2.273721};
   Double_t Graph_fy90[6] = {
   -4.664107e-09,
   -0.5626,
   -0.5626,
   -2.332053e-09,
   0.5626,
   0.5626};
   graph = new TGraph(6,Graph_fx90,Graph_fy90);
   graph->SetName("Sensor_90");
   graph->SetTitle("Sensor_90");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx91[6] = {
   -2.598538,
   -2.273721,
   -1.624086,
   -1.299269,
   -1.624086,
   -2.273721};
   Double_t Graph_fy91[6] = {
   -1.1252,
   -1.6878,
   -1.6878,
   -1.1252,
   -0.5626,
   -0.5626};
   graph = new TGraph(6,Graph_fx91,Graph_fy91);
   graph->SetName("Sensor_91");
   graph->SetTitle("Sensor_91");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx92[6] = {
   -2.598538,
   -2.273721,
   -1.624086,
   -1.299269,
   -1.624086,
   -2.273721};
   Double_t Graph_fy92[6] = {
   -2.2504,
   -2.813,
   -2.813,
   -2.2504,
   -1.6878,
   -1.6878};
   graph = new TGraph(6,Graph_fx92,Graph_fy92);
   graph->SetName("Sensor_92");
   graph->SetTitle("Sensor_92");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx93[6] = {
   -2.598538,
   -2.273721,
   -1.624086,
   -1.299269,
   -1.624086,
   -2.273721};
   Double_t Graph_fy93[6] = {
   -3.3756,
   -3.9382,
   -3.9382,
   -3.3756,
   -2.813,
   -2.813};
   graph = new TGraph(6,Graph_fx93,Graph_fy93);
   graph->SetName("Sensor_93");
   graph->SetTitle("Sensor_93");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx94[6] = {
   -2.598538,
   -2.273721,
   -1.624086,
   -1.299269,
   -1.624086,
   -2.273721};
   Double_t Graph_fy94[6] = {
   -4.5008,
   -5.0634,
   -5.0634,
   -4.5008,
   -3.9382,
   -3.9382};
   graph = new TGraph(6,Graph_fx94,Graph_fy94);
   graph->SetName("Sensor_94");
   graph->SetTitle("Sensor_94");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx95[6] = {
   -2.598538,
   -2.273721,
   -1.624086,
   -1.299269,
   -1.624086,
   -2.273721};
   Double_t Graph_fy95[6] = {
   -5.626,
   -6.1886,
   -6.1886,
   -5.626,
   -5.0634,
   -5.0634};
   graph = new TGraph(6,Graph_fx95,Graph_fy95);
   graph->SetName("Sensor_95");
   graph->SetTitle("Sensor_95");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx96[4] = {
   -3.57299,
   -3.248172,
   -2.598538,
   -2.273721};
   Double_t Graph_fy96[4] = {
   6.1886,
   5.626,
   5.626,
   6.1886};
   graph = new TGraph(4,Graph_fx96,Graph_fy96);
   graph->SetName("Sensor_96");
   graph->SetTitle("Sensor_96");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx97[6] = {
   -3.57299,
   -3.248172,
   -2.598538,
   -2.273721,
   -2.598538,
   -3.248172};
   Double_t Graph_fy97[6] = {
   5.0634,
   4.5008,
   4.5008,
   5.0634,
   5.626,
   5.626};
   graph = new TGraph(6,Graph_fx97,Graph_fy97);
   graph->SetName("Sensor_97");
   graph->SetTitle("Sensor_97");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx98[6] = {
   -3.57299,
   -3.248172,
   -2.598538,
   -2.273721,
   -2.598538,
   -3.248172};
   Double_t Graph_fy98[6] = {
   3.9382,
   3.3756,
   3.3756,
   3.9382,
   4.5008,
   4.5008};
   graph = new TGraph(6,Graph_fx98,Graph_fy98);
   graph->SetName("Sensor_98");
   graph->SetTitle("Sensor_98");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx99[6] = {
   -3.57299,
   -3.248172,
   -2.598538,
   -2.273721,
   -2.598538,
   -3.248172};
   Double_t Graph_fy99[6] = {
   2.813,
   2.2504,
   2.2504,
   2.813,
   3.3756,
   3.3756};
   graph = new TGraph(6,Graph_fx99,Graph_fy99);
   graph->SetName("Sensor_99");
   graph->SetTitle("Sensor_99");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx100[6] = {
   -3.57299,
   -3.248172,
   -2.598538,
   -2.273721,
   -2.598538,
   -3.248172};
   Double_t Graph_fy100[6] = {
   1.6878,
   1.1252,
   1.1252,
   1.6878,
   2.2504,
   2.2504};
   graph = new TGraph(6,Graph_fx100,Graph_fy100);
   graph->SetName("Sensor_100");
   graph->SetTitle("Sensor_100");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx101[6] = {
   -3.57299,
   -3.248173,
   -2.598538,
   -2.273721,
   -2.598538,
   -3.248172};
   Double_t Graph_fy101[6] = {
   0.5626,
   -5.830134e-09,
   -4.664107e-09,
   0.5626,
   1.1252,
   1.1252};
   graph = new TGraph(6,Graph_fx101,Graph_fy101);
   graph->SetName("Sensor_101");
   graph->SetTitle("Sensor_101");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx102[6] = {
   -3.57299,
   -3.248173,
   -2.598538,
   -2.273721,
   -2.598538,
   -3.248173};
   Double_t Graph_fy102[6] = {
   -0.5626,
   -1.1252,
   -1.1252,
   -0.5626,
   -4.664107e-09,
   -5.830134e-09};
   graph = new TGraph(6,Graph_fx102,Graph_fy102);
   graph->SetName("Sensor_102");
   graph->SetTitle("Sensor_102");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx103[6] = {
   -3.57299,
   -3.248173,
   -2.598538,
   -2.273721,
   -2.598538,
   -3.248173};
   Double_t Graph_fy103[6] = {
   -1.6878,
   -2.2504,
   -2.2504,
   -1.6878,
   -1.1252,
   -1.1252};
   graph = new TGraph(6,Graph_fx103,Graph_fy103);
   graph->SetName("Sensor_103");
   graph->SetTitle("Sensor_103");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx104[6] = {
   -3.57299,
   -3.248173,
   -2.598538,
   -2.273721,
   -2.598538,
   -3.248173};
   Double_t Graph_fy104[6] = {
   -2.813,
   -3.3756,
   -3.3756,
   -2.813,
   -2.2504,
   -2.2504};
   graph = new TGraph(6,Graph_fx104,Graph_fy104);
   graph->SetName("Sensor_104");
   graph->SetTitle("Sensor_104");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx105[6] = {
   -3.57299,
   -3.248173,
   -2.598538,
   -2.273721,
   -2.598538,
   -3.248173};
   Double_t Graph_fy105[6] = {
   -3.9382,
   -4.5008,
   -4.5008,
   -3.9382,
   -3.3756,
   -3.3756};
   graph = new TGraph(6,Graph_fx105,Graph_fy105);
   graph->SetName("Sensor_105");
   graph->SetTitle("Sensor_105");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx106[6] = {
   -3.57299,
   -3.248173,
   -2.598538,
   -2.273721,
   -2.598538,
   -3.248173};
   Double_t Graph_fy106[6] = {
   -5.0634,
   -5.626,
   -5.626,
   -5.0634,
   -4.5008,
   -4.5008};
   graph = new TGraph(6,Graph_fx106,Graph_fy106);
   graph->SetName("Sensor_106");
   graph->SetTitle("Sensor_106");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx107[4] = {
   -3.57299,
   -2.273721,
   -2.598538,
   -3.248173};
   Double_t Graph_fy107[4] = {
   -6.1886,
   -6.1886,
   -5.626,
   -5.626};
   graph = new TGraph(4,Graph_fx107,Graph_fy107);
   graph->SetName("Sensor_107");
   graph->SetTitle("Sensor_107");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx108[4] = {
   -4.222624,
   -3.57299,
   -3.248172,
   -3.57299};
   Double_t Graph_fy108[4] = {
   5.0634,
   5.0634,
   5.626,
   6.1886};
   graph = new TGraph(4,Graph_fx108,Graph_fy108);
   graph->SetName("Sensor_108");
   graph->SetTitle("Sensor_108");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx109[6] = {
   -4.547441,
   -4.222624,
   -3.57299,
   -3.248172,
   -3.57299,
   -4.222624};
   Double_t Graph_fy109[6] = {
   4.5008,
   3.9382,
   3.9382,
   4.5008,
   5.0634,
   5.0634};
   graph = new TGraph(6,Graph_fx109,Graph_fy109);
   graph->SetName("Sensor_109");
   graph->SetTitle("Sensor_109");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx110[6] = {
   -4.547441,
   -4.222624,
   -3.57299,
   -3.248172,
   -3.57299,
   -4.222624};
   Double_t Graph_fy110[6] = {
   3.3756,
   2.813,
   2.813,
   3.3756,
   3.9382,
   3.9382};
   graph = new TGraph(6,Graph_fx110,Graph_fy110);
   graph->SetName("Sensor_110");
   graph->SetTitle("Sensor_110");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx111[6] = {
   -4.547441,
   -4.222624,
   -3.57299,
   -3.248172,
   -3.57299,
   -4.222624};
   Double_t Graph_fy111[6] = {
   2.2504,
   1.6878,
   1.6878,
   2.2504,
   2.813,
   2.813};
   graph = new TGraph(6,Graph_fx111,Graph_fy111);
   graph->SetName("Sensor_111");
   graph->SetTitle("Sensor_111");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx112[6] = {
   -4.547441,
   -4.222624,
   -3.57299,
   -3.248172,
   -3.57299,
   -4.222624};
   Double_t Graph_fy112[6] = {
   1.1252,
   0.5626,
   0.5626,
   1.1252,
   1.6878,
   1.6878};
   graph = new TGraph(6,Graph_fx112,Graph_fy112);
   graph->SetName("Sensor_112");
   graph->SetTitle("Sensor_112");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx113[6] = {
   -4.547442,
   -4.222624,
   -3.57299,
   -3.248173,
   -3.57299,
   -4.222624};
   Double_t Graph_fy113[6] = {
   -8.162187e-09,
   -0.5626,
   -0.5626,
   -5.830134e-09,
   0.5626,
   0.5626};
   graph = new TGraph(6,Graph_fx113,Graph_fy113);
   graph->SetName("Sensor_113");
   graph->SetTitle("Sensor_113");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx114[6] = {
   -4.547442,
   -4.222624,
   -3.57299,
   -3.248173,
   -3.57299,
   -4.222624};
   Double_t Graph_fy114[6] = {
   -1.1252,
   -1.6878,
   -1.6878,
   -1.1252,
   -0.5626,
   -0.5626};
   graph = new TGraph(6,Graph_fx114,Graph_fy114);
   graph->SetName("Sensor_114");
   graph->SetTitle("Sensor_114");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx115[6] = {
   -4.547442,
   -4.222624,
   -3.57299,
   -3.248173,
   -3.57299,
   -4.222624};
   Double_t Graph_fy115[6] = {
   -2.2504,
   -2.813,
   -2.813,
   -2.2504,
   -1.6878,
   -1.6878};
   graph = new TGraph(6,Graph_fx115,Graph_fy115);
   graph->SetName("Sensor_115");
   graph->SetTitle("Sensor_115");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx116[6] = {
   -4.547442,
   -4.222624,
   -3.57299,
   -3.248173,
   -3.57299,
   -4.222624};
   Double_t Graph_fy116[6] = {
   -3.3756,
   -3.9382,
   -3.9382,
   -3.3756,
   -2.813,
   -2.813};
   graph = new TGraph(6,Graph_fx116,Graph_fy116);
   graph->SetName("Sensor_116");
   graph->SetTitle("Sensor_116");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx117[6] = {
   -4.547442,
   -4.222624,
   -3.57299,
   -3.248173,
   -3.57299,
   -4.222624};
   Double_t Graph_fy117[6] = {
   -4.5008,
   -5.0634,
   -5.0634,
   -4.5008,
   -3.9382,
   -3.9382};
   graph = new TGraph(6,Graph_fx117,Graph_fy117);
   graph->SetName("Sensor_117");
   graph->SetTitle("Sensor_117");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx118[4] = {
   -3.57299,
   -3.248173,
   -3.57299,
   -4.222624};
   Double_t Graph_fy118[4] = {
   -6.1886,
   -5.626,
   -5.0634,
   -5.0634};
   graph = new TGraph(4,Graph_fx118,Graph_fy118);
   graph->SetName("Sensor_118");
   graph->SetTitle("Sensor_118");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx119[4] = {
   -5.197076,
   -4.547441,
   -4.222624,
   -4.547441};
   Double_t Graph_fy119[4] = {
   3.3756,
   3.3756,
   3.9382,
   4.5008};
   graph = new TGraph(4,Graph_fx119,Graph_fy119);
   graph->SetName("Sensor_119");
   graph->SetTitle("Sensor_119");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx120[6] = {
   -5.521893,
   -5.197076,
   -4.547441,
   -4.222624,
   -4.547441,
   -5.197076};
   Double_t Graph_fy120[6] = {
   2.813,
   2.2504,
   2.2504,
   2.813,
   3.3756,
   3.3756};
   graph = new TGraph(6,Graph_fx120,Graph_fy120);
   graph->SetName("Sensor_120");
   graph->SetTitle("Sensor_120");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx121[6] = {
   -5.521893,
   -5.197076,
   -4.547441,
   -4.222624,
   -4.547441,
   -5.197076};
   Double_t Graph_fy121[6] = {
   1.6878,
   1.1252,
   1.1252,
   1.6878,
   2.2504,
   2.2504};
   graph = new TGraph(6,Graph_fx121,Graph_fy121);
   graph->SetName("Sensor_121");
   graph->SetTitle("Sensor_121");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx122[6] = {
   -5.521893,
   -5.197076,
   -4.547442,
   -4.222624,
   -4.547441,
   -5.197076};
   Double_t Graph_fy122[6] = {
   0.5626,
   -9.328214e-09,
   -8.162187e-09,
   0.5626,
   1.1252,
   1.1252};
   graph = new TGraph(6,Graph_fx122,Graph_fy122);
   graph->SetName("Sensor_122");
   graph->SetTitle("Sensor_122");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx123[6] = {
   -5.521893,
   -5.197076,
   -4.547442,
   -4.222624,
   -4.547442,
   -5.197076};
   Double_t Graph_fy123[6] = {
   -0.5626,
   -1.1252,
   -1.1252,
   -0.5626,
   -8.162187e-09,
   -9.328214e-09};
   graph = new TGraph(6,Graph_fx123,Graph_fy123);
   graph->SetName("Sensor_123");
   graph->SetTitle("Sensor_123");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx124[6] = {
   -5.521893,
   -5.197076,
   -4.547442,
   -4.222624,
   -4.547442,
   -5.197076};
   Double_t Graph_fy124[6] = {
   -1.6878,
   -2.2504,
   -2.2504,
   -1.6878,
   -1.1252,
   -1.1252};
   graph = new TGraph(6,Graph_fx124,Graph_fy124);
   graph->SetName("Sensor_124");
   graph->SetTitle("Sensor_124");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx125[6] = {
   -5.521893,
   -5.197076,
   -4.547442,
   -4.222624,
   -4.547442,
   -5.197076};
   Double_t Graph_fy125[6] = {
   -2.813,
   -3.3756,
   -3.3756,
   -2.813,
   -2.2504,
   -2.2504};
   graph = new TGraph(6,Graph_fx125,Graph_fy125);
   graph->SetName("Sensor_125");
   graph->SetTitle("Sensor_125");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx126[4] = {
   -4.547442,
   -4.222624,
   -4.547442,
   -5.197076};
   Double_t Graph_fy126[4] = {
   -4.5008,
   -3.9382,
   -3.3756,
   -3.3756};
   graph = new TGraph(4,Graph_fx126,Graph_fy126);
   graph->SetName("Sensor_126");
   graph->SetTitle("Sensor_126");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx127[4] = {
   -6.171528,
   -5.521893,
   -5.197076,
   -5.521893};
   Double_t Graph_fy127[4] = {
   1.6878,
   1.6878,
   2.2504,
   2.813};
   graph = new TGraph(4,Graph_fx127,Graph_fy127);
   graph->SetName("Sensor_127");
   graph->SetTitle("Sensor_127");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx128[6] = {
   -6.496345,
   -6.171528,
   -5.521893,
   -5.197076,
   -5.521893,
   -6.171528};
   Double_t Graph_fy128[6] = {
   1.1252,
   0.5626,
   0.5626,
   1.1252,
   1.6878,
   1.6878};
   graph = new TGraph(6,Graph_fx128,Graph_fy128);
   graph->SetName("Sensor_128");
   graph->SetTitle("Sensor_128");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx129[6] = {
   -6.496345,
   -6.171528,
   -5.521893,
   -5.197076,
   -5.521893,
   -6.171528};
   Double_t Graph_fy129[6] = {
   -1.166027e-08,
   -0.5626,
   -0.5626,
   -9.328214e-09,
   0.5626,
   0.5626};
   graph = new TGraph(6,Graph_fx129,Graph_fy129);
   graph->SetName("Sensor_129");
   graph->SetTitle("Sensor_129");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx130[6] = {
   -6.496345,
   -6.171528,
   -5.521893,
   -5.197076,
   -5.521893,
   -6.171528};
   Double_t Graph_fy130[6] = {
   -1.1252,
   -1.6878,
   -1.6878,
   -1.1252,
   -0.5626,
   -0.5626};
   graph = new TGraph(6,Graph_fx130,Graph_fy130);
   graph->SetName("Sensor_130");
   graph->SetTitle("Sensor_130");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx131[4] = {
   -5.521893,
   -5.197076,
   -5.521893,
   -6.171528};
   Double_t Graph_fy131[4] = {
   -2.813,
   -2.2504,
   -1.6878,
   -1.6878};
   graph = new TGraph(4,Graph_fx131,Graph_fy131);
   graph->SetName("Sensor_131");
   graph->SetTitle("Sensor_131");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx132[4] = {
   -7.14598,
   -6.496345,
   -6.171528,
   -6.496345};
   Double_t Graph_fy132[4] = {
   -1.282629e-08,
   -1.166027e-08,
   0.5626,
   1.1252};
   graph = new TGraph(4,Graph_fx132,Graph_fy132);
   graph->SetName("Sensor_132");
   graph->SetTitle("Sensor_132");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   
   Double_t Graph_fx133[4] = {
   -6.496345,
   -6.171528,
   -6.496345,
   -7.14598};
   Double_t Graph_fy133[4] = {
   -1.1252,
   -0.5626,
   -1.166027e-08,
   -1.282629e-08};
   graph = new TGraph(4,Graph_fx133,Graph_fy133);
   graph->SetName("Sensor_133");
   graph->SetTitle("Sensor_133");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   return hitmapoly;
}
