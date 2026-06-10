#define _CRT_SECURE_NO_WARNINGS
#include "old_school_sort.h"
#include "compare_integers.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

//"main" 
int old_school_sort(char *infile, char *outfile) {

    printf("entered the old_school_sort.c\n");

    // open read file
    FILE* fin = fopen(infile, "r");
    if (fin == NULL) {
        fprintf(stderr, "Error: Unable to open input file %s\n", infile);
        return 1;
    }

    // open write file
    FILE* fout = fopen(outfile, "w");
    if (fout == NULL) {
        fprintf(stderr, "Error: Unable to open output file %s\n", outfile);
        fclose(fin);
        return 1;
    }    

    // Allocate memory for numbers array
    int* numbers = NULL;
    const int MAX_NUMBERS = 1000000; // Maximum number of elements
    numbers = (int*)malloc(MAX_NUMBERS * sizeof(int));
    if (numbers == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(fin);
        fclose(fout);
        return 1;
    }
    
    // Read integers from the input file
    int num;
    int numCount = 0;
    while (fscanf(fin, "%d", &num) == 1){
        numbers[numCount++] = num;
        if (numCount >= MAX_NUMBERS) {
            printf("Warning: Maximum number of elements exceeded. Ignoring the rest.\n");
            break;
        }
    }
    // Sort the numbers
    qsort(numbers, numCount, sizeof(int), compare_integers);

    // Write sorted numbers to the output file
    for (int i = 0; i < numCount; i++) {
        printf("%d\n", numbers[i]);
        fprintf(fout, "%d\n", numbers[i]);
    }

    // Free dynamically allocated memory
    free(numbers);

    fclose(fin);
    fclose(fout);

    return 0;
}
