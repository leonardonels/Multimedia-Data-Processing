#include "compare_integers.h"
#include <stdio.h>
#include <stdlib.h>

// Function to compare integers for qsort
int compare_integers(const void* a, const void* b)
{
    int ia = *(int*)a;
    int ib = *(int*)b;
    if (ia < ib) {
        return -1;
    }
    else if (ia > ib) {
        return 1;
    }
    else {
        return 0;
    }
}