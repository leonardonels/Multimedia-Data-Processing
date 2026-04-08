/* ranges:
	un range � un oggetto che contiene un iteratore (begin) e un sentinel (end)
lower_bound, upper_bound:
	algoritmi di ricerca binaria, ritornano il primo e l'ultimo elemento con valore definito, complessit� o(n)
resize vs reserve:
	resize realloca lo spazio per n cose, se e solo se la nuova dimensione > della capacità corrente (impostata con reserve) e setta la dimensione
	reserve riserva dello spazio per n cose preallocandolo ma non modifica la dimensione
	reserve viene utilizzato per ottimizzare cicli di allocazioni */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <vector>
#include <queue>
#include <map>
#include <algorithm>

const std::string MAGIC_NUMBER = "HUFFMAN1";

void error(const std::string& message) {
	std::cout << message;
	EXIT_FAILURE;
}
void synthax() {
	error("Synthax error:\nhuffman1 [c|d] <input file> <output file>");
}
template<typename T>
void check_open(const T& stream, const std::string& filename) {
	if (!stream) {
		error("impossible to open file " + filename);
	}
}

class bitreader {
	uint8_t buffer_;	/*Questo � un buffer che memorizza un byte (8 bit) di dati letti dal flusso di input.
						Serve come riserva temporanea per estrarre singoli bit*/
	size_t n_ = 0;	/*Questo contatore tiene traccia di quanti bit rimangono nel buffer buffer_ da leggere.
					Inizialmente � impostato a 0, indicando che nessun bit � stato letto*/
	std::istream& is_;	/*Un riferimento a un flusso di input (std::istream) da cui vengono letti i dati. Questo � il flusso da cui i bit verranno estratti*/
	uint64_t readbit() {
		/*Questo metodo legge un singolo bit dal buffer.
		Se non ci sono bit disponibili (n_ == 0), viene letto un nuovo byte dal flusso di input e memorizzato in buffer_.
		Il contatore n_ viene quindi impostato a 8.
		Ogni volta che viene letto un bit, n_ viene decrementato.
		Il metodo ritorna il bit pi� significativo disponibile in buffer_ spostato a destra della quantit� necessaria per isolare il bit desiderato.*/
		if (n_ == 0) {
			buffer_ = is_.get();
			n_ = 8;
		}
		--n_;
		return (buffer_ >> n_) &1;
	}
public:
	bitreader(std::istream& is) : is_(is) {}	/*Costruttore della classe che inizializza il flusso di input is_ con il riferimento passato*/
	uint64_t operator()(uint64_t numbits) {
		/*Questo operatore funzione legge numbits bit dal flusso e li restituisce come un singolo valore uint64_t.
		La funzione itera fino a quando non ha letto tutti i bit richiesti, chiamando readbit() per ogni bit e concatenandoli in result*/
		uint64_t result = 0;
		while (numbits-- > 0) {
			result = (result << 1) | readbit();
		}
		return result;
	}
	bool fail() const {	/*Questo metodo controlla se il flusso di input ha incontrato un errore, restituendo il risultato della chiamata a is_.fail()*/
		return is_.fail();
	}
	operator bool() const {	/*Questo operatore di conversione permette di usare un oggetto bitreader in un contesto booleano.
							Restituisce true se non ci sono errori nel flusso di input, altrimenti false*/
		return !fail();
	}
};
class bitwriter {
	uint8_t buffer_;	/*Questo � un buffer che accumula i bit scritti finch� non viene riempito un byte completo (8 bit)*/
	size_t n_ = 0;	/*Questo contatore tiene traccia di quanti bit sono stati scritti nel buffer.
					Quando raggiunge 8, il buffer viene svuotato nel flusso di output*/
	std::ostream& os_;	/*Un riferimento a un flusso di output (std::ostream) in cui i dati vengono scritti*/
	void writebit(uint64_t curbit) {
		/*Questo metodo scrive un singolo bit nel buffer.
		Il bit corrente curbit viene aggiunto al buffer spostando il buffer di un bit a sinistra e concatenando il bit corrente con un'operazione OR.
		Il contatore n_ viene incrementato di 1.
		Se il buffer ha accumulato 8 bit (cio� un byte completo), il byte viene scritto nel flusso di output os_ e n_ viene reimpostato a 0*/
		buffer_ = (buffer_ << 1) | (curbit & 1);
		++n_;
		if (n_ == 8) {
			os_.put(buffer_);
			n_ = 0;
		}
	}
public:
	bitwriter(std::ostream& os): os_(os){}	/*Costruttore della classe che inizializza il flusso di output os_ con il riferimento passato*/
	~bitwriter() {
		/*Distruttore della classe.
		Quando un oggetto bitwriter viene distrutto, il metodo flush() viene chiamato per scrivere eventuali bit rimanenti nel buffer*/
		flush();
	}
	std::ostream& operator()(uint64_t source, uint64_t numbits) {	
		/*Questo operatore funzione scrive numbits bit dal valore source nel flusso di output.
		La funzione itera su ogni bit, partendo dal bit pi� significativo (MSB), e chiama writebit() per scriverlo nel buffer*/
		while (numbits-- > 0) {
			writebit(source >> numbits);
		}
		return os_;
	}
	std::ostream& operator()(const std::string& bits) {
		for (char bit : bits) {
			writebit(bit - '0');
		}
		return os_;
	}
	std::ostream& flush(int padbit=0) {
		/*Questo metodo svuota il buffer, scrivendo eventuali bit rimanenti nel flusso di output.
		Se il buffer non � pieno (cio� n_ non � 0), viene riempito con padbit (che � 0 per default) fino a completare il byte. 
		Questo � necessario per garantire che tutti i bit accumulati siano scritti nel flusso.
		Dopo che il buffer � stato svuotato, il metodo restituisce il flusso di output os_*/
		while (n_ > 0) {
			writebit(padbit);
		}
		return os_;
	}
};

