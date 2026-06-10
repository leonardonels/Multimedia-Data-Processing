#define _CRT_SECURE_NO_WARNINGS

#include "sort_prog_gen.h"
#include <cassert>
#include <crtdbg.h>
#include <utility>
#include <iostream>
#include <fstream>
#include <sstream>
/*  con le architetture di oggi sono praticamente sempre più veloci gli array poichè sono contigui e la cache 
carica facilmente svariati valori in memoria    */
#include <vector>
#include <list>     // lista doppiamente concatenata
#include <algorithm>

// VERSIONE 7 DELL'ESERCIZIO, CON PROGRAMMAZIONE GENERICA e LIBRERIA STANDARD

std::vector<double> read_vector(const char* filename) {
    std::vector<double> q;
    std::ifstream is(filename);

    if (!is) {
        return q;
    }

    double num;
    while (is >> num) {
        q.push_back(num);
    }
    return q;
}

// "main"
int sort_prog_gen(char* filein, char* fileout) {

    using std::vector;

    // Read integers from the input file
    vector<double> v = read_vector(filein);

    std::ofstream os(fileout/*, std::ios::binary*/);
    if (!os) {
        return EXIT_FAILURE;
    }
 
    // Lettura dei valori di un array

    // 1 versione: lunga, brutta e sfigata
    // tutti i contenitori della libreria standard hanno metodi begin e end per avere "inidice"/"oggetto" iniziale e finale
    vector<double>::iterator start = v.begin();      // begin punta al primo elemento
    vector<double>::iterator stop = v.end();     // end punta all'elemento dopo l'ultimo, non appartiene alla sequenza
    vector<double>::iterator current;
    /*    l'interfaccia degli iteratori è quella dei puntatori, gli iteratori sono oggetti che fanno varie cose, 
    nel caso di un array un iteratore è un puntatore, per strutture più complesse sono oggetti complessi    */
    current = start;
    while (current != stop) {
        // con gli iteratori si usa il !=, MAI usare il <
        double x = *current;
        os << x << '\n';
        ++current;
    }
    
    // se modifico un contenitore mentre ci sto iterando sopra gli iteratori vengono invalidati (vanno a puttane), non si fa
    
    // 2 versione: codice con c++11, il nome più classico per gli iteratori è it
    for (auto it = v.begin(), stop = v.end(); it != stop; ++it) {
        const double& x = *it;
        os << x << '\n';
    }

    // versione 3: si può fare ancora meglio... Range Based For, x scorre tutti gli elementi del vettore v, come il python :)
    for (const auto& x : v) {
        os << x << '\n';
    }

    // Copia di un vettore
    vector<double> x;
    x.resize(4);
    for (size_t i = 0; i < x.size(); ++i) {
        x[i] = v[i];
    }   // fa schifo, è lentissima

    memcpy(&x[0], &v[0], 4 * sizeof(double));
    /* molto più veloce perchè memcpy è una implementazione del codice macchina di una copia di byte
    in ogni caso se il mio oggetto non è un double ma tipo un array di array, la copia è shallow, quindi fa schifo */

    std::copy(x.begin(), x.end(), v.begin());       // si fa così
    std::copy(x.begin(), x.end(), std::back_inserter(v));       // oppure così
    // adaptor, creano iteratori a partire da oggetti, back_inserter fa la pushback

    /*
    contenitori pronti da usare in libreria standard: 
    sequenziali: array, vector, deque, list, forward_list
    associativi (alberi binari): set, map, multiset, multimap
    associativi non ordinati (più veloci, con hashmap): unordered_set, unordered_map, unordered_multiset, unordered_multimap
    adaptor (interfacce): stack (push e pop), queue (coda fifo), priority_queue (heap)
    */

    for (size_t i = 0; i < v.size(); i++) {
        os << v[i] << '\n';
        std::cout << v[i] << '\n';
    }

    return 0;
}
