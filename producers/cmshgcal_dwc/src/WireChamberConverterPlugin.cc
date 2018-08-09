#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"

#include <bitset>
#include <boost/format.hpp>

#include "CAENv1290Unpacker.h"

// All LCIO-specific parts are put in conditional compilation blocks
// so that the other parts may still be used if LCIO is not available.
#if USE_LCIO
#include "IMPL/LCEventImpl.h"
#include "IMPL/TrackerRawDataImpl.h"
#include "IMPL/LCCollectionVec.h"
#include "lcio.h"
#endif

CAENv1290Unpacker* tdc_unpacker;

namespace eudaq {

  // The event type for which this converter plugin will be registered
  // Modify this to match your actual event type (from the Producer)
  static const char *EVENT_TYPE = "DWC";

  // Declare a new class that inherits from DataConverterPlugin
  class WireChamberConverterPlugin : public DataConverterPlugin {

  public:
    // This is called once at the beginning of each run.
    // You may extract information from the BORE and/or configuration
    // and store it in member variables to use during the decoding later.
    virtual void Initialize(const Event &bore, const Configuration &cnf) {

#ifndef WIN32 // some linux Stuff //$$change
      (void)cnf; // just to suppress a warning about unused parameter cnf
#endif
    }

    // This should return the trigger ID (as provided by the TLU)
    // if it was read out, otherwise it can either return (unsigned)-1,
    // or be left undefined as there is already a default version.
    virtual unsigned GetTriggerID(const Event &ev) const {
      static const unsigned TRIGGER_OFFSET = 8;
      // Make sure the event is of class RawDataEvent
      if (const RawDataEvent *rev = dynamic_cast<const RawDataEvent *>(&ev)) {
	// This is just an example, modified it to suit your raw data format
        // Make sure we have at least one block of data, and it is large enough
        if (rev->NumBlocks() > 0 &&
            rev->GetBlock(0).size() >= (TRIGGER_OFFSET + sizeof(short))) {
          // Read a little-endian unsigned short from offset TRIGGER_OFFSET
          return getlittleendian<unsigned short>( &rev->GetBlock(0)[TRIGGER_OFFSET]);
        }
      }
      // If we are unable to extract the Trigger ID, signal with (unsigned)-1
      return (unsigned)-1;
    }

