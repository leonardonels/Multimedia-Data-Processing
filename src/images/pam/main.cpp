#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <array>
#include <algorithm>

void error(std::string_view s) {
	std::cout << "ERROR: " << s << "\n";
	exit(EXIT_FAILURE);
}
void header_error() {
	error("Error: wrong header PAM");
}
template<typename T>
/* mode = "bin" OR "text" */
T open(const std::string& filename, const std::string& mode) {
	// apro file in lettura
	if (mode == "bin") {
		T str(filename, std::ios::binary);
		// controllo corretta apertura
		if (!str) {
			error("Unable to open file: " + filename);
		}
		return str;
	}
	else if (mode == "txt") {
		T str(filename);
		// controllo corretta apertura
		if (!str) {
			error("Unable to open file: " + filename);
		}
		return str;
	}
	else {
		error("Insert correct mode: bin OR txt");
	}	
}

namespace pam {

	template<size_t N>
	class pixel {
		std::array<uint8_t, N> values_;	// o sono 1 o sono 3 o sono 4

	public:

		uint8_t& operator[](size_t index) {
			if (index >= values_.size()) {
				error("Index out of range");
			}
			return values_[index];
		}
		const uint8_t& operator[](size_t index) const {
			if (index >= values_.size()) {
				error("Index out of range");
			}
			return values_[index];
		}

		size_t size() { return values_.size(); }
	};

	class PAM_header {
		size_t width_, height_, depth_, maxval_;
		std::string tupltype_;

	public:

		PAM_header(const size_t width, const size_t height, const size_t depth, const size_t maxval = 255,
			const std::string tupltype = "GRAYSCALE") {

			width_ = width;
			height_ = height;
			depth_ = depth;
			maxval_ = maxval;
			tupltype_ = tupltype;
		}

		void print_header(std::ostream& os) {
			os << "P7\nWIDTH " << width_ << "\nHEIGHT " << height_ << "\nDEPTH " << depth_ << "\nMAXVAL " << maxval_
				<< "\nTUPLTYPE " << tupltype_ << "\nENDHDR\n";
		}

		size_t width() const { return width_; }
		size_t height() const { return height_; }
		size_t depth() const { return depth_; }
		size_t maxval() const { return maxval_; }
	};

	template<size_t N>
	class matrix {
		size_t rows_;
		size_t cols_;
		size_t depth_;
		std::vector<pixel<N>> data_;

	public:

		matrix(size_t rows, size_t cols) : data_(rows * cols) {
			rows_ = rows;
			cols_ = cols;
			depth_ = N;
		}
		/*	DEDUCING THIS PARADIGM
		auto&& operator()(this auto&& self, int a, int b) {
			int res = a + b;
			return self.res;
		}	*/

		pixel<N>& operator()(size_t r, size_t c) { return data_[r * cols_ + c]; }
		const pixel<N>& operator()(size_t r, size_t c) const { return data_[r * cols_ + c]; }

		void print_matrix(std::ostream& os) {
			os.write(rawdata(), rawsize());
			if (!os.good()) {
				error("Error in printing matrix");
			}
		}

		size_t rawsize() const { return size() * sizeof(pixel<N>); }
		char* rawdata() { return reinterpret_cast<char*>(data_.data()); }
		const char* rawdata() const { return reinterpret_cast<const char*>(data_.data()); }

		size_t rows() const { return rows_; }
		size_t cols() const { return cols_; }
		size_t depth() const { return depth_; }
		size_t size() const { return rows_ * cols_; }
	};

	template<size_t N>
	class PAM_image {
		PAM_header header_;
		matrix<N> mat_;

		void get_header(std::vector<std::string>& strings, std::istream& is) {
			for (int i = 0; i < 12; ++i) {
				std::string str;
				is >> str;
				strings.push_back(str);
			}
		}
		void check_header(std::vector<std::string> strings) {
			if (strings.size() != 12) {	header_error(); }
			if (strings[0] != "P7") { header_error(); }
			if (strings[1] != "WIDTH") { header_error(); }
			if (stoi(strings[2]) < 0) { header_error();	}
			if (strings[3] != "HEIGHT") { header_error(); }
			if (stoi(strings[4]) < 0) { header_error();	}
			if (strings[5] != "DEPTH") { header_error(); }
			if (stoi(strings[6]) < 0 || stoi(strings[6]) > 5) { header_error();	}
			if (strings[7] != "MAXVAL") { header_error(); }
			if (stoi(strings[8]) < 0) {	header_error(); }
			if (strings[9] != "TUPLTYPE") {	header_error();	}
			if (strings[10] != "GRAYSCALE" && strings[10] != "RGB") { header_error(); }
			if (strings[10] == "GRAYSCALE" && stoi(strings[6]) != 1){ header_error(); }
			if (strings[10] == "RGB" && stoi(strings[6]) < 3) { header_error();	}
			if (strings[11] != "ENDHDR") { header_error(); }
		}

	public:

		/* per quando costruisco una pam image dai parametri */
		PAM_image(size_t width, size_t height, size_t depth = N, size_t maxval = 255, std::string tupltype = "GRAYSCALE") :
			header_(width, height, depth, maxval, tupltype), mat_(height, width) {}