namespace huff {
	struct node {
		uint8_t symbol_;
		uint32_t frequency_;
		node* left_;	// Pointer to the left child
		node* right_;	// Pointer to the right child
		// Constructor for leaf nodes
		node(uint8_t symbol, uint32_t frequency) : symbol_(symbol), frequency_(frequency), left_(nullptr), right_(nullptr) {}
		// Constructor for internal nodes
		node(node* left, node* right) : symbol_(0), frequency_(left->frequency_ + right->frequency_), left_(left), right_(right) {}
		// Destructor to free child nodes
		~node() { delete left_; delete right_; }
	};
	struct compare_node {
		bool operator()(node* left, node* right) {
			return left->frequency_ > right->frequency_;
		}
	};
	node* build_tree(const std::map<uint8_t, uint32_t>& frequencies) {
		/*frequencies: A std::map<uint8_t, uint32_t> where the key is a byte (symbol) and the value is the frequency
		of that symbol in the input data.
		The function returns a pointer to the root node of the constructed Huffman tree (Node*)
		A priority queue is used to maintain the nodes during the construction of the Huffman tree. 
		The priority queue is a min-heap, meaning that the node with the smallest frequency will always be at the top.
		CompareNode is a custom comparator that ensures the priority queue functions as a min-heap based on the frequency of the nodes*/

		// Priority queue to build the Huffman tree
		std::priority_queue<node*,std::vector<node*>, compare_node> pq;

		// Step 1: Initialize the priority queue with leaf nodes
		for (const auto& [symbol, frequency] : frequencies) {
			pq.push(new node(symbol, frequency));
		}
		// Step 2: Build the Huffman tree
		while (pq.size() != 1) {
			// Remove the two nodes with the smallest frequency
			node* l = pq.top();
			pq.pop();
			node* r = pq.top();
			pq.pop();
			// Combine them into a new internal node with the sum of their frequencies
			node* n = new node(l, r);
			pq.push(n);
		}
		// Step 3: Return the root of the Huffman tree
		return pq.top();
	}
	void generate_codes(node* n, const std::string prefix, std::map<uint8_t, std::string>& codes) {
		// Base case: If the node is a leaf node, store the code
		if (!n->left_ && !n->right_) {
			codes[n->symbol_] = prefix;
			return;
		}
		// Recursive case: Traverse left and right subtrees
		if (n->left_) {
			generate_codes(n->left_, prefix + "0", codes);
		}
		if (n->right_) {
			generate_codes(n->right_, prefix + "1", codes);
		}
	}
}

