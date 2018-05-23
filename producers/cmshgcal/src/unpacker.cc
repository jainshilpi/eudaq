#include <iostream>
#include <iomanip>
#include <bitset>
#include <array>
#include "unpacker.h"

void unpacker::unpack_and_fill(std::vector<uint32_t> &raw_data)
{
  m_decoded_raw_data.clear();
  const std::bitset<32> ski_mask(raw_data[0]);
  const int mask_count = ski_mask.count();
  std::vector< std::array<uint16_t, 1924> > skirocs(mask_count, std::array<uint16_t, 1924>());
  uint32_t x;
  const int offset = 1; // first byte coming out of the hexaboard is FF
  for(int  i = 0; i < 1924; i++){
    for (int j = 0; j < 16; j++){
      x = raw_data[offset + i*16 + j];
      int k = 0;
      for (int fifo = 0; fifo < 32; fifo++){
	if (raw_data[0] & (1<<fifo)){
	  skirocs[k][i] = skirocs[k][i] | (uint16_t) (((x >> fifo ) & 1) << (15 - j));
	  k++;
	}
      }
    }
  }
  for( std::vector< std::array<uint16_t, 1924> >::iterator it=skirocs.begin(); it!=skirocs.end(); ++it )
    m_decoded_raw_data.insert(m_decoded_raw_data.end(),(*it).begin(),(*it).end());
  //std::cout << "m_decoded_raw_data.size() = " << std::dec << m_decoded_raw_data.size() << std::endl;
}

void unpacker::rawDataSanityCheck()
{
  unsigned int chipIndex=0;
  unsigned int offset=0;
  while(1){
    std::bitset<13> rollMask( m_decoded_raw_data[1920+offset]&MASK_ROLL );
    if( rollMask.count()!=2 ){
      std::cout << "Problem: wrong roll mask format in chip " << std::dec << chipIndex << "\t rollMask = "<< rollMask << std::endl;
    }
    uint16_t head[64];
    for( int ichan=0; ichan<64; ichan++ ){
      head[ichan]=(m_decoded_raw_data.at(offset+ichan+ADCLOW_SHIFT) & MASK_HEAD)>>12;
      if(head[ichan]!=8&&head[ichan]!=9)
	std::cout << "ISSUE in chip,chan " << std::dec<<chipIndex <<","<<ichan << " : we expected 8(1000) or 9(1001) for the adc header and I find " << std::bitset<4>(head[ichan]) << std::endl;
    }
    for( int ichan=0; ichan<64; ichan++ ){
      for(int isca=0; isca<NUMBER_OF_SCA-1; isca++){
    	uint16_t new_head=( m_decoded_raw_data.at(offset+ichan+ADCLOW_SHIFT+SCA_SHIFT*isca) & MASK_HEAD )>>12;
    	if( new_head!=head[ichan] )
    	  std::cout << "\n We have a major issue in chip,chan " << std::dec<<chipIndex<<","<<ichan << " : (LG)-> " << std::bitset<4>(new_head) << " should be the same as " << std::bitset<4>(head[ichan]) << std::endl;
      }
    }
    for( int ichan=0; ichan<64; ichan++ ){
      for(int isca=0; isca<NUMBER_OF_SCA-1; isca++){
    	uint16_t new_head=( m_decoded_raw_data.at(offset+ichan+ADCHIGH_SHIFT+SCA_SHIFT*isca) & MASK_HEAD )>>12;
    	if( new_head!=head[ichan] )
    	  std::cout << "\n We have a major issue in chip,chan " << std::dec<<chipIndex<<","<<ichan << " : (LG)-> " << std::bitset<4>(new_head) << " should be the same as " << std::bitset<4>(head[ichan]) << std::endl;
      }
    }
    chipIndex++;
    offset=chipIndex*1924;
    if( offset>=m_decoded_raw_data.size() )
      break;
  }
}

void unpacker::printDecodedRawData()
{
  unsigned int chipIndex=0;
  unsigned int offset=0;
  while(1){
    std::bitset<13> rollMask( m_decoded_raw_data[1920+offset]&MASK_ROLL );
    std::cout << std::dec << "chip ID" << chipIndex << " rollMask = " << (m_decoded_raw_data[1920+offset]&MASK_ROLL) << " = " << rollMask << std::endl;
    // // Low gain, TOA fall, TOT fast 
    for( int ichan=0; ichan<64; ichan++ ){
      std::cout << "channel " << 63-ichan << "\t";
      for(int isca=0; isca<NUMBER_OF_SCA; isca++)
    	std::cout << std::dec << gray_to_binary( m_decoded_raw_data.at(offset+ichan+ADCLOW_SHIFT+SCA_SHIFT*isca) & MASK_ADC ) << " ";
      std::cout << std::endl;
    }
    // High gain, TOA rise, TOT slow 
    for( int ichan=0; ichan<64; ichan++ ){
      std::cout << "channel " << 63-ichan << "\t";
      for(int isca=0; isca<NUMBER_OF_SCA; isca++)
    	std::cout << std::dec << gray_to_binary( m_decoded_raw_data.at(offset+ichan+ADCHIGH_SHIFT+SCA_SHIFT*isca) & MASK_ADC ) << " ";
      std::cout << std::endl;
    }
    chipIndex++;
    offset=chipIndex*1924;
    if( offset>=m_decoded_raw_data.size() )
      break;
  }
}

uint16_t unpacker::gray_to_binary(uint16_t gray) const
{
  gray ^= (gray >> 8);
  gray ^= (gray >> 4);
  gray ^= (gray >> 2);
  gray ^= (gray >> 1);
  return gray;
}