		/* per quando leggo una una pam image da file */
		PAM_image(std::istream& is) : header_(0, 0, 0), mat_(0, 0) {
			// estraggo l'header pam
			std::vector<std::string> strings;
			get_header(strings, is);

			// controllo la corretta formattazione dell'header
			check_header(strings);

			// estraggo i parametri
			size_t width = std::stoi(strings[2]);
			size_t height = std::stoi(strings[4]);
			size_t depth = std::stoi(strings[6]);
			size_t maxval = std::stoi(strings[8]);
			std::string tupltype = strings[10];

			// costruisco header pam
			header_ = PAM_header(width, height, depth, maxval, tupltype);

			// costruisco matrice
			mat_ = matrix<N>(height, width);
			
			// estraggo i pixel dall'immagine di input (skippo il primo carattere che è un \n)
			uint8_t val;
			is.read(reinterpret_cast<char*>(&val), sizeof(uint8_t));

			for (size_t r = 0; r < mat_.rows(); ++r) {
				for (size_t c = 0; c < mat_.cols(); ++c) {
					for (size_t i = 0; i < depth; ++i) {
						is.read(reinterpret_cast<char*>(&val), sizeof(uint8_t));
						mat_(r, c)[i] = val;
					}
				}
			}
		}

		template<size_t N>
		auto& operator()(size_t r, size_t c) { return mat_(r, c); }
		template<size_t N>
		auto& operator()(size_t r, size_t c) const { return mat_(r, c); }

		void print_image(std::ostream& os) {
			header_.print_header(os);
			mat_.print_matrix(os);
		}

		const PAM_header& header() const { return header_; }
		PAM_header& header() { return header_; }
		template<size_t N>
		const matrix<N>& mat() const { return mat_; }
		template<size_t N>
		matrix<N>& mat() { return mat_; }

		size_t rows() const { return mat_.rows(); }
		size_t cols() const { return mat_.cols(); }
		size_t size() const { return mat_.rows() * mat_.cols(); }
		size_t pixel_depth() const { return mat_.depth(); }
	};

	// esercizio 1
	void gray(matrix<1>& mat) {
		size_t rows = mat.rows();
		size_t cols = mat.cols();
		size_t depth = mat.depth();

		for (size_t r = 0; r < rows; ++r) {
			for (size_t c = 0; c < cols; ++c) {
				mat(r, c)[0] = static_cast<uint8_t>(r);
			}
		}
	}
	void gray(PAM_image<1>& img) {
		gray(img.mat<1>());
	}
	// esercizio 2
	template<size_t N>
	void upside_down(matrix<N>& mat) {
		size_t rows = mat.rows();
		size_t half_rows = rows / 2;
		size_t cols = mat.cols();
		size_t depth = mat.depth();

		for (size_t r = 0; r < half_rows; ++r) {
			// swap delle righe
			for (size_t c = 0; c < cols; ++c) {
				// swap dei pixel
				for (size_t i = 0; i < depth; ++i) {
					std::swap(mat(r, c)[i], mat(rows - 1 - r, c)[i]);
				}
			}
		}
	}
	template<size_t N>
	void upside_down(PAM_image<N>& img) {
		upside_down(img.mat<N>());
	}
	// esercizio 3
	template<size_t N>
	void mirror(matrix<N>& mat) {
		size_t rows = mat.rows();
		size_t cols = mat.cols();
		size_t half_cols = cols / 2;
		size_t depth = mat.depth();

		for (size_t c = 0; c < half_cols; ++c) {
			// swap delle colonne
			for (size_t r = 0; r < rows; ++r) {
				// swap dei pixel
				for (size_t i = 0; i < depth; ++i) {
					std::swap(mat(r, c)[i], mat(r, cols - 1 - c)[i]);
				}
			}
		}
	}
	template<size_t N>
	void mirror(PAM_image<N>& img) {
		mirror(img.mat<N>());
	}
}


int main(int argc, char* argv[]) {

	using namespace pam;

	// ESERCIZIO 1
	{
		// apro file in scrittura
		std::ofstream os = open<std::ofstream>("gray.pam", "bin");

		// creo l'immagine in formato pam
		PAM_image<1> img(256, 256);

		// coloro di grigio
		gray(img);
		
		// scrivo i valori della matrix
		img.print_image(os);
	}

	// ESERCIZIO 2
	{
		// apro file in lettura
		std::ifstream is = open<std::ifstream>("frog.pam", "bin");

		// apro file in scrittura
		std::ofstream os = open<std::ofstream>("reverse_frog.pam", "bin");

		// estraggo l'immagine pam
		PAM_image<1> img(is);

		// specchio l'immagine dall'alto al basso
		upside_down(img);

		// scrivo in output l'immagine
		img.print_image(os);
	}

	// ESERCIZIO 3
	{
		// apro file in lettura
		std::ifstream is = open<std::ifstream>("laptop.pam", "bin");

		// apro file in scrittura
		std::ofstream os = open<std::ofstream>("laptop_mirrored.pam", "bin");

		// estraggo l'immagine pam
		PAM_image<3> img(is);

		// specchio l'immagine da dx a sx
		mirror(img);

		// scrivo in output l'immagine
		img.print_image(os);
	}
	
	return EXIT_SUCCESS;
}