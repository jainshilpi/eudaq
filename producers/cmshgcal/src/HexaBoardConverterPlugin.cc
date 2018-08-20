#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Configuration.hh"

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

#include "../include/compressor.h"

//Median computation: https://stackoverflow.com/questions/2114797/compute-median-of-values-stored-in-vector-c
float CalcMHWScore(std::vector<float> scores)
{
	size_t size = scores.size();

	if (size == 0)
	{
		return 0;  // Undefined, really.
	}
	else
	{
		sort(scores.begin(), scores.end());
		if (size % 2 == 0)
		{
			return (scores[size / 2 - 1] + scores[size / 2]) / 2;
		}
		else
		{
			return scores[size / 2];
		}
	}
}



const size_t RAW_EV_SIZE_32 = 123160;
const int nSCA = 13;

// Size of ZS data (per channel)
const char hitSizeZS = 33;

const int thresh_HGTS3over0 = 20;
const int thresh_HGTS3over5 = 20;

std::vector<int> modules_per_board = {0, 6, 14, 20}; //hard coded number of boards per readout board

namespace eudaq {

// The event type for which this converter plugin will be registered
// Modify this to match your actual event type (from the Producer)
//static const char *EVENT_TYPE = "RPI";
static const char *EVENT_TYPE = "HexaBoard";

// Declare a new class that inherits from DataConverterPlugin
class HexaBoardConverterPlugin : public DataConverterPlugin {

public:
	// This is called once at the beginning of each run.
	// You may extract information from the BORE and/or configuration
	// and store it in member variables to use during the decoding later.
	virtual void Initialize(const Event &bore, const Configuration &cnf) {

#ifndef WIN32 // some linux Stuff //$$change
		(void)cnf; // just to suppress a warning about unused parameter cnf
#endif


		std::cout << "Config file name in HexaBoard Converter: " << cnf.Name() << std::endl;

		cnf.SetSection("RunControl");
		cnf.Print();

		cnf.SetSection("Producer.CMS-HGCAL");
		m_compressed = cnf.Get("DoCompression", false);

		cnf.SetSection("CMSHGCAL-OnlineMon");
		cnf.Print();

		m_runMode = cnf.Get("runMode", 1);

		m_skiMask[0] = std::stoul(cnf.Get("Mask_RDB1", "0xFFFFFFFF"), nullptr, 16);
		m_skiMask[1] = std::stoul(cnf.Get("Mask_RDB2", "0xFFFFFFFF"), nullptr, 16);
		m_skiMask[2] = std::stoul(cnf.Get("Mask_RDB3", "0xFFFFFFFF"), nullptr, 16);
		m_skiMask[3] = std::stoul(cnf.Get("Mask_RDB4", "0xFFFFFFFF"), nullptr, 16);
		m_skiMask[4] = std::stoul(cnf.Get("Mask_RDB4", "0xFFFFFFFF"), nullptr, 16);


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
				return getlittleendian<unsigned>( &rev->GetBlock(0)[TRIGGER_OFFSET]);
			}
		}
		// If we are unable to extract the Trigger ID, signal with (unsigned)-1
		return (unsigned) - 1;
	}

	// Here, the data from the RawDataEvent is extracted into a StandardEvent.
	// The return value indicates whether the conversion was successful.
	// Again, this is just an example, adapted it for the actual data layout.
	virtual bool GetStandardSubEvent(StandardEvent &sev, const Event &ev) const {

		// If the event type is used for different sensors
		// they can be differentiated here
		const std::string sensortype = "HexaBoard";

		// std::cout<<"\t Dans GetStandardSubEvent()  "<<std::endl;

		const RawDataEvent * rev = dynamic_cast<const RawDataEvent *> ( &ev );

		//rev->Print(std::cout);

		const unsigned nBlocks = rev->NumBlocks();
		// std::cout<<"Number of Raw Data Blocks: "<<nBlocks<<std::endl;

		int RDBOARD = 0;
		//reconstructed quantities to be computed across all readout boards
		unsigned nhits = 0; float energy_sum = 0, depth = 0;

		for (unsigned blo = 0; blo < nBlocks; blo++) {

			std::cout << "Hexa Block = " << blo << "  Raw GetID = " << rev->GetID(blo) << std::endl;

			const RawDataEvent::data_t & bl = rev->GetBlock(blo);

			std::cout << "size of block: " << bl.size() << std::endl;

			if (blo % 2 == 0) {
				// This block contains a string with
				std::vector<int> brdID;
				//brdID.push_back(0); // Temporary set the id for some debugging to work
				brdID.resize(bl.size() / sizeof(int));
				std::memcpy(&brdID[0], &bl[0], bl.size());
				RDBOARD = brdID[0];
				// std::cout<<"RDBRD ID = "<<RDBOARD<<std::endl;

				//const unsigned nPlanes = nSkiPerBoard[RDBOARD-1]/4;
				//std::cout<<"Number of Planes: "<<nPlanes<<std::endl;

				continue;
			}

			else {

				// This block contains the data
				// std::cout<<" We are in the data block. blo="<<blo<<std::endl;

				// We will put the data in this vector:
				// ----------------->>>>>>
				std::vector<uint32_t> rawData32;
				// <<<<<<<-----------------

				if (bl.size() != RAW_EV_SIZE_32 && !m_compressed) {
					EUDAQ_WARN("There is something wrong with the data. Size= " + eudaq::to_string(bl.size()));
					std::cout << " Bad data size in block " << blo << "   size: " << bl.size() << std::endl;
					return true;
				}
				else if (bl.size() != RAW_EV_SIZE_32 && m_compressed) {
					// std::cout<<" This data are compressed! Data size in block "<<blo<<"   size: "<<bl.size()<<std::endl;
					// Let's decompress it!
					//std::string decompressed=decompressor::decompress(bl); // Taking the function from this class
					std::string decompressed = compressor::decompress(bl); // Taking the function from compressor.h

					const std::vector<uint8_t> decompData( decompressed.begin(), decompressed.end() );
					rawData32.resize(decompData.size() / sizeof(uint32_t));
					std::memcpy(&rawData32[0], &decompData[0], decompData.size());

					if (rawData32.size() * 4 != RAW_EV_SIZE_32) {
						std::cout << "Size of raw data after decompression is wong! size = " << rawData32.size() * 4 << " bytes" << std::endl;
						return true;
					}

				}
				else if (bl.size() == RAW_EV_SIZE_32 && m_compressed) {
					std::cout << "Hmmm. You says that the data are compressed, but the data size says otherwisse..\n"
					          << "Who is right? Machine is always right. Human is wrong.\n"
					          << "Data size in block " << blo << "   size: " << bl.size() << std::endl;
					return true;
				}
				else if (bl.size() == RAW_EV_SIZE_32 && !m_compressed) {
					// This is the simplest case, data are not compressed. Just mem-copy them:
					rawData32.resize(bl.size() / sizeof(uint32_t));
					std::memcpy(&rawData32[0], &bl[0], bl.size());
				}


				const std::vector<std::array<unsigned int, 1924>> decoded = decode_raw_32bit(rawData32, RDBOARD);

				// Here we parse the data per hexaboard and per ski roc and only leave meaningful data (Zero suppress and finding main frame):
				const std::vector<std::vector<unsigned>> dataBlockZS = GetZSdata(decoded, RDBOARD);
				//std::cout << "DBG 4444" << std::endl;

				unsigned dataBlockSize = dataBlockZS.size();

				for (unsigned h = 0; h < dataBlockSize - 1; h++) {		//the last index contains reconstructed quantities
					// std::cout<<"Hexa plane = "<<h<<std::endl;
					std::ostringstream oss;
					oss << sensortype << "-RDB" << RDBOARD;
					StandardPlane plane(h, EVENT_TYPE, oss.str());
					//StandardPlane plane(blo*8+h, EVENT_TYPE, sensortype);

					// -------------
					// Now we can use the data to fill the euDAQ Planes
					// -------------
					const unsigned nHits  = dataBlockZS[h].size() / hitSizeZS;

					//std::cout<<"Number of Hits (above ZS threshold): "<<nHits<<std::endl;

					plane.SetSizeZS(4, 64, nHits, hitSizeZS - 1);

					for (int ind = 0; ind < nHits; ind++) {

						const unsigned skiID_1 = dataBlockZS[h][hitSizeZS * ind] / 100;
						// This should give us a number between 0 and 3:
						const unsigned skiID_2 = 3 - (skiID_1) % 4;

						if (skiID_2 > 3) {
							std::cout << " ERROR in ski. It is " << skiID_2 << std::endl;
							EUDAQ_WARN("There is another error with encoding. ski=" + eudaq::to_string(skiID_2));
						}
						else {
							const unsigned pix = dataBlockZS[h][hitSizeZS * ind] - skiID_1 * 100;

							//if (skiID==0 && pix==0)
							//std::cout<<" Zero-Zero problem. ind = "<<ind<<"  ID:"<<data[hitSizeZS*ind]<<std::endl;

							for (int ts = 0; ts < hitSizeZS - 1; ts++)
								plane.SetPixel(ind, skiID_2, pix, dataBlockZS[h][hitSizeZS * ind + 1 + ts], false, ts);
						}

					}
					// Set the trigger ID
					plane.SetTLUEvent(ev.GetEventNumber());
					// Add the plane to the StandardEvent
					sev.AddPlane(plane);
					//eudaq::mSleep(10);
				}


				unsigned _nhits; float _energy_sum, _depth;
				std::memcpy(&_nhits, &dataBlockZS[dataBlockSize - 1][0], sizeof(unsigned)) ;
				std::memcpy(&_energy_sum, &dataBlockZS[dataBlockSize - 1][1], sizeof(unsigned)) ;
				std::memcpy(&_depth, &dataBlockZS[dataBlockSize - 1][2], sizeof(unsigned)) ;
				std::cout<<"RDBOARD"<<RDBOARD<<" hits: "<<_nhits<<"   energy: "<<_energy_sum<<"   _depth: "<<_depth<<std::endl;

				nhits += _nhits;
				energy_sum += _energy_sum;
				depth += _depth + modules_per_board[RDBOARD] * _energy_sum;
			}

		}

		if (energy_sum > 0) depth /= energy_sum; else depth = -1;

		//add the plane with reconstructed quantities
		StandardPlane analysis_plane(0, EVENT_TYPE, "HGCalOnlineAnalysis");
		analysis_plane.SetSizeRaw(3, 1, 1);

		std::cout << "(READ) Number of hits: " << nhits << "  energy: " << energy_sum << "   depth: " << depth << std::endl;
		analysis_plane.SetPixel(0, 0, 0, nhits);
		analysis_plane.SetPixel(1, 0, 0, energy_sum);
		analysis_plane.SetPixel(2, 0, 0, depth);
		analysis_plane.SetTLUEvent(ev.GetEventNumber());
		sev.AddPlane(analysis_plane);

		std::cout << "Hexa St Ev NumPlanes: " << sev.NumPlanes() << std::endl;

		// Indicate that data was successfully converted
		return true;
	}

	unsigned int gray_to_brady(unsigned int gray) const {
		// Code from:
		//https://github.com/CMS-HGCAL/TestBeam/blob/826083b3bbc0d9d78b7d706198a9aee6b9711210/RawToDigi/plugins/HGCalTBRawToDigi.cc#L154-L170
		unsigned int result = gray & (1 << 11);
		result |= (gray ^ (result >> 1)) & (1 << 10);
		result |= (gray ^ (result >> 1)) & (1 << 9);
		result |= (gray ^ (result >> 1)) & (1 << 8);
		result |= (gray ^ (result >> 1)) & (1 << 7);
		result |= (gray ^ (result >> 1)) & (1 << 6);
		result |= (gray ^ (result >> 1)) & (1 << 5);
		result |= (gray ^ (result >> 1)) & (1 << 4);
		result |= (gray ^ (result >> 1)) & (1 << 3);
		result |= (gray ^ (result >> 1)) & (1 << 2);
		result |= (gray ^ (result >> 1)) & (1 << 1);
		result |= (gray ^ (result >> 1)) & (1 << 0);
		return result;
	}


	std::vector<std::array<unsigned int, 1924>> decode_raw_32bit(std::vector<uint32_t>& raw, const int board_id) const {

		//const uint32_t ch_mask = m_skiMask[board_id-1];
		const uint32_t ch_mask = raw[0];

		//std::cout<<"In decoder"<<std::endl;
		//printf("\t SkiMask: 0x%08x;   Length of Raw: %d\n", ch_mask, raw.size());

		// Check that an external mask agrees with first 32-bit word in data
		if (ch_mask != m_skiMask[board_id - 1])
			EUDAQ_WARN("You extarnal mask (" + eudaq::to_hex(m_skiMask[board_id - 1]) + ") does not agree with the one found in data (" + eudaq::to_hex(raw[0]) + ")");



		// First, we need to determine how many skiRoc data is presnt

		const std::bitset<32> ski_mask(ch_mask);
		//std::cout<<"ski mask: "<<ski_mask<<std::endl;

		const int mask_count = ski_mask.count();
		//if (mask_count!= nSkiPerBoard[board_id-1]) {
		//EUDAQ_WARN("The mask does not agree with expected number of SkiRocs. Mask count:"+ eudaq::to_string(mask_count));
		//}

		std::vector<std::array<unsigned int, 1924>> ev(mask_count, std::array<unsigned int, 1924>());


		uint32_t x;
		const int offset = 1; // Due to FF or other things in data head
		for (int  i = 0; i < 1924; i++) {
			for (int j = 0; j < 16; j++) {
				x = raw[offset + i * 16 + j];
				//x = x & ch_mask; // <-- This is probably not needed
				int k = 0;
				for (int fifo = 0; fifo < 32; fifo++) {
					if (ch_mask & (1 << fifo)) {
						ev[k][i] = ev[k][i] | (unsigned int) (((x >> fifo ) & 1) << (15 - j));
						k++;
					}
				}
			}
		}

		return ev;

	}

	char GetRollMaskEnd(const unsigned int r) const {
		//printf("Roll mask = %d \n", r);
		int k1 = -1, k2 = -1;
		for (int p = 0; p < nSCA; p++) {
			//printf("pos = %d, %d \n", p, r & (1<<12-p));
			if (r & (1 << p)) {
				if (k1 == -1)
					k1 = 12 - p;
				else if (k2 == -1)
					k2 = 12 - p;
				else
					EUDAQ_WARN("Error: more than two positions in roll mask! RollMask=" + eudaq::to_hex(r, 8));
			}
		}

		//printf("ROLL MASK:  k1 = %d, k2 = %d \n", k1, k2);

		// Check that k1 and k2 are consecutive
		char last = -1;
		if (k1 == 12 && k2 == 0) { last = 0;}

		//else if (abs(k1-k2)>1)
		//EUDAQ_WARN("The k1 and k2 are not consecutive! abs(k1-k2) = "+ eudaq::to_string(abs(k1-k2)) +" rollMask="+eudaq::to_hex(r,8)+ " k1="+eudaq::to_string(k1)+"  k2="+eudaq::to_string(k2));
		//printf("The k1 and k2 are not consecutive! abs(k1-k2) = %d\n", abs(k1-k2));
		else
			last = k1;

		//printf("last = %d\n", last);

		return last;
	}

	std::vector<std::vector<unsigned>> GetZSdata(const std::vector<std::array<unsigned int, 1924>> &decoded, const int board_id) const {

		// std::cout<<"In GetZSdata() method"<<std::endl;

		const int nSki  =  decoded.size();
		const int nHexa =  nSki / 4;
		if (nSki % 4 != 0)
			EUDAQ_WARN("Number of SkiRocs is not right: " + eudaq::to_string(nSki));

		std::bitset<32> skibits(m_skiMask[board_id - 1]);
		if (nHexa != skibits.count() / 4)
			EUDAQ_WARN("Number of HexaBoards is not right: " + eudaq::to_string(nHexa));

		// A vector per HexaBoard of vector of Hits from 4 ski-rocs
		std::vector<std::vector<unsigned>> dataBlockZS(nHexa + 1);

		//read data per channel
		std::vector<float> hg_0;
		std::vector<float> hg_2;
		std::vector<float> hg_3;
		std::vector<float> hg_2_pedestal_subtracted;
		std::vector<float> hg_3_pedestal_subtracted;
		std::vector<float> lg_0;
		std::vector<float> lg_2;
		std::vector<float> lg_3;
		std::vector<float> lg_2_pedestal_subtracted;
		std::vector<float> lg_3_pedestal_subtracted;
		std::vector<float> tot;
		int LG_TS3, LG_TS2, LG_TS0;
		int HG_TS3, HG_TS2, HG_TS0;


		float energy = 0;

		float LG_pedestal;
		float HG_pedestal;

		//reconstructed quantities per board
		float energy_sum_perBoard = 0;

		unsigned TOT = 0;
		unsigned toa_sum_perBoard = 0;
		unsigned n_toa_fired_perBoard = 0;
		unsigned lg_sum_perBoard = 0;

		//global reconstructed quantities
		float energy_sum = 0;
		unsigned int nhits = 0;
		float depth = 0;

		for (int ski = 0; ski < nSki; ski++ ) {
			const int hexa = ski / 4;

			//std::cout<<"Chip "<<ski<<" RollMask is "<<std::hex<<decoded[ski][1920]<<std::endl;

			// ----------
			// -- Based on the rollmask, lets determine which time-slices (frames) to add
			//
			const unsigned int r = decoded[ski][1920];

			// This is the position of "track" in TS array:
			const int TS0 = (GetRollMaskEnd(r) + 1) % 13;

			//const int mainFrame = (TS0+3)%13;

			hg_0.clear();
			lg_0.clear();



			for (int ch = 0; ch < 64; ch += 2) {

				const int chArrPos = 63 - ch; // position of the hit in array

				// This channel is not connected. Notice that ski-roc numbering here is reverted by (3-ski) relation.
				// (Ie, the actual disconnected channel is (1,60), but in this numbering it's (2.60))
				// (UPD 09 June 2018: this is no longer applicable for V3 modules.)
				//if (ski==2 && ch==60) continue;

				unsigned adc = 0;

				// Signal-like selection based on HG
				//ts = 12-(TS0+ts)%13
				const int TS3 = 12 - (TS0 + 3) % 13;
				const int TS5 = 12 - (TS0 + 5) % 13;
				const int TS7 = 12 - (TS0 + 7) % 13;

				int hg_TS0 = gray_to_brady(decoded[ski][TS0 * 128 + 64 + chArrPos] & 0x0FFF);
				int hg_TS3 = gray_to_brady(decoded[ski][TS3 * 128 + 64 + chArrPos] & 0x0FFF);
				int hg_TS5 = gray_to_brady(decoded[ski][TS5 * 128 + 64 + chArrPos] & 0x0FFF);
				int hg_TS7 = gray_to_brady(decoded[ski][TS7 * 128 + 64 + chArrPos] & 0x0FFF);

				int hg_TS3over0 = hg_TS3 - hg_TS0;
				int hg_TS3over5 = hg_TS3 - hg_TS5;

				//adc = gray_to_brady(decoded[ski][mainFrame*128 + chArrPos] & 0x0FFF);
				//if (adc==0) adc=4096;

				//const int chargeLG = adc;

				//std::cout<<ch <<": charge LG:"<<chargeLG
				//<<"ped + noi = "<<ped+noi<<"   charge-(ped+noi) = "<<chargeLG - (ped+noi)<<std::endl;

				// Check that all Ha hits are the same for a given channel
				if ((decoded[ski][chArrPos] & 0x1000) != (decoded[ski][chArrPos + 64] & 0x1000))
					std::cout << "Warning: HA is not what we think it is..." << std::endl;

				if (m_runMode == 0)
					; // Pass. This is pedestal mode, no zero suppression is needed
				else if (m_runMode == 1) {
					//std::cout<<"DBG ski="<<ski<<"  ch pos = "<<chArrPos<<"  HA bit: "<<(decoded[ski][chArrPos] & 0x1000)<<std::endl;

					// ZeroSuppress it:
					//if (chargeHG_avg_in3TS < thresh)  // - Based on ADC in LG/HG
					if (! (decoded[ski][chArrPos] & 0x1000)) // - Based on HA bit (TOA hit)
						continue;
				}
				else if (m_runMode == 2) {
					// For the last hexaboard in the last readout board (4-7) always do a TOA selection
					if ( board_id == 4 && hexa == 7)
						if (! (decoded[ski][chArrPos] & 0x1000))
							continue;
					// For the rest of them, do MIP selection
					if (! (hg_TS3over0 > thresh_HGTS3over0
					        && hg_TS3over5 > thresh_HGTS3over5
					        && hg_TS7 - hg_TS0 < 0
					      ) )
						continue;
				}
				else
					EUDAQ_WARN("This run-mode is not yet implemented: " + eudaq::to_string(m_runMode));


				dataBlockZS[hexa].push_back((ski % 4) * 100 + ch);

				// Low gain (save nSCA time-slices after track):
				//std::cout<<"\t Roll positions:"<<std::endl;



				for (int ts = 0; ts < nSCA; ts++) {
					const int t = 12 - (TS0 + ts) % 13; // subtract from 12 (nSCA-1) because the data in SkiRock memory is saved backwards
					adc = gray_to_brady(decoded[ski][t * 128 + chArrPos] & 0x0FFF);
					if (adc == 0) adc = 4096;
					dataBlockZS[hexa].push_back(adc);

					if (t == 0) LG_TS0 = adc;
					else if (t == 2) LG_TS2 = adc;
					else if (t == 3) LG_TS3 = adc;

				}

				if (LG_TS3 - LG_TS0 > 4)		//5 LG ADC ~ 1 MIP
					lg_sum_perBoard += LG_TS3 - LG_TS0;

				//std::cout<<std::endl;



				// High gain:
				for (int ts = 0; ts < nSCA; ts++) {
					const int t = 12 - (TS0 + ts) % 13;
					adc = gray_to_brady(decoded[ski][t * 128 + 64 + chArrPos] & 0x0FFF);
					if (adc == 0) adc = 4096;
					dataBlockZS[hexa].push_back(adc);

					if (t == 0) HG_TS0 = adc;
					else if (t == 2) HG_TS2 = adc;
					else if (t == 3) HG_TS3 = adc;
				}

				// Filling TOA (stop falling clock)
				adc = gray_to_brady(decoded[ski][1664 + chArrPos] & 0x0FFF);
				if (adc == 0) adc = 4096;
				dataBlockZS[hexa].push_back(adc);

				// Summing all TOA values of the pixels which fire it (to do average later)
				if (adc != 4) {
					toa_sum_perBoard += adc;
					n_toa_fired_perBoard += 1;
				}
				// Filling TOA (stop rising clock)
				adc = gray_to_brady(decoded[ski][1664 + 64 + chArrPos] & 0x0FFF);
				if (adc == 0) adc = 4096;
				dataBlockZS[hexa].push_back(adc);

				// Filling TOT (fast)
				adc = gray_to_brady(decoded[ski][1664 + 2 * 64 + chArrPos] & 0x0FFF);
				if (adc == 0) adc = 4096;
				dataBlockZS[hexa].push_back(adc);

				// Filling TOT (slow)
				adc = gray_to_brady(decoded[ski][1664 + 3 * 64 + chArrPos] & 0x0FFF);
				if (adc == 0) adc = 4096;
				dataBlockZS[hexa].push_back(adc);
				TOT = adc;		//TOT = TOT slow

				// This frame is reserved to hold per-module variables.
				// (which are set outside of this loop):
				dataBlockZS[hexa].push_back(0);		//average toa
				dataBlockZS[hexa].push_back(0);		//low gain sum


				hg_0.push_back(HG_TS0);
				hg_2.push_back(HG_TS2);
				hg_3.push_back(HG_TS3);
				lg_0.push_back(LG_TS0);
				lg_2.push_back(LG_TS2);
				lg_3.push_back(LG_TS3);
				tot.push_back(TOT);


			}	//outside the channel loop

			//perform the pedesal subtraction for a fixed skiroc
			float average_hg0 = accumulate( hg_0.begin(), hg_0.end(), 0.0) / hg_0.size();
			for (size_t i = 0; i < hg_0.size(); i++) hg_2_pedestal_subtracted.push_back(hg_2[i] - average_hg0);
			for (size_t i = 0; i < hg_0.size(); i++) hg_3_pedestal_subtracted.push_back(hg_3[i] - average_hg0);
			hg_0.clear(); hg_2.clear(); hg_3.clear();
			float average_lg0 = accumulate( lg_0.begin(), lg_0.end(), 0.0) / lg_0.size();
			for (size_t i = 0; i < lg_0.size(); i++) lg_2_pedestal_subtracted.push_back(lg_2[i] - average_lg0);
			for (size_t i = 0; i < lg_0.size(); i++) lg_3_pedestal_subtracted.push_back(lg_3[i] - average_lg0);
			lg_0.clear(); lg_2.clear(); lg_3.clear();


			if (ski % 4 == 3 && dataBlockZS[hexa].size() != 0) {
				if (toa_sum_perBoard > 0 && n_toa_fired_perBoard > 0) {
					// Put here the average TOA value of the hexaboard
					dataBlockZS[hexa][31] = toa_sum_perBoard / n_toa_fired_perBoard;
					// Reset them:
					toa_sum_perBoard = 0;
					n_toa_fired_perBoard = 0;
				}
				// Put the sum of LG values
				dataBlockZS[hexa][32] = lg_sum_perBoard;
				lg_sum_perBoard = 0;


				//energy sum estmation for each board based on pedestal and common mode noise estimated subtracted values
				energy_sum_perBoard = 0;
				float cm_noise_hg2 = CalcMHWScore(hg_2_pedestal_subtracted);
				float cm_noise_hg3 = CalcMHWScore(hg_3_pedestal_subtracted);
				float cm_noise_lg2 = CalcMHWScore(lg_2_pedestal_subtracted);
				float cm_noise_lg3 = CalcMHWScore(lg_3_pedestal_subtracted);


				for (size_t i = 0; i < hg_3_pedestal_subtracted.size(); i++) {
					float signal_hg = std::max(hg_2_pedestal_subtracted[i] - cm_noise_hg2, hg_3_pedestal_subtracted[i] - cm_noise_hg3);
					float signal_lg = std::max(lg_2_pedestal_subtracted[i] - cm_noise_lg2, lg_3_pedestal_subtracted[i] - cm_noise_lg3);
					
					if ((signal_hg > 0) && ((signal_hg) < 1500))	energy = (signal_hg) / 45.;		//one MIP = 45 HG ADC
					else if ((signal_lg > 0) && ((signal_lg) < 1200))	energy = 9.*(1.*signal_lg) / 45.;		//9HG ADC = 1LG ADC, one MIP = 45 HG ADC
					else if ((signal_lg) >= 1200) energy = 4.*9.*(tot[i] - 100.) / 45.;		//100 ADC offset in TOT, 1TOT ADC = LG ADC, 9HG ADC = 1LG ADC, one MIP = 45 HG ADC
					else energy = -1;

					//if (HG_TS3>1000) std::cout << "Energy: " << energy << "   LG: " << lg_3_pedestal_subtracted[i] - cm_noise_lg3 << "   TOT: " << tot[i] << std::endl;

					if (energy > 1)	{ //1 MIP threshold
						energy_sum_perBoard += energy;
						nhits += 1;
					}
				}

				std::cout<<"Energy_sum: "<<energy_sum_perBoard<<"   depth: "<<energy_sum_perBoard * hexa<<std::endl;

				energy_sum += energy_sum_perBoard;
				depth += energy_sum_perBoard * hexa;
				// Reset them:
				hg_3_pedestal_subtracted.clear();
				lg_3_pedestal_subtracted.clear();
				tot.clear();
			}

		}

		//fill the reconstructed quantities to the block
		dataBlockZS[nHexa].push_back(nhits);

		dataBlockZS[nHexa].push_back(0.);
		std::memcpy(&dataBlockZS[nHexa][1], &energy_sum, sizeof(unsigned)) ;		//memcpy because energy_sum is a float, must be accounted for in the readout

		dataBlockZS[nHexa].push_back(0.);
		std::memcpy(&dataBlockZS[nHexa][2], &depth, sizeof(unsigned)) ;		//memcpy because depth is a float, must be accounted for in the readout

		return dataBlockZS;

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
	HexaBoardConverterPlugin()
		: DataConverterPlugin(EVENT_TYPE) {}

	// Information extracted in Initialize() can be stored here:
	uint32_t m_skiMask[5];

	int m_runMode;
	bool m_compressed;

	// The single instance of this converter plugin
	static HexaBoardConverterPlugin m_instance;
}; // class HexaBoardConverterPlugin

// Instantiate the converter plugin instance
HexaBoardConverterPlugin HexaBoardConverterPlugin::m_instance;

} // namespace eudaq
