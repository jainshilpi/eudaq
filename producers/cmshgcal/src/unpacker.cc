#include <iostream>
#include <iomanip>
#include <bitset>
#include <array>

#include "unpacker.h"

void unpacker::unpack_and_fill(std::vector<uint32_t> &raw_data)
{
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
}

void unpacker::rawDataSanityCheck()
{
}

void unpacker::printDecodedRawData()
{
  unsigned int chipIndex=0;
  unsigned int offset=0;
  while(1){
    std::bitset<13> rollMask( m_decoded_raw_data[1920+offset]&MASK_ROLL );
    std::cout << std::dec << "chip ID" << chipIndex << " rollMask = " << (m_decoded_raw_data[1920+offset]&MASK_ROLL) << " = " << rollMask << std::endl;
    // Low gain, TOA fall, TOT fast 
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
    offset=(chipIndex+1)*1924;
    if( offset>=m_decoded_raw_data.size() )
      break;
  }
}

uint16_t unpacker::gray_to_binary(const uint16_t gray) const
{
  uint16_t result = gray & (1 << 15);
  result |= (gray ^ (result >> 1)) & (1 << 14); 
  result |= (gray ^ (result >> 1)) & (1 << 13); 
  result |= (gray ^ (result >> 1)) & (1 << 12); 
  result |= (gray ^ (result >> 1)) & (1 << 11); 
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
