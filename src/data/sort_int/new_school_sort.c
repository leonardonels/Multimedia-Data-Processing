#define _CRT_SECURE_NO_WARNINGS
#include "new_school_sort.h"
#include "compare_integers.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// VERSIONE 2 DELL'ESERCIZIO, CON STRUCT IN C

typedef struct vector {
    int* data_;
    size_t capacity_;
    size_t size_;
} vector;

void vector_constructor(vector* this) {
    this->capacity_ = 10;
    this->size_ = 0;
    this->data_ = malloc(this->capacity_ * sizeof(int));
}

void vector_destructor(vector* this) {
    free(this->data_);
}

void vector_push_back(vector* this, int num) {
    // se necessito di spazio realloco memoria
    if (this->size_ == this->capacity_) {
        this->capacity_ *= 2;
        this->data_ = realloc(this->data_, this->capacity_ * sizeof(int));
    }
    //inserisco il dato
    this->data_[this->size_] = num;
    //incremento la dimensione
    this->size_++;
}

void vector_sort(vector* this) {
    qsort(this->data_, this->size_, sizeof(int), compare_integers);
}

int vector_at(const vector* this, int i) {
    assert(i >= 0 && i < this->size_);
    return this->data_[i];
}

size_t vector_size(const vector* this) {
    return this->size_;
}

//"main"
int new_school_sort(char* infile, char* outfile) {

    FILE* fin = fopen(infile, "r");
    if (fin == NULL) {
        fprintf(stderr, "Error: Unable to open input file %s\n", infile);
        return 1;
    }

    FILE* fout = fopen(outfile, "w");
    if (fout == NULL) {
        fprintf(stderr, "Error: Unable to open output file %s\n", outfile);
        fclose(fin);
        return 1;
    }

    vector v;
    vector_constructor(&v);

    // Read integers from the input file
    int num;
    while (fscanf(fin, "%d", &num) == 1) {
        //printf("%d\n", num);
        vector_push_back(&v, num);
    }

    // Sort the numbers
    //printf("%d\n", vector_at(&v, 0));
    vector_sort(&v);
    //printf("%d\n", vector_at(&v, 0));

    // Write sorted numbers to the output file
    for (int i = 0; i < vector_size(&v); i++) {
        printf("%d\n", vector_at(&v, i));
        fprintf(fout, "%d\n", vector_at(&v, i));
    }

    // Free dynamically allocated memory
    vector_destructor(&v);

    fclose(fin);
    fclose(fout);

    return 0;
}
