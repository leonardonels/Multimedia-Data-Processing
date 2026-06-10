// main.c
#include "old_school_sort.h"
#include "new_school_sort.h"
#include "sort_with_ref.h"
#include "sort_int.h"
#include "sort_namespace.h"
#include "sort_stream.h"
#include "sort_prog_gen.h"
#include "sort_lambda.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {

    if (argc != 3) {
        printf("Usage: %s <filein.txt> <fileout.txt>\n", argv[0]);
        return 1;
    }
    printf("infile: %s, outfile: %s\n", __argv[1], __argv[2]);

    int ret = 0;
    //ret = old_school_sort(__argv[1], __argv[2]);
    
    //ret = new_school_sort(__argv[1], __argv[2]);

    //ret = sort_int(__argv[1], __argv[2]);

    //ret = sort_with_ref(__argv[1], __argv[2]);

    //ret = sort_namespace(__argv[1], __argv[2]);

    //ret = sort_stream(__argv[1], __argv[2]);

    //ret = sort_prog_gen(__argv[1], __argv[2]);

    ret = sort_lambda(__argv[1], __argv[2]);

    return ret;
}
