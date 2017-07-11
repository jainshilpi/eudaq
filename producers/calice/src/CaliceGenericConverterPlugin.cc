// CaliceGenericConverterPlugin.cc
#include <string>
#include <iostream>
#include <typeinfo>  // for the std::bad_cast
#include <memory> // for std::unique_ptr

#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"

// All LCIO-specific parts are put in conditional compilation blocks
// so that the other parts may still be used if LCIO is not available.
#if USE_LCIO
#  include "IMPL/LCEventImpl.h"
#  include "IMPL/LCGenericObjectImpl.h"
#  include "IMPL/TrackerRawDataImpl.h"
#  include "IMPL/TrackerDataImpl.h"
#  include "IMPL/LCCollectionVec.h"
#  include "lcio.h"
#endif

using namespace eudaq;
using namespace std;
using namespace lcio;

namespace eudaq {

static const char* EVENT_TYPE = "CaliceObject";

#if USE_LCIO
// LCIO class
class CaliceLCGenericObject: public lcio::LCGenericObjectImpl {
   public:
      CaliceLCGenericObject()
            : lcio::LCGenericObjectImpl() {
         _typeName = EVENT_TYPE;
      }
      virtual ~CaliceLCGenericObject() {
      }

      void setTags(std::string &s) {
         _dataDescription = s;
      }
      std::string getTags() const {
         return _dataDescription;
      }

      void setIntDataInt(std::vector<int> &vec) {
         _intVec.resize(vec.size());
         std::copy(vec.begin(), vec.end(), _intVec.begin());
      }

      void setShortDataInt(std::vector<short> &vec) {
         _intVec.resize(vec.size());
         std::copy(vec.begin(), vec.end(), _intVec.begin());
      }

      const std::vector<int> & getDataInt() const {
         return _intVec;
      }
};

#endif

class CaliceGenericConverterPlugin: public DataConverterPlugin {