void compress(const std::string& input_file, const std::string& output_file) {
	using namespace huff;
	std::ifstream is(input_file, std::ios::binary);
	check_open(is, input_file);
	std::ofstream os(output_file, std::ios::binary);
	check_open(os, output_file);

	// Read data from inputfile and Calculate frequencies
	std::map<uint8_t, uint32_t> frequencies;
	char byte;
	while (is.get(byte)) {
		frequencies[static_cast<uint8_t>(byte)]++;
	}
	// Build Huffman Tree
	node* root = build_tree(frequencies);
	// Generate Huffman Codes
	std::map<uint8_t, std::string> codes;	// symbol, code
	generate_codes(root, "", codes);
	bitwriter bw(os);
	// Write the header. 1-MagicNumber
	os.write(MAGIC_NUMBER.c_str(), 8);
	// 2-TableEntries
	uint8_t table_entries = static_cast<uint8_t>(codes.size());
	os.put(table_entries);
	// 3-HuffmanTable, triplets (sym = 8 bit, len = 5 bit, code = len bit)
	for (const auto& [symbol, code] : codes) {
		bw(symbol, 8);
		bw(code.size(), 5);
		bw(code);
	}
	// Write number of symbols in big-endian format.
	uint32_t num_symbols = 0;
	for (const auto& [symbol, freq] : frequencies) {
		num_symbols += freq;
	}
	bw(num_symbols, 32);

	// Write the encoded data. 
	is.clear();
	is.seekg(0, std::ios::beg);
	while (is.get(byte)) {
		bw(codes[static_cast<uint8_t>(byte)]);
	}
	bw.flush();
	delete root;
}

void decompress(const std::string& input_file, const std::string& output_file) {
	using namespace huff;
	std::ifstream is(input_file, std::ios::binary);
	check_open(is, input_file);
	std::ofstream os(output_file, std::ios::binary);
	check_open(os, output_file);
	bitreader br(is);

	// Read and verify magic number
	char magic_number[9];
	is.read(magic_number, 8);
	magic_number[8] = 0;
	if (std::string(magic_number) != MAGIC_NUMBER) {
		std::cout << "Invalid header";
		return;
	}
	// Read the number of table entries
	uint64_t table_entries = br(8);
	if (table_entries == 0) {
		table_entries = 256;
	}
	// Reconstruct the Huffman table
	std::map<std::pair<uint64_t, uint64_t>, uint8_t> codes;	// (code, length), symbol
	for (uint64_t i = 0; i < table_entries; ++i) {
		uint8_t symbol = static_cast<uint8_t>(br(8));
		uint64_t len = (br(5));
		uint64_t code = br(len);
		codes.insert({ {code, len}, symbol });
	}
	// Read the number of symbols in big-endian format
	size_t num_symbols = (br(32));

	// Decode the data
	std::vector<uint8_t> symbols;
	uint64_t code = 0;
	uint64_t len = 0;
	// need to find num_symbols encoded symbols
	for (size_t i = 0; i < num_symbols;) {
		// each bit added correspond to an increase of length
		code = (code << 1) | (br(1) & 1);
		++len;
		// if the code exists in the map codes and the length is equal than the symbol is found
		if (codes.find({ code,len }) != codes.end()) {
			symbols.push_back(codes[{ code, len }]);
			os.put(codes[{ code, len }]);
			++i;
			code = 0;
			len = 0;
		}
	}
}

int main(int argc, char** argv) {
	// standard main
	if (argc != 4) {
		synthax();
	}
	const std::string command = argv[1];
	const std::string input_file = argv[2];
	const std::string output_file = argv[3];	
	if (command == "c") {
		compress(input_file, output_file);
	}
	else if (command == "d") {
		decompress(input_file, output_file);
	}
	else {
		synthax();
	}
	return EXIT_SUCCESS;
}