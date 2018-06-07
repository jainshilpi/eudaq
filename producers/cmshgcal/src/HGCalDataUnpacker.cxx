#include "eudaq/FileReader.hh"
#include "eudaq/PluginManager.hh"

#include <iostream>
#include <sstream>
#include <exception>
#include <boost/program_options.hpp>

#include "TFile.h"
#include "TTree.h"
#include "unpacker.h"
#include "compressor.h"

using namespace eudaq;

int main(int argc, char** argv) {
  std::string filePath,outputPath,eventType;
  int runID,nMaxEvent,fifoSize,trailerSize,compressionLevel;
  bool compressedData,showUnpackedData;
  try { 
    namespace po = boost::program_options; 
    po::options_description desc("Options"); 
    desc.add_options() 
      ("help,h", "Print help messages")
      ("filePath", po::value<std::string>(&filePath)->default_value("./data/"), "path to input files")
      ("outputPath", po::value<std::string>(&outputPath)->default_value("./data_root/"), "path to input files")
      ("eventType", po::value<std::string>(&eventType)->default_value("HexaBoard"), "collection name of the event that has to be unpacked")
      ("runID", po::value<int>(&runID)->default_value(0), "runID")
      ("nMaxEvent", po::value<int>(&nMaxEvent)->default_value(0), "maximum numver of events before stopping the run")
      ("fifoSize", po::value<int>(&fifoSize)->default_value(30787), "number of 32 bits words in ctrl ORM FIFO")
      ("trailerSize", po::value<int>(&trailerSize)->default_value(3), "number of 32 bits words in trailer (1 trailer per ctrl ORM)")
      ("showUnpackedData", po::value<bool>(&showUnpackedData)->default_value(false), "set to true to print unpacked data")
      ("compressedData", po::value<bool>(&compressedData)->default_value(false), "set to true if data were compressed (with boost gzip)")
      ("compressionLevel", po::value<int>(&compressionLevel)->default_value(5), "level of compression (0 for no compression, 9 for best compressino)");
    
    po::variables_map vm; 
    try { 
      po::store(po::parse_command_line(argc, argv, desc),  vm); 
      if ( vm.count("help")  ) { 
        std::cout << desc << std::endl; 
        return 0; 
      } 
      po::notify(vm);
    }
    catch(po::error& e) { 
      std::cerr << "ERROR: " << e.what() << std::endl << std::endl; 
      std::cerr << desc << std::endl; 
      return 1; 
    }
    std::cout << "Printing configuration : " << std::endl;
    std::cout << "\t runID = " << runID << std::endl;
    std::cout << "\t nMaxEvent = " << nMaxEvent << std::endl;
    std::cout << "\t filePath = " << filePath << std::endl;
    std::cout << "\t eventType = " << eventType << std::endl;
    std::cout << "\t fifoSize = " << fifoSize << std::endl;
    std::cout << "\t trailerSize = " << trailerSize << std::endl;
    std::cout << "\t showUnpackedData = " << showUnpackedData << std::endl;
    std::cout << "\t compressedData = " << compressedData << std::endl;
    std::cout << "\t compressionLevel = " << compressionLevel << std::endl;
  }
  catch(std::exception& e) { 
    std::cerr << "Unhandled Exception reached the top of main: " 
              << e.what() << ", application will now exit" << std::endl; 
    return 0; 
  } 

  std::ostringstream os( std::ostringstream::ate );
  os.str("");
  os << filePath << "run" << std::setw(6) << std::setfill('0') << runID << ".raw";
  FileReader reader = FileReader(os.str(), "");

  os.str("");
  os << outputPath << "run" << std::setw(6) << std::setfill('0') << runID << ".root";
  TFile *rfile = new TFile(os.str().c_str(),"RECREATE");
  TTree *rtree = new TTree("hgcalraw","hgcal raw data");
  std::vector<uint16_t> sk2cmsData;
  long long timeStamp;
  int triggerId;
  int event_nr(0);
  rtree->Branch("eventId",&event_nr);
  rtree->Branch("triggerId",&triggerId);
  rtree->Branch("timeStamp",&timeStamp);
  rtree->Branch("sk2cmsData","std::vector<uint16_t>",&sk2cmsData);
    
  unpacker _unpacker;
  do {
    DetectorEvent evt = reader.GetDetectorEvent();
    if (evt.IsBORE()) { eudaq::PluginManager::Initialize(evt); continue;}
    // std::ostringstream os(std::ostringstream::ate );
    // evt.Print(os);
    // std::cout << os.str() << std::endl;
    // std::cout << evt.NumEvents() << std::endl;
    sk2cmsData.clear();
    for( int i=0; i<evt.NumEvents(); i++ ){
      const RawDataEvent *rev = dynamic_cast<const RawDataEvent*>( evt.GetEvent(i) );
      if(NULL==rev || rev->GetSubType() != eventType){//check if rev is NULL is important because the DetectorEvent can contains non RawDataEvent (e.g. TLUEvent) sub-events
	continue;
      }
      size_t nblocks = rev->NumBlocks() ;
      uint32_t firstWord[nblocks][5];
      for( int iblo=0; iblo<nblocks; iblo++ ){
	if( iblo%2==0 ) continue;
	const RawDataEvent::data_t & block = rev->GetBlock(iblo);
	//std::cout << "size of block = " << std::dec << block.size() << std::endl;
	std::vector<uint32_t> rawData32;
	if( compressedData==true ){
	  std::string decompressed=compressor::decompress(block);
	  std::vector<uint8_t> decompData( decompressed.begin(), decompressed.end() );
	  rawData32.resize(decompData.size() / sizeof(uint32_t));
	  std::memcpy(&rawData32[0], &decompData[0], decompData.size());
	}
	else{
	  rawData32.resize(block.size() / sizeof(uint32_t));
	  std::memcpy(&rawData32[0], &block[0], block.size());
	}

	firstWord[iblo][0]=rawData32[0];
	firstWord[iblo][1]=rawData32[1];
	firstWord[iblo][2]=rawData32[2];
	firstWord[iblo][3]=rawData32[3];
	firstWord[iblo][4]=rawData32[4];
	_unpacker.unpack_and_fill(rawData32);
	//_unpacker.rawDataSanityCheck();
	if(showUnpackedData)_unpacker.printDecodedRawData();
	sk2cmsData.insert(sk2cmsData.end(),_unpacker.getDecodedData().begin(),_unpacker.getDecodedData().end());
      }
      if (event_nr%100==0) {
	std::cout<<"Processing event "<<std::dec<<event_nr<<" with "<<nblocks<<" blocks"<<std::endl;
	for( int iblo=0; iblo<nblocks; iblo++ )
	  if( iblo%2!=0 )
	    std::cout<<"\t\t data block "<<std::dec<<iblo<<" first 4 words = "<<std::hex<<firstWord[iblo][0]<<" "<<std::hex<<firstWord[iblo][1]<<" "<<std::hex<<firstWord[iblo][2]<<" "<<std::hex<<firstWord[iblo][3]<<" "<<std::hex<<firstWord[iblo][4]<<std::endl;
      } 
    }
    triggerId=_unpacker.lastTriggerId();
    timeStamp=_unpacker.lastTimeStamp();
    rtree->Fill();
    event_nr++;
    if( event_nr>=nMaxEvent && nMaxEvent!=0 )break;;
    
  } while (reader.NextEvent());

  _unpacker.checkTimingSync();
  rfile->Write();
  rfile->Close();
  return 0;
}
