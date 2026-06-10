#include <cstdint>
#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>
#include <bit>
#include <vector>

void error(const std::string& message) {
	std::cout << message << std::endl;
	exit(EXIT_FAILURE);
}
void syntax() {
	error("Use: MOBIdecode <input filename> <output filename>");
}
template<typename T>
void check_open(T& stream, const std::string& filename) {
	if (!stream) {
		error("Unable to open file: " + filename);
	}
}

template<typename T>
void swapbytes(T& x) {
	x = std::byteswap(x);
}

class PDBheader {
	std::string db_name;
	uint32_t cr_date;
	uint16_t num_records;
public:
	PDBheader(std::string dn, uint32_t cd, uint16_t nr)
		: db_name(dn), cr_date(cd), num_records(nr){}

	void printPDBheader() {
		std::cout << "PDB name: " << db_name << std::endl
			<< "Creation date (s): " << std::to_string(cr_date) << std::endl
			<< "Type: " << "BOOK" << std::endl
			<< "Creator: " << "MOBI" << std::endl
			<< "Records: " << std::to_string(num_records) << std::endl;
	}

	uint16_t num_rec() {
		return num_records;
	}
};
PDBheader readPDBheader(std::ifstream& is) {
	// database name
	char database_name[32];
	is.read(&database_name[0], 32);
	std::string db_name(database_name);
	is.ignore(4);

	// creation date (in seconds)
	uint32_t cr_date;
	is.read(reinterpret_cast<char*>(&cr_date), 4);
	//cr_date = _byteswap_ulong(cr_date);
	swapbytes(cr_date);
	is.ignore(36);

	// number of records
	uint16_t num_records;
	is.read(reinterpret_cast<char*>(&num_records), 2);
	//num_records = _byteswap_ushort(num_records);
	swapbytes(num_records);

	PDBheader pdbh(db_name, cr_date, num_records);

	return pdbh;
}

class RecordInfo {
	uint32_t offset, id;
public:
	RecordInfo(uint32_t o, uint32_t i)
		: offset(o), id(i){}

	void printRecordInfo(size_t i) {
		std::cout << std::to_string(i) << " - offset: " << std::to_string(offset)
			<< " - id: " << std::to_string(id) << std::endl;
	}
	uint32_t getoffset() {
		return offset;
	}

};
RecordInfo readRecordInfoEntry(std::ifstream& is) {
	uint32_t recordDataOffset, uniqueID = 0;
	is.read(reinterpret_cast<char*>(&recordDataOffset), 4);
	swapbytes(recordDataOffset);

	is.ignore(1);
	is.read(reinterpret_cast<char*>(&uniqueID), 3);
	swapbytes(uniqueID);

	RecordInfo rec_info(recordDataOffset, uniqueID);
	return rec_info;
}

class PalmDOCheader {
	uint16_t compression, count, size, enc_type;
	uint32_t text_length;
public:
	PalmDOCheader(uint16_t cmp, uint32_t tl, uint16_t cnt, uint16_t s, uint16_t et)
		: compression(cmp), text_length(tl), count(cnt), size(s), enc_type(et){}

	void printPalmDOC() {
		std::cout << "Compression: " << std::to_string(compression) << std::endl
			<< "TextLength: " << std::to_string(text_length) << std::endl
			<< "RecordCount: " << std::to_string(count) << std::endl
			<< "RecordSize: " << std::to_string(size) << std::endl
			<< "EncryptionType: " << std::to_string(enc_type) << std::endl << std::endl;
	}
	uint16_t getcount() {
		return count;
	}
};
PalmDOCheader readPalmDOCheader(std::ifstream& is) {
	uint16_t compression, count, size, enc_type;
	is.read(reinterpret_cast<char*>(&compression), 2);
	swapbytes(compression);
	is.ignore(2);

	uint32_t text_length;
	is.read(reinterpret_cast<char*>(&text_length), 4);
	swapbytes(text_length);

	is.read(reinterpret_cast<char*>(&count), 2);
	swapbytes(count);

	is.read(reinterpret_cast<char*>(&size), 2);
	swapbytes(size);

	is.read(reinterpret_cast<char*>(&enc_type), 2);
	swapbytes(enc_type);

	PalmDOCheader pdh(compression, text_length, count, size, enc_type);
	return pdh;
}

uint8_t LZ77(char first, char second, std::ifstream& is, std::ofstream& os) {
	// get distance and length
	uint16_t distance;
	uint8_t length = second & 7;
	length += 3;

	distance = first & 63;
	distance = distance << 5;
	distance = distance | ((second >> 3) & 31);

	// Move back distance bytes
	auto cur_pos = std::ios::cur;
	is.seekg(-distance, std::ios::cur);

	// Copy length bytes
	std::vector<char> bytes(length);
	is.read(&bytes[0], length);
	os.write(&bytes[0], length);

	// Reset position
	is.seekg(0, cur_pos);

	return length;
}

void readRecord(std::ifstream& is, std::ofstream& os) {
	char buffer;
	uint32_t cnt = 0;	// when 4096 stop reading
	while (cnt < 4096) {
		is.read(&buffer, 1);
		if (is.eof()) { break; }

		if (buffer == 0x00) { break; }
		else if (buffer >= 0x01 && buffer <= 0x08) {
			// read buffer bytes and write them in os
			std::vector<char> bytes(buffer);
			is.read(&bytes[0], buffer);
			os.write(&bytes[0], buffer);
			cnt += buffer;
		}
		else if (buffer >= 0x09 && buffer <= 0x7f) {
			// write buffer in os
			os.write(&buffer, 1);
			cnt++;
		}
		else if (buffer >= 0x80 && buffer <= 0xbf) {
			// LZ77
			char second;
			is.read(&second, 1);
			cnt += LZ77(buffer, second, is, os);
		}
		else if (buffer >= 0xc0 && buffer <= 0xff) {
			// space + buffer without most significant bit
			char space = 0x20;
			os.write(&space, 1);
			char corrected = buffer & 127;
			os.write(&corrected, 1);
			cnt += 2;
		}
	}
}

int main(int argc, char** argv) {
	if (argc != 3) { syntax(); }

	std::string input_fname(argv[1]);
	std::string output_fname(argv[2]);

	std::ifstream is(input_fname, std::ios::binary);
	check_open(is, input_fname);
	std::ofstream os(output_fname, std::ios::binary);
	check_open(os, output_fname);

	// write BOM
	char bom[3];
	bom[0] = 0xef; bom[1] = 0xbb; bom[2] = 0xbf;
	os.write(&bom[0], 3);

	// Read (and print) PDB header
	PDBheader pdbh = readPDBheader(is);
	pdbh.printPDBheader();
	
	// Read (and print) records info entry
	std::vector<RecordInfo> entries;
	for (size_t i = 0; i < pdbh.num_rec(); ++i) {
		auto entry = readRecordInfoEntry(is);
		entry.printRecordInfo(i);
		entries.push_back(entry);
	}
	std::cout << std::endl;

	// Move to first record
	auto cur_pos = std::ios::cur;
	auto first_record_offset = entries[0].getoffset();
	is.seekg(first_record_offset, std::ios::beg);

	// Read (and print) palmDOC header
	PalmDOCheader pdh = readPalmDOCheader(is);
	pdh.printPalmDOC();

	// Move to second record
	auto second_record_offset = entries[1].getoffset();
	is.seekg(second_record_offset, std::ios::beg);

	// Read record
	for (size_t i = 0; i < pdh.getcount(); ++i) {
		readRecord(is, os);
	}


	return EXIT_SUCCESS;
}