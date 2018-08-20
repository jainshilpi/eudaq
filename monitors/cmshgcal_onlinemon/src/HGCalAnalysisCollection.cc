#include "HGCalAnalysisCollection.hh"
#include "OnlineMon.hh"

static int counting = 0;
static int events = 0;

bool HGCalAnalysisCollection::isPlaneRegistered(eudaq::StandardPlane p) {
  std::map<eudaq::StandardPlane, HGCalAnalysisHistos *>::iterator it;
  it = _map.find(p);
  return (it != _map.end());
}

void HGCalAnalysisCollection::fillHistograms(const eudaq::StandardEvent &ev, const eudaq::StandardPlane &pl, int eventNumber) {
  //std::cout<<"In HGCalAnalysisCollection::fillHistograms(StandardPlane)"<<std::endl;

  if (pl.Sensor() != "HGCalOnlineAnalysis")
    return;

  if (!isPlaneRegistered(pl)) {
    registerPlane(pl);
    isOnePlaneRegistered = true;
  }

  HGCalAnalysisHistos *histomap = _map[pl];
  histomap->Fill(pl, eventNumber);

  ++counting;
}

void HGCalAnalysisCollection::bookHistograms(const eudaq::StandardEvent &ev) {
  //std::cout<<"In HGCalAnalysisCollection::bookHistograms(StandardEvent)"<<std::endl;

  for (int plane = 0; plane < ev.NumPlanes(); plane++) {
    const eudaq::StandardPlane Plane = ev.GetPlane(plane);
    if (!isPlaneRegistered(Plane)) {
      if (Plane.Sensor() == "HGCalOnlineAnalysis") registerPlane(Plane);
    }
  }
}

void HGCalAnalysisCollection::Write(TFile *file) {
  if (file == NULL) {
    // cout << "HGCalAnalysisCollection::Write File pointer is NULL"<<endl;
    exit(-1);
  }

  if (gDirectory != NULL) // check if this pointer exists
  {
    gDirectory->mkdir("HGCal");
    gDirectory->cd("HGCal");

    std::map<eudaq::StandardPlane, HGCalAnalysisHistos *>::iterator it;
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

void HGCalAnalysisCollection::Calculate(const unsigned int currentEventNumber) {
  if ((currentEventNumber > 10 && currentEventNumber % 1000 * _reduce == 0)) {
    std::map<eudaq::StandardPlane, HGCalAnalysisHistos *>::iterator it;
    for (it = _map.begin(); it != _map.end(); ++it) {
      // std::cout << "Calculating ..." << std::endl;
      it->second->Calculate(currentEventNumber / _reduce);
    }
  }
}

void HGCalAnalysisCollection::Reset() {
  std::map<eudaq::StandardPlane, HGCalAnalysisHistos *>::iterator it;
  for (it = _map.begin(); it != _map.end(); ++it) {
    (*it).second->Reset();
  }
}


void HGCalAnalysisCollection::Fill(const eudaq::StandardEvent &ev, int evNumber) {
  //std::cout<<"In HGCalAnalysisCollection::Fill(StandardEvent)"<<std::endl;

  for (int plane = 0; plane < ev.NumPlanes(); plane++) {
    const eudaq::StandardPlane &Plane = ev.GetPlane(plane);
    if (Plane.Sensor() == "HGCalOnlineAnalysis")
      fillHistograms(ev, Plane, evNumber);
  }
}


HGCalAnalysisHistos *HGCalAnalysisCollection::getHGCalAnalysisHistos(std::string sensor, int id) {
  const eudaq::StandardPlane p(id, "HGCalOnlineAnalysis", sensor);
  return _map[p];
}

void HGCalAnalysisCollection::registerPlane(const eudaq::StandardPlane &p) {
  //std::cout<<"In HGCalAnalysisCollection::registerPlane(StandardPlane)"<<std::endl;

  HGCalAnalysisHistos *tmphisto = new HGCalAnalysisHistos(p, _mon, NumberOfDWCPlanes);
  _map[p] = tmphisto;

  // std::cout << "Registered Plane: " << p.Sensor() << " " << p.ID() <<
  // std::endl;
  // PlaneRegistered(p.Sensor(),p.ID());

  if (_mon != NULL) {
    if (_mon->getOnlineMon() == NULL) {
      return; // don't register items
    }
    // cout << "HGCalAnalysisCollection:: Monitor running in online-mode" << endl;
    char tree[1024], folder[1024];


    sprintf(tree, "%s/Nhits", "HGCalOnlineAnalysis");
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree, getHGCalAnalysisHistos(p.Sensor(), p.ID())->getNHits(), "", 0);
    sprintf(folder, "%s", "HGCalOnlineAnalysis");
    _mon->getOnlineMon()->addTreeItemSummary(folder, tree);


    sprintf(tree, "%s/EnergySum", "HGCalOnlineAnalysis");
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree, getHGCalAnalysisHistos(p.Sensor(), p.ID())->getEnergysum(), "", 0);
    sprintf(folder, "%s", "HGCalOnlineAnalysis");
    _mon->getOnlineMon()->addTreeItemSummary(folder, tree);


    sprintf(tree, "%s/Depth", "HGCalOnlineAnalysis");
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree, getHGCalAnalysisHistos(p.Sensor(), p.ID())->getDepth(), "", 0);
    sprintf(folder, "%s", "HGCalOnlineAnalysis");
    _mon->getOnlineMon()->addTreeItemSummary(folder, tree);


    sprintf(tree, "%s/NhitsVsEnergySum", "HGCalOnlineAnalysis");
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree, getHGCalAnalysisHistos(p.Sensor(), p.ID())->getNhits_vs_Energysum(), "COLZ2", 0);
    sprintf(folder, "%s", "HGCalOnlineAnalysis");
    _mon->getOnlineMon()->addTreeItemSummary(folder, tree);


    sprintf(tree, "%s/NhitsVsDepth", "HGCalOnlineAnalysis");
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree, getHGCalAnalysisHistos(p.Sensor(), p.ID())->getNits_vs_Depth(), "COLZ2", 0);
    sprintf(folder, "%s", "HGCalOnlineAnalysis");
    _mon->getOnlineMon()->addTreeItemSummary(folder, tree);


    sprintf(tree, "%s/EnergySumVsDepth", "HGCalOnlineAnalysis");
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree, getHGCalAnalysisHistos(p.Sensor(), p.ID())->getEnergysum_vs_Depth(), "COLZ2", 0);
    sprintf(folder, "%s", "HGCalOnlineAnalysis");
    _mon->getOnlineMon()->addTreeItemSummary(folder, tree);
  }
}
