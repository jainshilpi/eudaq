#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"

#include <bitset>
#include <boost/format.hpp>

#include "CAENv1742Unpacker.h"

// All LCIO-specific parts are put in conditional compilation blocks
// so that the other parts may still be used if LCIO is not available.
#if USE_LCIO
#include "IMPL/LCEventImpl.h"
#include "IMPL/TrackerRawDataImpl.h"
#include "IMPL/LCCollectionVec.h"
#include "lcio.h"
#endif

//must be adjusted to each setup of the digitizer
#define Nchannels 9
#define Ngroups 4
#define Nsamples 1024


namespace eudaq {

  // The event type for which this converter plugin will be registered
  // Modify this to match your actual event type (from the Producer)
  static const char *EVENT_TYPE = "DIGITIZER";

  // Declare a new class that inherits from DataConverterPlugin
  class MCPConverterPlugin : public DataConverterPlugin {

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
      const std::string sensortype = "DIGITIZER";

      //std::cout<<"\t Dans GetStandardSubEvent()  "<<std::endl;

      const RawDataEvent * rev = dynamic_cast<const RawDataEvent *> ( &ev );

      std::cout<<"time stamp: "<<rev->GetTimestamp()<<std::endl;
      sev.SetTag("cpuTime_mcpProducer_mus", rev->GetTimestamp());

      const unsigned nBlocks = rev->NumBlocks();
      if (nBlocks>1) {
        std::cout<<"More than one blocks in the MCPConverter, stop!!!"<<std::endl;
        return false;
      }
      
      std::vector<digiData> converted_samples;      
      unsigned int digitizer_triggerTimeTag;
      for (unsigned digititzer_index=0; digititzer_index<nBlocks; digititzer_index++){
              
        const RawDataEvent::data_t & bl = rev->GetBlock(digititzer_index);

        //conversion block
        std::vector<uint32_t> Words;
        Words.resize(bl.size() / sizeof(uint32_t));
        std::memcpy(&Words[0], &bl[0], bl.size());   

        unpacker->Unpack(Words, converted_samples, digitizer_triggerTimeTag);
        
      }
      sev.SetTag("digitizer_triggertimetag", digitizer_triggerTimeTag);

      StandardPlane digitizer_full_plane(0, EVENT_TYPE, sensortype);
      digitizer_full_plane.SetSizeRaw(Ngroups, Nchannels, Nsamples+1);    
      for (int gr=0; gr<Ngroups; gr++) for (int ch=0; ch<Nchannels; ch++) digitizer_full_plane.SetPixel(gr*Nchannels+ch, gr, ch, Nsamples, false, 0);

      //plane for offline reconstruction
      int gr, ch, sample_index;
      for (int i=0; i<converted_samples.size(); i++) {
        gr = converted_samples[i].group;
        ch = converted_samples[i].channel;
        sample_index = converted_samples[i].sampleIndex;
        digitizer_full_plane.SetPixel(gr*Nchannels+ch, gr, ch, converted_samples[i].sampleValue, false, 1+sample_index);
      }

      sev.AddPlane(digitizer_full_plane);

      //todo:
      
      //2. make DQM plots (2 per channel: accumulation + last event % 100)

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
    CAEN_V1742_Unpacker* unpacker;

    MCPConverterPlugin()
        : DataConverterPlugin(EVENT_TYPE) {
          unpacker = new CAEN_V1742_Unpacker;
        }

    // Information extracted in Initialize() can be stored here:
    ~MCPConverterPlugin() {
      
    }

    // The single instance of this converter plugin
    static MCPConverterPlugin m_instance;
    }; // class MCPConverterPlugin

    // Instantiate the converter plugin instance
    MCPConverterPlugin MCPConverterPlugin::m_instance;

  } // namespace eudaq
