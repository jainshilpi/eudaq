#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"

#include <bitset>
#include <boost/format.hpp>

// All LCIO-specific parts are put in conditional compilation blocks
// so that the other parts may still be used if LCIO is not available.
#if USE_LCIO
#include "IMPL/LCEventImpl.h"
#include "IMPL/TrackerRawDataImpl.h"
#include "IMPL/LCCollectionVec.h"
#include "lcio.h"
#endif

namespace eudaq {

  // The event type for which this converter plugin will be registered
  // Modify this to match your actual event type (from the Producer)
  //static const char *EVENT_TYPE = "RPI";
  static const char *EVENT_TYPE = "WireChambers";

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
      const std::string sensortype = "WireChamber";

      std::cout<<"\t Dans GetStandardSubEvent()  "<<std::endl;

      const RawDataEvent * rev = dynamic_cast<const RawDataEvent *> ( &ev );

      //rev->Print(std::cout);

      const unsigned nBlocks = rev->NumBlocks();
      std::cout<<"Number o Raw Data Blocks: "<<nBlocks<<std::endl;

      for (unsigned blo=0; blo<nBlocks; blo++){
        if (blo != 1) continue;  //we only want the unpacked dwc data
              
        std::cout<<"Block = "<<blo<<"  Raw GetID = "<<rev->GetID(blo)<<std::endl;

        const RawDataEvent::data_t & bl = rev->GetBlock(blo);

        std::cout<<"size of block: "<<bl.size()<<std::endl;


        std::vector<StandardPlane> wcs;
        for (int wc_index=0; wc_index<4; wc_index++) {    //adjust here in case only one DWC is read out
          float xl, xr, yu, yd;
          memcpy(&xl, &bl[4*wc_index*4+0], sizeof(xl));
          memcpy(&xr, &bl[4*wc_index*4+4], sizeof(xr));
          memcpy(&yu, &bl[4*wc_index*4+8], sizeof(yu));
          memcpy(&yd, &bl[4*wc_index*4+12], sizeof(yd));

          //std::cout<<"Wire chamber "<<wc_index<<": "<<"xl="<<xl<<" xr="<<xr<<"     yu="<<yu<<" yd="<<yd<<std::endl;;
          StandardPlane wc(wc_index, EVENT_TYPE, sensortype);
          wc.SetSizeRaw(1, 4);
          wc.SetPixel(0, 0, 0, xl);
          wc.SetPixel(1, 1, 0, xr);
          wc.SetPixel(2, 0, 0, yu);
          wc.SetPixel(3, 0, 0, yd);

          wc.SetTLUEvent(GetTriggerID(ev));

          wcs.push_back(wc);
        }

        for (size_t i = 0; i<wcs.size(); i++) sev.AddPlane(wcs[i]);

      	//eudaq::mSleep(10);
	
      }
      

      
      std::cout<<"St Ev NumPlanes: "<<sev.NumPlanes()<<std::endl;

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
    // The constructor can be private, only one static instance is created
    // The DataConverterPlugin constructor must be passed the event type
    // in order to register this converter for the corresponding conversions
    // Member variables should also be initialized to default values here.
    WireChamberConverterPlugin()
        : DataConverterPlugin(EVENT_TYPE) {}

    // Information extracted in Initialize() can be stored here:


    // The single instance of this converter plugin
    static WireChamberConverterPlugin m_instance;
    }; // class WireChamberConverterPlugin

    // Instantiate the converter plugin instance
    WireChamberConverterPlugin WireChamberConverterPlugin::m_instance;

  } // namespace eudaq