   public:
      virtual bool GetStandardSubEvent(StandardEvent & sev, const Event & ev) const
            {
         std::string sensortype = "Calice"; //TODO ?? "HBU"
         const int planeCount = 12;
         // Unpack data
         const RawDataEvent * rev = dynamic_cast<const RawDataEvent *>(&ev);
         //std::cout << "[Number of blocks] " << rev->NumBlocks() << std::endl;

         // 0 .. "EUDAQDataScCAL";
         // 1 .. "i:CycleNr,i:BunchXID,i:EvtNr,i:ChipID,i:NChannels,i:TDC14bit[NC],i:ADC14bit[NC]";
         // 2 .. unix time
         // 3 .. slowcontrol
         // 4 .. LED info
         // 5 .. Temperature
         // 6 .. Cycle info - start, stop, trigger
         // 7 .. AHCAL data

         //prepare 6 AHCAL layers
         std::vector<std::unique_ptr<StandardPlane>> planes;
         std::vector<int> HBUHits;
         std::vector<std::array<int, 144>> HBUs;         //HBU(aka plane) index, x*12+y
         for (int i = 0; i < planeCount; ++i) {
            std::array<int, 144> HBU;
            HBU.fill(-1); //fill all channels to -1
            HBUs.push_back(HBU); //add the HBU to the HBU
            HBUHits.push_back(0);
         }

         unsigned int nblock = 7; // the first 7 blocks contain other information
         //if (rev->NumBlocks() == 7) return false;
         std::cout << ev.GetEventNumber() << "<" << std::flush;

         while (nblock < rev->NumBlocks()) {         //iterate over all asic packets from (hopefully) same BXID
            vector<int> data;
            const RawDataEvent::data_t & bl = rev->GetBlock(nblock++);
            data.resize(bl.size() / sizeof(int));
            memcpy(&data[0], &bl[0], bl.size());
            if (data.size() != 77) cout << "vector has size : " << bl.size() << "\tdata : " << data.size() << endl;

            //data structure of packet: data[i]=
            //i=0 --> cycleNr
            //i=1 --> bunch crossing id
            //i=2 --> memcell or EvtNr (same thing, different naming)
            //i=3 --> ChipId
            //i=4 --> Nchannels per chip (normally 36)
            //i=5 to NC+4 -->  14 bits that contains TDC and hit/gainbit
            //i=NC+5 to NC+NC+4  -->  14 bits that contains ADC and again a copy of the hit/gainbit
            //debug prints:
            //std:cout << "Data_" << data[0] << "_" << data[1] << "_" << data[2] << "_" << data[3] << "_" << data[4] << "_" << data[5] << std::endl;
            if (data[1] == 0) continue; //don't store dummy trigger
            int chipid = data[3];
            int planeNumber = getPlaneNumberFromCHIPID(chipid);

            for (int ichan = 0; ichan < data[4]; ichan++) {
               int adc = data[5 + data[4] + ichan] & 0x0FFF; // extract adc
               int gainbit = (data[5 + data[4] + ichan] & 0x2000) ? 1 : 0; //extract gainbit
               int hitbit = (data[5 + data[4] + ichan] & 0x1000) ? 1 : 0;  //extract hitbit
               if (planeNumber >= 0) {  //plane, which is not found, has index -1
                  if (hitbit) {
                     int coordIndex = getXcoordFromCHIPID(chipid, ichan) * 12 + getYcoordFromCHIPID(chipid, ichan);  //get the index from the HBU array
                     if (HBUs[planeNumber][coordIndex] >= 0) std::cout << "ERROR: channel already has a value" << std::endl;
                     HBUs[planeNumber][coordIndex] = gainbit ? adc : 10 * adc;
                     //HBUs[planeNumber][coordIndex] = 1;
                     if (HBUs[planeNumber][coordIndex] < 0) HBUs[planeNumber][coordIndex] = 0;
                     HBUHits[planeNumber]++;
                  }
               }
            }
            std::cout << "." << std::flush;
         }
         for (int i = 0; i < HBUs.size(); ++i) {
            std::unique_ptr<StandardPlane> plane(new StandardPlane(i, EVENT_TYPE, sensortype));
            //plane->SetSizeRaw(13, 13, 1, 0);
            int pixindex = 0;
            plane->SetSizeZS(12, 12, HBUHits[i], 1, 0);
            for (int x = 0; x < 12; x++) {
               for (int y = 0; y < 12; y++) {
                  if (HBUs[i][x * 12 + y] >= 0) {
                     plane->SetPixel(pixindex++, x, y, HBUs[i][x * 12 + y]);
                  }
               }
            }

            if (ev.GetEventNumber() > 0) plane->SetTLUEvent(ev.GetEventNumber());
            //save planes with hits hits to the onlinedisplay

            sev.AddPlane(*plane);
            std::cout << ":" << std::flush;
         }
         std::cout << ">" << std::endl;
         return true;
      }

#if USE_LCIO
      virtual bool GetLCIOSubEvent(lcio::LCEvent &result, eudaq::Event const &source) const
            {
         //cout << "CaliceGenericConverterPlugin::GetLCIOSubEvent" << endl;
         // try to cast the Event
         eudaq::RawDataEvent const * rawev = 0;
         try {
            eudaq::RawDataEvent const & rawdataevent = dynamic_cast<eudaq::RawDataEvent const &>(source);
            rawev = &rawdataevent;
         } catch (std::bad_cast& e) {
            std::cout << e.what() << std::endl;
            return false;
         }
         // should check the type
         if (rawev->GetSubType() != EVENT_TYPE) {
            cout << "CaliceGenericConverter: type failed!" << endl;
            return false;
         }

         // no contents -ignore
         if (rawev->NumBlocks() < 5) return true;

         unsigned int nblock = 0;

         if (rawev->NumBlocks() > 2) {

            // check if the data is okay (checked at the producer level)
            int DAQquality = rawev->GetTag("DAQquality", 1);

            // first two blocks should be string, 3rd is time, time_usec
            auto& bl0 = rawev->GetBlock(nblock++);
            string colName((char *) &bl0.front(), bl0.size());

            auto& bl1 = rawev->GetBlock(nblock++);
            string dataDesc((char *) &bl1.front(), bl1.size());

            // EUDAQ TIMESTAMP, saved in ScReader.cc
            auto& bl2 = rawev->GetBlock(nblock++);
            //if(bl2.size()<8) {cout << "CaliceGenericConverter: time data too short!" << endl; return false;}
            time_t timestamp = *(unsigned int *) (&bl2[0]);
            //unsigned int timestamp_usec = *(int *)(&bl2[4]);

            //-------------------
            // READ/WRITE Temperature
            //fourth block is temperature (if colName ==  "EUDAQDataScCAL")
            //const RawDataEvent::data_t & bl4 = rawev->GetBlock(nblock++);

            if (colName == "EUDAQDataBIF") {
               //-------------------
               auto& bl3 = rawev->GetBlock(nblock++);
               if (bl3.size() > 0) cout << "Error, block 3 is filled in the BIF raw data" << endl;
               auto& bl4 = rawev->GetBlock(nblock++);
               if (bl4.size() > 0) cout << "Error, block 4 is filled in the BIF raw data" << endl;
               auto& bl5 = rawev->GetBlock(nblock++);
               if (bl5.size() > 0) cout << "Error, block 5 is filled in the BIF raw data" << endl;

               // READ BLOCKS WITH DATA
               LCCollectionVec *col = 0;
               col = createCollectionVec(result, colName, dataDesc, timestamp, DAQquality);
               getDataLCIOGenericObject(rawev, col, nblock);
            }

            if (colName == "EUDAQDataScCAL") {

               //-------------------
               // READ/WRITE SlowControl info
               //the  block=3, if non empty, contaions SlowControl info
               auto& bl3 = rawev->GetBlock(nblock++);

               if (bl3.size() > 0) {
                  cout << "Looking for SlowControl collection..." << endl;
                  LCCollectionVec *col = 0;
                  col = createCollectionVec(result, "SlowControl", "i:sc1,i:sc2,i:scN", timestamp, DAQquality);
                  getDataLCIOGenericObject(bl3, col, nblock);
               }

               // //-------------------
               // // READ/WRITE LED info
               // //the  block=4, if non empty, contaions LED info
               auto& bl4 = rawev->GetBlock(nblock++);
               if (bl4.size() > 0) {
                  cout << "Looking for LED voltages collection..." << endl;
                  LCCollectionVec *col = 0;
                  col = createCollectionVec(result, "LEDinfo", "i:Nlayers,i:LayerID_j,i:LayerVoltage_j,i:LayerOnOff_j", timestamp, DAQquality);
                  getDataLCIOGenericObject(bl4, col, nblock);
               }

               // //-------------------
               // // READ/WRITE Temperature info
               // //the  block=5, if non empty, contaions Temperature info
               auto& bl5 = rawev->GetBlock(nblock++);
               if (bl5.size() > 0) {
                  LCCollectionVec *col = 0;
                  col = createCollectionVec(result, "TempSensor", "i:LDA,i:port,i:T1,i:T2,i:T3,i:T4,i:T5,i:T6,i:TDIF,i:TPWR", timestamp, DAQquality);
                  getScCALTemperatureSubEvent(bl5, col);
               }

               // READ BLOCKS WITH DATA
               nblock++;
               LCCollectionVec *col = 0;
               col = createCollectionVec(result, colName, dataDesc, timestamp, DAQquality);
               getDataLCIOGenericObject(rawev, col, nblock);

               // //-------------------
               // // READ/WRITE Timestamps
               // //the  block=6, if non empty,
               auto& bl6 = rawev->GetBlock(6);
               if (bl6.size() == 0) cout << "Nothing in Timestamps collection..." << endl;
               if (bl6.size() > 0) {
                  cout << "Looking for Timestamps collection..." << endl;
                  LCCollectionVec *col = 0;
                  col = createCollectionVec(result, "TimeStamps", "Start:Stop:BusyOn:BusyOff:Trig", timestamp, DAQquality);
                  getDataLCIOGenericObject(bl6, col, nblock);
                  int triggerNumber = rawev->GetTag("TriggerN", -1);
                  if (triggerNumber >= 0) {
                     col->parameters().setValue("TriggerN", triggerNumber);
                  }
               }

            }

         }
         return true;
      }

