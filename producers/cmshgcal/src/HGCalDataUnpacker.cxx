#include "eudaq/FileReader.hh"
#include "eudaq/PluginManager.hh"

#include <iostream>
#include <sstream>
#include <boost/program_options.hpp>

#include "TFile.h"
#include "TTree.h"
#include "unpacker.h"
#include "compressor.h"

using namespace eudaq;

int main(int argc, char** argv) {
  std::string filePath;
  int runID,nMaxEvent,fifoSize,trailerSize,compressionLevel;
  bool compressedData,showUnpackedData;
  try { 
    namespace po = boost::program_options; 
    po::options_description desc("Options"); 
    desc.add_options() 
      ("help,h", "Print help messages")
      ("filePath", po::value<std::string>(&filePath)->default_value("./data/"), "path to input files")
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
    std::cout << "\t fifoSize = " << fifoSize << std::endl;
    std::cout << "\t tralerSizer = " << trailerSize << std::endl;
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

  size_t event_nr = 0;
  
  unpacker _unpacker;
  do {
    // Get next event:
    DetectorEvent evt = reader.GetDetectorEvent();

    if (evt.IsBORE()) { eudaq::PluginManager::Initialize(evt); continue;}

    const RawDataEvent rev = evt.GetRawSubEvent("HexaBoard");
    size_t nblocks = rev.NumBlocks() ;
    uint32_t firstWord[nblocks][5];
    for( int iblo=0; iblo<nblocks; iblo++ ){
      if( iblo%2==0 ) continue;
      const RawDataEvent::data_t & block = rev.GetBlock(iblo);
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
      firstWord[iblo][3]=rawData32[4];
      _unpacker.unpack_and_fill(rawData32);
      _unpacker.rawDataSanityCheck();
      if(showUnpackedData)_unpacker.printDecodedRawData();
    }
    if (event_nr%1000==0) {
      std::cout<<"Processing event "<<std::dec<<event_nr<<" with "<<nblocks<<std::endl;
      for( int iblo=0; iblo<nblocks; iblo++ )
	std::cout<<"\t\t data block "<<std::dec<<iblo<<" first 4 words = "<<std::hex<<firstWord[iblo][0]<<" "<<std::hex<<firstWord[iblo][1]<<" "<<std::hex<<firstWord[iblo][2]<<" "<<std::hex<<firstWord[iblo][3]<<" "<<std::hex<<firstWord[iblo][4]<<std::endl;
    }
    event_nr++;
    if( event_nr>=nMaxEvent && nMaxEvent!=0 )break;;
  } while (reader.NextEvent());

  return 0;
}
