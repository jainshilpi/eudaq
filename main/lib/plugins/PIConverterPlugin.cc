#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"

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
    static const char *EVENT_TYPE = "PI";

    // Declare a new class that inherits from DataConverterPlugin
    class PIConverterPlugin : public DataConverterPlugin {

    public:
	// This is called once at the beginning of each run.
	// You may extract information from the BORE and/or configuration
	// and store it in member variables to use during the decoding later.
	virtual void Initialize(const Event &bore, const Configuration &cnf) {
	    m_pi_pos_chan1 = bore.GetTag("pi_pos_chan1", 0);
	    m_pi_pos_chan2 = bore.GetTag("pi_pos_chan2", 0);
	    m_pi_pos_chan3 = bore.GetTag("pi_pos_chan3", 0);
	    m_pi_pos_chan4 = bore.GetTag("pi_pos_chan4", 0);

	#ifndef WIN32 // some linux Stuff //$$change
	    (void)cnf; // just to suppress a warning about unused parameter cnf
	#endif
	}

	virtual bool GetStandardSubEvent(StandardEvent &sev,
					 const Event &ev) const {

	    // Read PI stage position
	    double pi_pos_chan1;
	    double pi_pos_chan2;
	    double pi_pos_chan3;
	    double pi_pos_chan4;

	    pi_pos_chan1 = ev.GetTag("pi_pos_chan1", 0);
	    pi_pos_chan2 = ev.GetTag("pi_pos_chan2", 0);
	    pi_pos_chan3 = ev.GetTag("pi_pos_chan3", 0);
	    pi_pos_chan4 = ev.GetTag("pi_pos_chan4", 0);

	    // Create "plane" of the PI stage position
	    std::string sensortype = "pistage";
	    // Create a StandardPlane representing one sensor plane
	    int id = 0;
	    StandardPlane plane(id, EVENT_TYPE, sensortype);
	    // Set the number of pixels >>> "max range of stage"
	    int width = 2000, height = 2000;
	    // preallocate 1 "pixel" - the position
	    plane.SetSizeZS(width, height, 1);
	    // Push positions a la pixel hits
	    plane.PushPixel(m_pi_pos_chan1,m_pi_pos_chan2, 1);
	    // Add the plane to the StandardEvent
	    sev.AddPlane(plane);

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
	PIConverterPlugin()
	    : DataConverterPlugin(EVENT_TYPE), m_pi_pos_chan1(0), m_pi_pos_chan2(0), m_pi_pos_chan3(0), m_pi_pos_chan4(0) {}

	// Information extracted in Initialize() can be stored here:
	double m_pi_pos_chan1;
	double m_pi_pos_chan2;
	double m_pi_pos_chan3;
	double m_pi_pos_chan4;

	// The single instance of this converter plugin
	static PIConverterPlugin m_instance;
    }; // class PIConverterPlugin

    // Instantiate the converter plugin instance
    PIConverterPlugin PIConverterPlugin::m_instance;

} // namespace eudaq
