#define _CRT_SECURE_NO_WARNINGS

#include "sort_int.h"
#include "compare_integers.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// VERSIONE 3 DELL'ESERCIZIO, IN C++ CON STRUCT

struct vector {
    int* data_;
    size_t capacity_;
    size_t size_;

    //costruttore
    vector() {
        capacity_ = 10;
        size_ = 0;
        //il c++ è molto più rigoroso sui puntatori a void, serve un cast esplicito
        data_ = (int*)malloc(capacity_ * sizeof(int));
    }

    //distruttore
    ~vector() {
        free(data_);
    }
    //costruttore e distruttore non devono essere chiamati esplicitamente, avviene in automatico

    /*
    i metodi sono dentro alla struct, non c'è bisogno di specificare vector* this
    è sottointeso come primo parametro, se lo voglio const lo specifico fuori dalle parantesi
    */
    void push_back(int num) {
        // se necessito di spazio realloco memoria
        if (size_ == capacity_) {
            capacity_ *= 2;
            data_ = (int*)realloc(data_, capacity_ * sizeof(int));
        }
        // inserisco il dato
        data_[size_] = num;
        // incremento la dimensione
        size_++;
    }

    void sort() {
        qsort(data_, size_, sizeof(int), compare_integers);
    }

    int at(int i) const {
        assert(i >= 0 && i < size_);
        return data_[i];
    }
    int operator[](int i) const {
        assert(i >= 0 && i < size_);
        return data_[i];
    }

    // getter
    size_t size() const {
        return size_;
    }
};

// "main"
int sort_int(char* filein, char* fileout) {

    FILE* fin = fopen(filein, "r");
    if (fin == NULL) {
        fprintf(stderr, "Error: Unable to open input file %s\n", filein);
        return 1;
    }

    FILE* fout = fopen(fileout, "w");
    if (fout == NULL) {
        fprintf(stderr, "Error: Unable to open output file %s\n", fileout);
        fclose(fin);
        return 1;
    }

    vector v;

    // Read integers from the input file
    int num;
    while (fscanf(fin, "%d", &num) == 1) {
        v.push_back(num);
    }

    // Sort the numbers
    v.sort();

    // Write sorted numbers to the output file
    for (int i = 0; i < v.size(); i++) {
        printf("%d\n", v[i]);
        fprintf(fout, "%d\n", v[i]);
        // v.operator[](i) è identico a v[i] e fa la stessa cosa di v.at(i)
    }

    fclose(fin);
    fclose(fout);

    return 0;
}