      virtual LCCollectionVec* createCollectionVec(lcio::LCEvent &result, string colName, string dataDesc, time_t timestamp, int DAQquality) const {
         LCCollectionVec *col = 0;
         try {
            // looking for existing collection
            col = dynamic_cast<IMPL::LCCollectionVec *>(result.getCollection(colName));
         } catch (DataNotAvailableException &e) {
            // create new collection
            col = new IMPL::LCCollectionVec(LCIO::LCGENERICOBJECT);
            result.addCollection(col, colName);
         }
         col->parameters().setValue("DataDescription", dataDesc);
         //add timestamp (set by the Producer, is EUDAQ, not real timestamp!!)
         struct tm *tms = localtime(&timestamp);
         char tmc[256];
         strftime(tmc, 256, "%a, %d %b %Y %T %z", tms);
         col->parameters().setValue("Timestamp", tmc);
         col->parameters().setValue("DAQquality", DAQquality);

         return col;
      }

      virtual void getScCALTemperatureSubEvent(const std::vector<uint8_t>& bl, LCCollectionVec *col) const {

         // sensor specific data
         cout << "Looking for Temperature Collection... " << endl;
         vector<int> vec;
         vec.resize(bl.size() / sizeof(int));
         memcpy(&vec[0], &bl[0], bl.size());

         vector<int> output;
         int lda = -1;
         int port = 0;
         for (unsigned int i = 0; i < vec.size() - 2; i += 3) {
            if ((i / 3) % 2 == 0) continue; // just ignore the first data;
            if (output.size() != 0 && port != vec[i + 1]) {
               cout << "Different port number comes before getting 8 data!." << endl;
               break;
            }
            port = vec[i + 1]; // port number
            int data = vec[i + 2]; // data
            if (output.size() == 0) {
               if (port == 0) lda++;
               output.push_back(lda);
               output.push_back(port);
            }
            output.push_back(data);

            if (output.size() == 10) {
               CaliceLCGenericObject *obj = new CaliceLCGenericObject;
               obj->setIntDataInt(output);
               try {
                  col->addElement(obj);
               } catch (ReadOnlyException &e) {
                  cout << "CaliceGenericConverterPlugin: the collection to add is read only! skipped..." << endl;
                  delete obj;
               }
               output.clear();
            }
         }
      }

