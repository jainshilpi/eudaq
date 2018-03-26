#include "CrossCorrelationCollection.hh"
#include "OnlineMon.hh"

static int counting = 0;
static int events = 0;

bool CrossCorrelationCollection::isPlaneRegistered(eudaq::StandardPlane p) {
  std::map<eudaq::StandardPlane, CrossCorrelationHistos *>::iterator it;
  it = _map.find(p);
  return (it != _map.end());
}

void CrossCorrelationCollection::fillHistograms(const eudaq::StandardPlane &pl, const eudaq::StandardPlane &plMIMOSA1) {
  //std::cout<<"In CrossCorrelationCollection::fillHistograms(StandardPlane)"<<std::endl;

  if (pl.Sensor().find("HexaBoard")==std::string::npos)
    return;
  
  if (!isPlaneRegistered(pl)) {
    registerPlane(pl);
    isOnePlaneRegistered = true;
  }

  CrossCorrelationHistos *wcmap = _map[pl];
  wcmap->Fill(pl, plMIMOSA1);

  ++counting;
}

void CrossCorrelationCollection::bookHistograms(const eudaq::StandardEvent &ev) {
  //std::cout<<"In CrossCorrelationCollection::bookHistograms(StandardEvent)"<<std::endl;

  for (int plane = 0; plane < ev.NumPlanes(); plane++) {
    const eudaq::StandardPlane Plane = ev.GetPlane(plane);
    if (!isPlaneRegistered(Plane)) {
      if (Plane.Sensor().find("HexaBoard")!=std::string::npos)
	     registerPlane(Plane);
    }
  }
}

void CrossCorrelationCollection::Write(TFile *file) {
  if (file == NULL) {
    // cout << "CrossCorrelationCollection::Write File pointer is NULL"<<endl;
    exit(-1);
  }  

  if (gDirectory != NULL) // check if this pointer exists
  {
    gDirectory->mkdir("CrossCorrelation");
    gDirectory->cd("CrossCorrelation");

    std::map<eudaq::StandardPlane, CrossCorrelationHistos *>::iterator it;
    for (it = _map.begin(); it != _map.end(); ++it) {

      char sensorfolder[255] = "";
      sprintf(sensorfolder, "%s_%d", it->first.Sensor().c_str(), it->first.ID());
      
      // cout << "Making new subfolder " << sensorfolder << endl;
      gDirectory->mkdir(sensorfolder);
      gDirectory->cd(sensorfolder);
      it->second->Write();

      // gDirectory->ls();
      gDirectory->cd("..");
    }
    gDirectory->cd("..");
  }
}

void CrossCorrelationCollection::Calculate(const unsigned int currentEventNumber) {
  if ((currentEventNumber > 10 && currentEventNumber % 1000 * _reduce == 0)) {
    std::map<eudaq::StandardPlane, CrossCorrelationHistos *>::iterator it;
    for (it = _map.begin(); it != _map.end(); ++it) {
      // std::cout << "Calculating ..." << std::endl;
      it->second->Calculate(currentEventNumber / _reduce);
    }
  }
}

void CrossCorrelationCollection::Reset() {
  std::map<eudaq::StandardPlane, CrossCorrelationHistos *>::iterator it;
  for (it = _map.begin(); it != _map.end(); ++it) {
    (*it).second->Reset();
  }
}


void CrossCorrelationCollection::Fill(const eudaq::StandardEvent &ev, int evNumber) {
  //std::cout<<"In CrossCorrelationCollection::Fill(StandardEvent)"<<std::endl;

  for (int plane = 0; plane < ev.NumPlanes(); plane++) {
    const eudaq::StandardPlane &Plane = ev.GetPlane(plane);
    if (Plane.Sensor().find("HexaBoard")!=std::string::npos) {
      for (int plane2 = 0; plane2 < ev.NumPlanes(); plane2++) {
        const eudaq::StandardPlane &Plane2 = ev.GetPlane(plane2);
        if (Plane2.Sensor()!="MIMOSA26") continue;
        if (Plane2.ID()!=3) continue;
        fillHistograms(Plane, Plane2);
      }
    }
  }
}


CrossCorrelationHistos *CrossCorrelationCollection::getCrossCorrelationHistos(std::string sensor, int id) {
  const eudaq::StandardPlane p(id, "HexaBoard", sensor);
  return _map[p];
}

void CrossCorrelationCollection::registerPlane(const eudaq::StandardPlane &p) {
  //std::cout<<"In CrossCorrelationCollection::registerPlane(StandardPlane)"<<std::endl;

  CrossCorrelationHistos *tmphisto = new CrossCorrelationHistos(p, _mon);
  _map[p] = tmphisto;

  // std::cout << "Registered Plane: " << p.Sensor() << " " << p.ID() <<
  // std::endl;
  // PlaneRegistered(p.Sensor(),p.ID());
  
  if (_mon != NULL) {
    if (_mon->getOnlineMon() == NULL) {
      return; // don't register items
    }
    // cout << "CrossCorrelationCollection:: Monitor running in online-mode" << endl;
    char tree[1024], folder[1024];
    sprintf(folder, "%s", p.Sensor().c_str());
    
    for (int ch=0; ch<128; ch++) {
      sprintf(tree, "XCorrelation/%s/Module %i/Ch_%iCorrelationToMIMOSA3", p.Sensor().c_str(), p.ID(), ch);      
      _mon->getOnlineMon()->registerTreeItem(tree);
      _mon->getOnlineMon()->registerHisto(tree, getCrossCorrelationHistos(p.Sensor(), p.ID())->getMIMOSA_map_ForChannel(ch), "", 0);
      //_mon->getOnlineMon()->addTreeItemSummary(folder, tree); 
      
    }


    sprintf(tree, "XCorrelation/%s/Module %i", p.Sensor().c_str(), p.ID());
    _mon->getOnlineMon()->makeTreeItemSummary(tree);

    
  }
}


