#define _CRT_SECURE_NO_WARNINGS

#include "sort_stream.h"
#include "compare_integers.h"
// header in stile c++: non si mette il .h, se la libreria è in c si mette una c all'inizio del nome
#include <cassert>
#include <crtdbg.h>
#include <utility>
#include <iostream>
#include <fstream>
#include <sstream>

// VERSIONE 6 DELL'ESERCIZIO, CON STREAM, NEW E DELETE, TEMPLATE E OPERATORE <<

/*
STREAM 
sostituto più generico in c++ dei file, non devo ricordarmi di aprirlo o chiuderlo
esistono console stream (iostream), file stream (fsream) e anche stream che scrivono su stringa (sstream)
*/

namespace mdp {
    /*
    vector non è più una struct ma è un template di struct, quindi quando lo uso devo specificare il tipo con <T>
    il compilatore assegnerà il tipo giusto al posto mio
    */
    template<typename T>
    struct vector {

    private:

        size_t capacity_;
        size_t size_;
        T* data_;

    public:

        friend void swap(mdp::vector<T>& x, mdp::vector<T>& y) {
            using std::swap;
            //enables ADl (argument dependant lookup): il compilatore cerca il modo migliore di scambiare quei due elementi
            swap(x.capacity_, y.capacity_);
            swap(x.size_, y.size_);
            swap(x.data_, y.data_);
        }

        // i magnifici sei ora sono diventati 5: copy assignement e move assignement si sono fusi in copy and swap idiom
        // costruttore
        vector() {
            capacity_ = 10;
            size_ = 0;
            data_ = new T[capacity_];
        }
        // (deep) copy constructor
        vector(const vector& other) {   // & lvalue reference
            capacity_ = other.capacity_;
            size_ = other.size_;
            data_ = new T[capacity_];
            for (size_t i = 0; i < size_; i++) {
                data_[i] = other.data_[i];
            }
        }
        // move constructor
        vector(vector&& other) noexcept {  // && rvalue reference, non sono mai const, non avrebbe senso
            // il noexcept serve per aiutare il compilatore a ottimizzare, tanto non può generare eccezioni
            capacity_ = other.capacity_;
            size_ = other.size_;
            data_ = other.data_;
            // l'oggetto deve rimanere in uno stato consistente:
            other.capacity_ = 0;
            other.size_ = 0;
            other.data_ = nullptr;
        }
        // copy and swap idiom
        vector& operator=(vector rhs) noexcept {
            swap(*this, rhs);
            return *this;
        }
        // distruttore
        ~vector() {
            delete[] data_;
        }   
        
        void push_back(const T& num) {
            // se necessito di spazio realloco memoria
            if (size_ == capacity_) {
                capacity_ *= 2;
                // la realloc non funziona più, devo farla io a mano :(
                auto tmp = new T[capacity_];
                for (size_t i = 0; i < size_; i++) {
                    tmp[i] = data_[i];
                }
                delete[] data_;
                data_ = tmp;
            }
            //inserisco il dato
            data_[size_] = num;
            //incremento la dimensione
            size_++;
        }

        const T& at(size_t i) const {
            assert(i < size_);
            return data_[i];
        }
        T& at(size_t i) {
            assert(i < size_);
            return data_[i];
        }
        const T& operator[](size_t i) const {
            assert(i < size_);
            return data_[i];
        }
        T& operator[](size_t i) {
            assert(i < size_);
            return data_[i];
        }
        size_t size() const {
            return size_;
        }
    };
}

// funzione di stampa
template<typename T>
void print_vector(std::ostream& os, const mdp::vector<T>& v) {
    for (int i = 0; i < v.size(); i++) {
        /*
        operatore inserter : << , significa inserisci in cout quello che c'è a dx
        l'operatore << ritorna una reference allo stream: in questo caso std::cout
        */
        //std::cout << v[i];
        //std::cout.operator<<(v[i]);
        // le due scritture sono equivalenti

        std::cout << v[i] << '\n';
        //std::cout.operator<<(v[i]).operator<<('\n');
        // le due scritture sono equivalenti

        //scrivo su stream
        os << v[i];
    }
}

mdp::vector<double> read_vector(const char* filename) {
    mdp::vector<double> q;
    std::ifstream is(filename);

    if (!is) {
        return q;
    }

    double num;
    // se la lettura fallisce num vale il valore precedente
    while (is >> num) {
        q.push_back(num);
    }

    return q;
}


/* new
fa la malloc e chiama il costruttore

delete
chiama il distruttore e fa la free
*/
struct widget {
    int *i;
    widget() {
        i = new int;
        *i = 42;
    }
    // dato che non voglio permettere l'uso di copy constructor e copia li cancello
    widget(const widget&) = delete;
    widget& operator=(const widget&) = delete;

    ~widget() {
        delete i;
    }
    int get() const {
        return *i;
    }

    // dichiarazione di funzione friend, la definizione è fuori dalla struct
    friend std::ostream& operator<<(std::ostream& os, const widget& w);

    // operatore inserimento >>
    friend std::istream& operator>>(std::istream& is, widget& w) {
        is >> *w.i;
        return is;
    }
};

// funzione free standing globale
std::ostream& operator<<(std::ostream& os, const widget& w) {
    return os << w.i[0];
}


// "main"
int sort_stream(char* filein, char* fileout) {

    using namespace mdp;
    
    // se alloco con new struct dovrò chiamare delete, se alloco con new struc[] dovrò chiamare delete[]
    widget *pw = new widget;
    int k = pw->get();
    delete pw;

    widget* pw_array = new widget[3];
    int x = pw_array[0].get();
    int y = pw_array[1].get();
    int z = pw_array[2].get();
    delete[] pw_array;
    
    //widget w;
    //std::cout << w << '\n';
    //std::cin >> w;
    //std::cout << w << '\n';


    // Read integers from the input file
    vector<double> v = read_vector(filein);
    
    /*    ofstream = output file stream, std::ios::binary è indispensabile in lettura o scrittura di file binari,
    windows interpreta 1310 come a capo    */
    std::ofstream os(fileout/*, std::ios::binary*/);

    // Sort the numbers
    //v.sort();    

    // Write sorted numbers to the output file
    print_vector(os, v);

    return 0;
}
