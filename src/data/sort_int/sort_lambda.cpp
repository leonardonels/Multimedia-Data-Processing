#define _CRT_SECURE_NO_WARNINGS

#include "sort_lambda.h"
#include <crtdbg.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <iterator>

// VERSIONE 8 DELL'ESERCIZIO, CON FUNZIONI LAMBDA E CLASS

/*
ENCLOSURE:
"enclosure" (o "incapsulamento") in un contesto di programmazione si riferisce tipicamente all'atto di racchiudere 
un blocco di codice all'interno di un altro blocco di codice. Questo viene comunemente realizzato utilizzando le 
parentesi graffe {} per creare uno scope.

FUNCTORS:
Un funtore è un oggetto che può essere trattato come se fosse una funzione, poiché può essere invocato utilizzando 
l'operatore di chiamata di funzione (). Questo rende possibile passare oggetti funtore a funzioni o usarli in 
espressioni funzionali.

CLASS:
è una struct dove tutto di default è privato

LAMBDA FUNCTION:
[catture](parametri) -> tipo_di_ritorno { corpo_funzione }
[catture] specifica quali variabili esterne (variabili esterne alla lambda) verranno catturate e come.
(parametri) specifica i parametri della funzione lambda, come le funzioni regolari.
-> tipo_di_ritorno specifica il tipo di ritorno della funzione lambda (opzionale se il compilatore può dedurlo).
{ corpo_funzione } contiene il corpo della funzione lambda, che è l'implementazione della funzione stessa.
*/

class Widget {
    int& i;
    int j = 42;     // se inizializzo una variabile di classe questa verrà inizializzata come fosse nel costruttore
public:
    Widget(int& val) : i{ val } {       // inizializzazione di reference
    }
};


// "main"
int sort_lambda(char* filein, char* fileout) {

    using std::vector;

    std::ifstream is(filein);
    if (!is) {
        return EXIT_FAILURE;
    }

    // cast esplicito di const
    const int x = 10;
    //x = 5;
    //(int)x = 5;
    *(int*)&x = 5;      // c style
    const_cast<int&>(x) = 5;    // c++ style


    std::istream_iterator<double> is_iter(is);
    std::istream_iterator<double> is_iter_end;
    std::vector<double> v(is_iter, is_iter_end);

    /*   THE MOST VEXING PARSE (PROBLEM)
    Il "most vexing parse" è una situazione ambigua che può verificarsi in C++ durante la dichiarazione di oggetti di classi con 
    costruttori che possono sembrare chiamate di funzioni. Questo problema può portare a risultati non intuitivi o indesiderati.
    In genere si verifica quando si tenta di dichiarare un oggetto di una classe utilizzando una coppia di parentesi vuote () che 
    sembrano indicare una chiamata di funzione, ma in realtà vengono interpretate come una dichiarazione di una funzione. Questo 
    comportamento può essere sorprendente per i programmatori che non sono a conoscenza di questa caratteristica del linguaggio.    */
    std::vector<double> w{ std::istream_iterator<double>(is), std::istream_iterator<double>() };

    std::sort(std::begin(v), std::end(v), [](double a, double b) {return a > b;});
    // algoritmo che funziona solo con RanIt, iteratori random access, cioè non liste
    // ADL, non mi serve specificare std::begin e std::end visto vhe v è un std::vector e 
    // comincio a cercare le funzioni in quel namespace

    /*  ADL (Argument-Dependent Lookup), noto anche come Koenig Lookup, è un meccanismo nel linguaggio di programmazione C++ che 
    viene utilizzato per cercare funzioni e operatori non membro in uno scope in base agli argomenti passati a una funzione.
    Quando si chiama una funzione o un operatore non membro all'interno di una classe e gli argomenti della funzione o 
    dell'operatore non sono del tipo della classe stessa, il compilatore cercherà di risolvere la funzione o l'operatore anche 
    nello scope degli argomenti. Questo comportamento si verifica automaticamente senza la necessità di utilizzare l'operatore 
    di risoluzione di ambito ::    */

    std::ofstream os(fileout/*, std::ios::binary*/);
    if (!os) {
        return EXIT_FAILURE;
    }
    for (const auto& x : v) {
        os << x << '\n';
        std::cout << x << '\n';
    }

    //std::copy(begin(v), end(v), std::back_inserter(x));       
    
    return 0;
}
/*
Tipi di cast

C-style Cast: Questo è il più generico e può essere usato per convertire un tipo di dato in un altro. 
Tuttavia, è considerato meno sicuro rispetto agli altri cast in quanto il compilatore non effettua controlli 
di tipo durante la compilazione.
int a = 10;
double b = (double)a; // Cast esplicito da int a double


Static Cast: Questo è il cast preferito in C++ perché è più sicuro rispetto al C-style cast. Viene controllato a 
tempo di compilazione e offre conversioni implicithe tra tipi compatibili.
int a = 10;
double b = static_cast<double>(a); // Cast esplicito da int a double

Dynamic Cast: Questo è utilizzato principalmente per conversioni tra tipi di puntatori o riferimenti in gerarchie di classi 
polimorfiche. Effettua un controllo a tempo di esecuzione per garantire la correttezza della conversione.
Base* basePtr = new Derived();
Derived* derivedPtr = dynamic_cast<Derived*>(basePtr); // Cast esplicito da Base* a Derived*
if (derivedPtr) {
    // Conversione riuscita
} else {
    // Conversione fallita
}

Const Cast: Questo viene utilizzato per rimuovere la qualifica di const o volatile da un tipo di dato. È rischioso 
e può portare a comportamenti indefiniti se usato in modo errato.
const int a = 10;
int& b = const_cast<int&>(a); // Rimuove il const da a

Reinterpret Cast: Questo è il più pericoloso e dovrebbe essere utilizzato solo quando si è completamente consapevoli 
delle implicazioni. Esegue una conversione bit-a-bit di un oggetto da un tipo a un altro.
int a = 65;
char b = reinterpret_cast<char>(a); // Cast esplicito da int a char
*/