    // Here, the data from the RawDataEvent is extracted into a StandardEvent.
    // The return value indicates whether the conversion was successful.
    // Again, this is just an example, adapted it for the actual data layout.
    virtual bool GetStandardSubEvent(StandardEvent &sev, const Event &ev) const {

      // If the event type is used for different sensors
      // they can be differentiated here
      const std::string sensortype = "DWC";

      //std::cout<<"\t Dans GetStandardSubEvent()  "<<std::endl;

      const RawDataEvent * rev = dynamic_cast<const RawDataEvent *> ( &ev );


      sev.SetTag("cpuTime_mus", rev->GetTimestamp());

      //rev->Print(std::cout);

      std::vector<tdcData*> unpacked;
      std::map<std::pair<int, int>, uint32_t> time_of_arrivals;


      const unsigned nBlocks = rev->NumBlocks();
      //std::cout<<"Number o Raw Data Blocks: "<<nBlocks<<std::endl;

      //one plane for offline 
      int max_nhits = 1;
      for (unsigned tdc_index=0; tdc_index<nBlocks; tdc_index++){
              
        //std::cout<<"Block = "<<tdc_index<<"  Raw GetID = "<<rev->GetID(tdc_index)<<std::endl;

        const RawDataEvent::data_t & bl = rev->GetBlock(tdc_index);

        //std::cout<<"size of block: "<<bl.size()<<std::endl;

        //conversion block
        std::vector<uint32_t> Words;
        Words.resize(bl.size() / sizeof(uint32_t));
        std::memcpy(&Words[0], &bl[0], bl.size());   


        unpacked.push_back(tdc_unpacker->ConvertTDCData(Words));        
                
      }


      //plane for offline reconstruction
      StandardPlane full_dwc_plane(0, EVENT_TYPE, sensortype+"_fullTDC");
      full_dwc_plane.SetSizeRaw(nBlocks, 16, 1+NHits_forReadout);    //nBlocks = number of TDCs, 16=number of channels in the TDC, one frame
      
      for (unsigned tdc_index=0; tdc_index<nBlocks; tdc_index++)for (int channel=0; channel<16; channel++) {
        full_dwc_plane.SetPixel(tdc_index*16+channel, tdc_index, channel, unpacked[tdc_index]->hits[channel].size(), false, 0);           
      }
        
      for (unsigned tdc_index=0; tdc_index<nBlocks; tdc_index++)for (int channel=0; channel<16; channel++) {
        time_of_arrivals[std::make_pair(tdc_index, channel)] = unpacked[tdc_index]->timeOfArrivals[channel];    //it is peculiar that we need that hack in order to access the time of arrivals in a subsequent step
        for (size_t hit_index=0; hit_index<unpacked[tdc_index]->hits[channel].size(); hit_index++) {
          if (hit_index>=NHits_forReadout) break;
          full_dwc_plane.SetPixel(tdc_index*16+channel, tdc_index, channel, unpacked[tdc_index]->hits[channel][hit_index], false, 1+hit_index);            
        }
      }
      sev.AddPlane(full_dwc_plane);

      //plane with the trigger time stamps
      StandardPlane timestamp_dwc_plane(0, EVENT_TYPE, sensortype+"_triggerTimestamps");
      timestamp_dwc_plane.SetSizeRaw(nBlocks, 1, 1);  
      for (unsigned tdc_index=0; tdc_index<nBlocks; tdc_index++)  timestamp_dwc_plane.SetPixel(tdc_index, tdc_index, 1, unpacked[tdc_index]->extended_trigger_timestamp, false, 0);
      sev.AddPlane(timestamp_dwc_plane);


      //Hard coded assignment of channel time stamps to DWC planes for the DQM to guarantee downward compatibility with <=July 2018 DQM
      //note: for the DWC correlation plots, the indexing must start at 0!
      if (nBlocks>=1) {    //at least one TDC
        StandardPlane DWCE_plane(0, EVENT_TYPE, sensortype);
        DWCE_plane.SetSizeRaw(4, 1, 1);
        DWCE_plane.SetPixel(0, 0, 0, time_of_arrivals[std::make_pair(0, 0)]);   //left
        DWCE_plane.SetPixel(1, 0, 0, time_of_arrivals[std::make_pair(0, 1)]);   //right
        DWCE_plane.SetPixel(2, 0, 0, time_of_arrivals[std::make_pair(0, 2)]);   //up
        DWCE_plane.SetPixel(3, 0, 0, time_of_arrivals[std::make_pair(0, 3)]);   //down
        sev.AddPlane(DWCE_plane);

        StandardPlane DWCD_plane(1, EVENT_TYPE, sensortype);
        DWCD_plane.SetSizeRaw(4, 1, 1);
        DWCD_plane.SetPixel(0, 0, 0, time_of_arrivals[std::make_pair(0, 4)]);   //left
        DWCD_plane.SetPixel(1, 0, 0, time_of_arrivals[std::make_pair(0, 5)]);   //right
        DWCD_plane.SetPixel(2, 0, 0, time_of_arrivals[std::make_pair(0, 6)]);   //up
        DWCD_plane.SetPixel(3, 0, 0, time_of_arrivals[std::make_pair(0, 7)]);   //down
        sev.AddPlane(DWCD_plane);

        StandardPlane DWCA_plane(2, EVENT_TYPE, sensortype);
        DWCA_plane.SetSizeRaw(4, 1, 1);
        DWCA_plane.SetPixel(0, 0, 0, time_of_arrivals[std::make_pair(0, 8)]);   //left
        DWCA_plane.SetPixel(1, 0, 0, time_of_arrivals[std::make_pair(0, 9)]);   //right
        DWCA_plane.SetPixel(2, 0, 0, time_of_arrivals[std::make_pair(0, 10)]);   //up
        DWCA_plane.SetPixel(3, 0, 0, time_of_arrivals[std::make_pair(0, 11)]);   //down
        sev.AddPlane(DWCA_plane);

        StandardPlane DWCExt_plane(3, EVENT_TYPE, sensortype);
        DWCExt_plane.SetSizeRaw(4, 1, 1);
        DWCExt_plane.SetPixel(0, 0, 0, time_of_arrivals[std::make_pair(0, 12)]);   //left
        DWCExt_plane.SetPixel(1, 0, 0, time_of_arrivals[std::make_pair(0, 13)]);   //right
        DWCExt_plane.SetPixel(2, 0, 0, time_of_arrivals[std::make_pair(0, 14)]);   //up
        DWCExt_plane.SetPixel(3, 0, 0, time_of_arrivals[std::make_pair(0, 15)]);   //down
        sev.AddPlane(DWCExt_plane);
      }


      //Hard coded assignment of channel time stamps to DWC planes for the DQM to guarantee downward compatibility with <=July 2018 DQM
      //note: for the DWC correlation plots, the indexing must start at 0!
      if (nBlocks>=2) {    //at least two TDCs
        StandardPlane DWCE_plane_2(4, EVENT_TYPE, sensortype);
        DWCE_plane_2.SetSizeRaw(4, 1, 1);
        DWCE_plane_2.SetPixel(0, 0, 0, time_of_arrivals[std::make_pair(1, 0)]);   //left
        DWCE_plane_2.SetPixel(1, 0, 0, time_of_arrivals[std::make_pair(1, 1)]);   //right
        DWCE_plane_2.SetPixel(2, 0, 0, time_of_arrivals[std::make_pair(1, 2)]);   //up
        DWCE_plane_2.SetPixel(3, 0, 0, time_of_arrivals[std::make_pair(1, 3)]);   //down
        sev.AddPlane(DWCE_plane_2);

        StandardPlane DWCD_plane_2(5, EVENT_TYPE, sensortype);
        DWCD_plane_2.SetSizeRaw(4, 1, 1);
        DWCD_plane_2.SetPixel(0, 0, 0, time_of_arrivals[std::make_pair(1, 4)]);   //left
        DWCD_plane_2.SetPixel(1, 0, 0, time_of_arrivals[std::make_pair(1, 5)]);   //right
        DWCD_plane_2.SetPixel(2, 0, 0, time_of_arrivals[std::make_pair(1, 6)]);   //up
        DWCD_plane_2.SetPixel(3, 0, 0, time_of_arrivals[std::make_pair(1, 7)]);   //down
        sev.AddPlane(DWCD_plane_2);

        StandardPlane DWCA_plane_2(6, EVENT_TYPE, sensortype);
        DWCA_plane_2.SetSizeRaw(4, 1, 1);
        DWCA_plane_2.SetPixel(0, 0, 0, time_of_arrivals[std::make_pair(1, 8)]);   //left
        DWCA_plane_2.SetPixel(1, 0, 0, time_of_arrivals[std::make_pair(1, 9)]);   //right
        DWCA_plane_2.SetPixel(2, 0, 0, time_of_arrivals[std::make_pair(1, 10)]);   //up
        DWCA_plane_2.SetPixel(3, 0, 0, time_of_arrivals[std::make_pair(1, 11)]);   //down
        sev.AddPlane(DWCA_plane_2);

        StandardPlane DWCExt_plane_2(7, EVENT_TYPE, sensortype);
        DWCExt_plane_2.SetSizeRaw(4, 1, 1);
        DWCExt_plane_2.SetPixel(0, 0, 0, time_of_arrivals[std::make_pair(1, 12)]);   //left
        DWCExt_plane_2.SetPixel(1, 0, 0, time_of_arrivals[std::make_pair(1, 13)]);   //right
        DWCExt_plane_2.SetPixel(2, 0, 0, time_of_arrivals[std::make_pair(1, 14)]);   //up
        DWCExt_plane_2.SetPixel(3, 0, 0, time_of_arrivals[std::make_pair(1, 15)]);   //down
        sev.AddPlane(DWCExt_plane_2);
      }
    

      for (size_t i=0; i<unpacked.size(); i++) delete unpacked[i];
      unpacked.clear();

      // Indicate that data was successfully converted
      return true;

    }


#if USE_LCIO
    // This is where the conversion to LCIO is done
    virtual lcio::LCEvent *GetLCIOEvent(const Event * /*ev*/) const {
      return 0;
    }
#endif
  
  private:
    CAENv1290Unpacker* tdc_unpacker;


    unsigned NHits_forReadout;      //limit the number of converted hits to the first NHits_forReadout

    // The constructor can be private, only one static instance is created
    // The DataConverterPlugin constructor must be passed the event type
    // in order to register this converter for the corresponding conversions
    // Member variables should also be initialized to default values here.
    WireChamberConverterPlugin()
        : DataConverterPlugin(EVENT_TYPE) {
          tdc_unpacker = new CAENv1290Unpacker(16);
          NHits_forReadout = 20;
        }

    // Information extracted in Initialize() can be stored here:
    ~WireChamberConverterPlugin() {
      delete tdc_unpacker;
    }

    // The single instance of this converter plugin
    static WireChamberConverterPlugin m_instance;
    }; // class WireChamberConverterPlugin

    // Instantiate the converter plugin instance
    WireChamberConverterPlugin WireChamberConverterPlugin::m_instance;

  } // namespace eudaq
