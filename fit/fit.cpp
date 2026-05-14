#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <cstdint>
#include <cassert>
#include <ranges>
#include <unordered_map>
#include <iomanip>

using namespace std;

void FitCRC_Get16(uint16_t& crc, uint8_t byte)
{
	static const uint16_t crc_table[16] =
	{
		0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401,
		0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400
	};
	uint16_t tmp;
	// compute checksum of lower four bits of byte
	tmp = crc_table[crc & 0xF];
	crc = (crc >> 4) & 0x0FFF;
	crc = crc ^ tmp ^ crc_table[byte & 0xF];
	// now compute checksum of upper four bits of byte
	tmp = crc_table[crc & 0xF];
	crc = (crc >> 4) & 0x0FFF;
	crc = crc ^ tmp ^ crc_table[(byte >> 4) & 0xF];
}

class BitReader
{
private:
	ifstream& is_;
	uint64_t buffer_;
	size_t bits_;
	uint16_t crc_;
public:
 	BitReader(ifstream& is) : is_(is), buffer_(0), bits_(0), crc_(0) {}
	BitReader() = default;

	uint16_t get_crc() const { return crc_; }
	void reset_crc() { crc_ = 0; }

	uint32_t read_bits(size_t size = 0)	// little endian
	{
		while(bits_<size){
			uint64_t byte = is_.get();
			FitCRC_Get16(crc_, byte);
			buffer_ |= (uint64_t)(byte) << (bits_);
			bits_ += 8;
		}

		uint32_t x = buffer_&((1ull << size)-1);
		buffer_ >>= size;
		bits_ -= size;
		return x;
	}
};

class Fit
{
private:
	string MAGICNUMBER = ".FIT";
	BitReader& br_;
	struct field{
		uint8_t number;
		uint8_t size;
		uint8_t base_type;
	};
	struct entry{
		uint8_t arch;
		uint16_t glob_msg_n;
		vector<field> fields;
		size_t payload_size;
	};
public:
	~Fit() = default;
	Fit(BitReader& br) : br_(br) 
	{
		uint8_t header_size = br.read_bits(8);	// including header size, but why?
		uint8_t protocol_version = br.read_bits(8);
		uint16_t profile_version = br.read_bits(16);
		uint32_t data_size = br.read_bits(32);
		
		uint32_t fit = br.read_bits(32);
		for(size_t i : ranges::iota_view(0,4)){
			if(((fit>>8*i)&0xFF)!=MAGICNUMBER[i])
				exit(1);
		}

		uint16_t crc = br.get_crc();
		if(crc != br.read_bits(16))
			exit(1);

		cout << "Header CRC ok" << endl;
		
		// ---

		unordered_map<uint8_t, entry> table;

		uint8_t local, arch, n_fields;
		uint16_t glob_msg_n;

		size_t read = 0;
		uint8_t x;
		while(read < data_size){
			x = br.read_bits(8); read++;
			local = x&0xF;
			if(((x>>4)&0xF)==4){	// Definition
				
				if(br.read_bits(8) != 0)
					exit(1);
				read++;

				arch = br.read_bits(8); read++;
				glob_msg_n = br.read_bits(16); read+=2;
				n_fields = br.read_bits(8); read++;
				
				vector<field> fields;
				size_t sum = 0;
				for(size_t j = 0; j < n_fields; j++){
					field f;
					f.number = br.read_bits(8); read++;
					f.size = br.read_bits(8); read++; sum+=f.size;
					f.base_type = br.read_bits(8); read++;
					fields.push_back(f);
				}

				table[local]=entry(arch, glob_msg_n, fields, sum);
				// cout << "table created with global " << glob_msg_n << endl;
			}else{
				for(auto e : table[local].fields){
					uint64_t ex = br.read_bits(8*e.size); read+=e.size;

					if(table[local].glob_msg_n == 0 && e.number == 4)
						cout << "time_created = " << ex << endl;
					if(e.number == 13 && table[local].glob_msg_n == 19)
						cout << "avg_speed = " << fixed << setprecision(3) << ((double)(ex)* 0.0036) << " km/h" << endl;
				}
			}
		}
		crc = br.get_crc();
		if(crc != br.read_bits(16))
			exit(1);
		cout << "File CRC ok" << endl;
	}
};

int main(int argc, char **argv)
{
	#ifdef DEBUG
	char* d_argv[] = {(char*)"./fitdump",
			(char*)"./example.fit",
			nullptr};
	argv = d_argv;
	argc = 2;
	#endif

	if(argc != 2)
		exit(1);

	ifstream is(argv[1], ios::binary);
	if(!is)
		exit(1);
	
	BitReader br(is);
	Fit dump(br);

	return 0;
}