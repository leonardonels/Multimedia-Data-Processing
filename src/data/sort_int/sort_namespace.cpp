#define _CRT_SECURE_NO_WARNINGS

#include "sort_namespace.h"
#include "compare_integers.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <utility>

// VERSIONE 5  DELL'ESERCIZIO, CON NAMESPPACE, FRIEND, LVALUE-RVALUE REFERENCE E SWAP

/*
uso i namespace per evitare problemi di conflitti tra classi con lo stesso nome. mdp = multimedia data processing
in questo modo la classe non si chiamerà più vector ma mdp::vector, se mi da fastidio scriverlo ogni volta posso usare:
-using mdp::vector
    così "importo" solo la classe vector del namespace mdp
-using namespace mdp
    così "importo" l'intero namespace
*/
namespace mdp {

struct vector {

private:

    size_t capacity_;
    size_t size_;
    int* data_;

public:

    // funzioni friend, swap non è un metodo di vector ma è una funzione che può accedere alle sue variabili private
    friend void swap(mdp::vector& x, mdp::vector& y) {
        // enables ADl (argument dependant lookup): il compilatore cerca il modo migliore di scambiare quei due elementi
        using std::swap;

        swap(x.capacity_, y.capacity_);
        swap(x.size_, y.size_);
        swap(x.data_, y.data_);
    }


    // i magnifici sei :)
    // costruttore
    vector() {
        capacity_ = 10;
        size_ = 0;
        data_ = (int*)malloc(capacity_ * sizeof(int));
    }
    // (deep) copy constructor
    vector(const vector& other) {   //& lvalue reference
        capacity_ = other.capacity_;
        size_ = other.size_;
        data_ = (int*)malloc(capacity_ * sizeof(int));
        for (size_t i = 0; i < size_; i++) {
            data_[i] = other.data_[i];
        }
    }
    // move constructor
    vector(vector&& other) noexcept {  //&& rvalue reference, non sono mai const, non avrebbe senso. il noexcept serve per aiutare il compilatore
        capacity_ = other.capacity_;
        size_ = other.size_;
        data_ = other.data_;
        // l'oggetto deve rimanere in uno stato consistente:
        other.capacity_ = 0;
        other.size_ = 0;
        other.data_ = nullptr;
    }
    // copy assignement operator 
    vector& operator=(const vector& rhs) {
        if (&rhs != this) {     //self assignement could be a problem
            free(data_);
            capacity_ = rhs.capacity_;
            size_ = rhs.size_;
            data_ = (int*)malloc(capacity_ * sizeof(int));
            for (size_t i = 0; i < size_; i++) {
                data_[i] = rhs.data_[i];
            }
        }
        return *this;
    }
    // move assignement operator 
    vector& operator=(vector&& rhs) noexcept {
        free(data_);
        capacity_ = rhs.capacity_;
        size_ = rhs.size_;
        data_ = rhs.data_;
        rhs.capacity_ = 0;
        rhs.size_ = 0;
        rhs.data_ = nullptr;

        return *this;
    }
    // distruttore
    ~vector() {
        free(data_);
    }

    /*
    COPY AND SWAP IDIOM
    vector& operator=(vector rhs) noexcept {
        swap(*this, rhs);
        return *this;
    }
    */

    void push_back(int num) {
        // se necessito di spazio realloco memoria
        if (size_ == capacity_) {
            capacity_ *= 2;
            data_ = (int*)realloc(data_, capacity_ * sizeof(int));
        }
        //inserisco il dato
        data_[size_] = num;
        //incremento la dimensione
        size_++;
    }

    void sort() {
        qsort(data_, size_, sizeof(int), compare_integers);
    }

    const int& at(int i) const {
        assert(i >= 0 && i < size_);
        return data_[i];
    }
    int& at(int i) {
        assert(i >= 0 && i < size_);
        return data_[i];
    }
    const int& operator[](int i) const {
        assert(i >= 0 && i < size_);
        return data_[i];
    }
    int& operator[](int i) {
        assert(i >= 0 && i < size_);
        return data_[i];
    }

    size_t size() const {
        return size_;
    }
};
}
// funzione di stampa
void print_vector(FILE* fout, const mdp::vector& v) {
    for (int i = 0; i < v.size(); i++) {
        printf("%d\n", v[i]);
        fprintf(fout, "%d\n", v[i]);
    }
}

/*
RETURN VALUE OPTIMIZATION
se la funzione crea un oggetto e lo ritorna, questo viene assegnato ad un variabile del main,
non avrebbe senso copiarlo, quindi l'oggetto viene direttamente creato all'esterno della funzione (senza copie)
e poi la funzione ci lavora sopra normalmente, come se fosse interno.
in questo modo viene chiamato un solo costruttore
*/
mdp::vector read_vector(const char* filename) {
    mdp::vector v;
    FILE* fin = fopen(filename, "r");
    if (fin != NULL) {
        int num;
        while (fscanf(fin, "%d", &num) == 1) {
            v.push_back(num);
        }
        fclose(fin);
    }
    else {
        printf("Error: Unable to open input file %s\n", filename);
    }
    return v;
}

void swap_brutta(mdp::vector& x, mdp::vector& y) {

    mdp::vector tmp = (mdp::vector&&)x;
    x = (mdp::vector&&)y;
    y = (mdp::vector&&)tmp;
    /*
    se non faccio un cast esplicito a rvalue reference il compilatore chiama il metodo copy e non move
    per fare uno swap copierebbe 3 volte l'intero data_, che potrebbe pesare gigabyte, in questo modo ottimizzo
    in generale non serve, si può usare il move già definito nella libreria std
    */
    
    auto tmp2 = std::move(x);
    /*
    auto fa decidere al compilatore che tipo di variabile è tmp2 a seconda di cosa gli assegno, in questo caso poiché
    std::move() ritorna mdp::vector tmp2 è mdp::vector. quando possibile usare sempre auto
    */
    x = std::move(y);
    y = std::move(tmp2);
    
}

/*
in questa swap non creo un nuovo vector ma direttamente i suoi parametri,
in questo modo non verrà chiamato nessun costruttore.
questi assegnamenti genereranno un errore nel caso in cui gli attributi dovessero essere privati (lo sono) 
la swap definitiva è dentro a vector

void swap_meno_brutta(mdp::vector& x, mdp::vector& y) {
    
    size_t capacity;
    size_t size;
    int* data;

    x.capacity_ = y.capacity_;
    x.size_ = y.size_;
    x.data_ = y.data_;

    y.capacity_ = capacity;
    y.size_ = size;
    y.data_ = data;
}
*/


// "main"
int sort_namespace(char* filein, char* fileout) {

    using namespace mdp;
    /*
    non si scrive mai nei file headers, altrimenti dovrei importare tutto ogni volta che qualcuno usa i miei headers
    non si scrive mai fuori dalle funzioni
    */

    // Read integers from the input file
    vector v = read_vector(filein);
    
    FILE* fout = fopen(fileout, "w");
    if (fout == NULL) {
        fprintf(stderr, "Error: Unable to open output file %s\n", fileout);
        return 1;
    }

    // Sort the numbers
    v.sort();

    // Write sorted numbers to the output file
    print_vector(fout, v);
    fclose(fout);

    return 0;
}