      virtual void getDataLCIOGenericObject(const std::vector<uint8_t> & bl, LCCollectionVec *col, int nblock) const {

         // further blocks should be data (currently limited to integer)

         vector<int> v;
         v.resize(bl.size() / sizeof(int));
         memcpy(&v[0], &bl[0], bl.size());

         CaliceLCGenericObject *obj = new CaliceLCGenericObject;
         obj->setIntDataInt(v);
         try {
            col->addElement(obj);
         } catch (ReadOnlyException &e) {
            cout << "CaliceGenericConverterPlugin: the collection to add is read only! skipped..." << endl;
            delete obj;
         }

      }

      virtual void getDataLCIOGenericObject(eudaq::RawDataEvent const * rawev, LCCollectionVec *col, int nblock, int nblock_max = 0) const {

         if (nblock_max == 0) nblock_max = rawev->NumBlocks();
         while (nblock < nblock_max) {
            // further blocks should be data (currently limited to integer)

            vector<int> v;
            auto& bl = rawev->GetBlock(nblock++);
            v.resize(bl.size() / sizeof(int));
            memcpy(&v[0], &bl[0], bl.size());

            CaliceLCGenericObject *obj = new CaliceLCGenericObject;
            obj->setIntDataInt(v);
            try {
               col->addElement(obj);
            } catch (ReadOnlyException &e) {
               cout << "CaliceGenericConverterPlugin: the collection to add is read only! skipped..." << endl;
               delete obj;
            }
         }
      }
//      virtual void getDataLCIOGenericObject(eudaq::RawDataEvent const * rawev, LCCollectionVec *col, int nblock) const {
//
//         while(nblock < rawev->NumBlocks()) {
//            // further blocks should be data (currently limited to integer)
//            vector<short> v;
//            const RawDataEvent::data_t & bl5 = rawev->GetBlock(nblock++);
//            v.resize(bl5.size() / sizeof(short));
//            memcpy(&v[0], &bl5[0],bl5.size());
//
//            CaliceLCGenericObject *obj = new CaliceLCGenericObject;
//            obj->setDataInt(v);
//            try {
//               col->addElement(obj);
//            } catch(ReadOnlyException &e) {
//               cout << "CaliceGenericConverterPlugin: the collection to add is read only! skipped..." << endl;
//               delete obj;
//            }
//         }
//      }

#endif

   private:
      CaliceGenericConverterPlugin()
            : DataConverterPlugin(EVENT_TYPE)
      {
      }

      int getPlaneNumberFromCHIPID(int chipid) const {
         int planeNumber = -1;
         switch ((chipid - 1) >> 2) {
            case 59: //237
               planeNumber = 0;
               break;
            case 60: //241
               planeNumber = 1;
               break;
            case 61: //245
               planeNumber = 2;
               break;
            case 30: //121
               planeNumber = 3;
               break;
            case 29: //117
               planeNumber = 4;
               break;
            case 62: //249
               planeNumber = 5;
               break;
            case 56: //225
               planeNumber = 6;
               break;
            case 54: //217
               planeNumber = 7;
               break;
            case 53: //213
               planeNumber = 8;
               break;
            case 51: //205
               planeNumber = 9;
               break;
            case 55: //221
               planeNumber = 10;
               break;
            case 50: //201
               planeNumber = 11;
               break;
            default:
               break;
         }
         return planeNumber;
      }

      int getXcoordFromCHIPID(int chipid, int channelNr) const {
         int subx = channelNr / 6;
         if (((chipid & 0x03) == 0x01) || ((chipid & 0x03) == 0x02)) {
            //1st and 2nd spiroc are in the righ half of HBU
            subx += 6;
         }
         return subx;
      }

      int getYcoordFromCHIPID(int chipid, int channelNr) const {
         int suby = channelNr % 6;
         if (((chipid & 0x03) == 0x00) || ((chipid & 0x03) == 0x03)) {
            //3rd and 4th spiroc have different channel order
            suby = 5 - suby;
         }
         if (((chipid & 0x03) == 0x01) || ((chipid & 0x03) == 0x03)) {
            suby += 6;
         }
         return suby;
      }

      static CaliceGenericConverterPlugin m_instance;

};

// Instantiate the converter plugin instance
CaliceGenericConverterPlugin CaliceGenericConverterPlugin::m_instance;

}

