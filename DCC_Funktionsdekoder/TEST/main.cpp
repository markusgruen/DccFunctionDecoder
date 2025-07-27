/******************************************************************************

                              Online C++ Compiler.
               Code, Compile, Run and Debug C++ program online.
Write your code in this editor and press "Run" button to compile and execute it.

*******************************************************************************/

#include <iostream>
#include "dccdecoder.h"
#include <bit>
#include <bitset>


DCCDecoder decoder;

int main()
{
 uint64_t bitstream = 0;   
 uint8_t pos = 0;
  bitstream = 0b100001111001111111011111111111110110ULL;
  pos = 36;
  
  std::bitset<64> x(bitstream);
    std::cout << x << '\n';

  // Nun: bitstream enthält die korrekte Bitfolge
  uint8_t totalBits = pos;
    
    decoder.TESTsetBitstream(bitstream, pos);
    for(int i=0; i<2; i++){
        decoder.run();
    }
    
    char out[8];
    uint8_t num = 42;
    
    num = decoder.getBytestream(out);
    std::cout<<(int)num<<std::endl;
    std::cout<<(int)out[0]<<std::endl;
    std::cout<<(int)out[1]<<std::endl;

    return 0;
}