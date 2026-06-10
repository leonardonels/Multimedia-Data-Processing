#include "base64.h"


std::map<uint8_t, uint8_t> d_map={{'A',0},{'B',1},{'C',2},{'D',3},{'E',4},{'F',5},{'G',6},{'H',7},{'I',8},{'J',9},{'K',10},{'L',11},{'M',12},{'N',13},{'O',14},{'P',15},{'Q',16},{'R',17},{'S',18},{'T',19},{'U',20},{'V',21},{'W',22},{'X',23},{'Y',24},{'Z',25},
                                  {'a',26},{'b',27},{'c',28},{'d',29},{'e',30},{'f',31},{'g',32},{'h',33},{'i',34},{'j',35},{'k',36},{'l',37},{'m',38},{'n',39},{'o',40},{'p',41},{'q',42},{'r',43},{'s',44},{'t',45},{'u',46},{'v',47},{'w',48},{'x',49},{'y',50},{'z',51},
                                  {'0',52},{'1',53},{'2',54},{'3',55},{'4',56},{'5',57},{'6',58},{'7',59},{'8',60},{'9',61},{'+',62},{'/',63},{'=',64}};

std::string base64_decode(const std::string& input)
{
    std::string out_buffer = "";
    
    size_t size = input.size();
    for(size_t i = 0; i < size/4; i++){

        uint32_t in_buffer = (input[i*4    ] << 24) | 
                             (input[i*4 + 1] << 16) | 
                             (input[i*4 + 2] <<  8) |
                             (input[i*4 + 3]      ) ;

        uint32_t buffer = (((d_map[in_buffer >> 24 &0xFF])&0x3F)<<18) | 
                          (((d_map[in_buffer >> 16 &0xFF])&0x3F)<<12) | 
                          (((d_map[in_buffer >>  8 &0xFF])&0x3F)<< 6) | 
                          (((d_map[in_buffer       &0xFF])&0x3F)    ) ; 
        
        uint32_t paolo = 0;
        for(size_t j = 0; j < 4; j++){
            if(((d_map[in_buffer >> (24 - 8*j) &0xFF])&0x7F)==64)
                paolo |= 63 << (18 - 6*j);
        }

        if(((paolo>>16)&0x3f)==0)
            out_buffer += ((char)((buffer>>16)&0xFF));
        if(((paolo>>8)&0x3f)==0)
            out_buffer += ((char)((buffer>> 8)&0xFF));
        if(((paolo)&0x3f)==0)
            out_buffer += ((char)((buffer    )&0xFF));
        
    }

    return out_buffer;
}