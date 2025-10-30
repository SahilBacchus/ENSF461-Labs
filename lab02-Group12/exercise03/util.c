#include "util.h"
#include <stdlib.h>
#include <math.h>

int* read_next_line(FILE* fin) {
    // TODO: This function reads the next line from the input file
    // The line is a comma-separated list of integers
    // Return the list of integers as an array where the first element
    // is the number of integers in the rest of the array
    // Return NULL if there are no more lines to read
    if (feof(fin)) {
        return NULL;
    }

    int capacity = 20;
    int *arr = malloc(capacity * sizeof(int));
    if (arr == NULL) {
        return NULL;
    }
    
    int count = 0;
    int value;
    char ch;
    
    // Read first number
    if (fscanf(fin, "%d", &value) != 1) {
        free(arr);
        return NULL;
    }
    arr[++count] = value;
    
    // Read remaining numbers separated by commas
    while (fscanf(fin, ",%d", &value) == 1) {
        if (count + 1 >= capacity) {
            capacity *= 2;
            int *temp = realloc(arr, capacity * sizeof(int));
            if (temp == NULL) {
                free(arr);
                return NULL;
            }
            arr = temp;
        }
        arr[++count] = value;
    }
    
    // Clear the newline or EOF
    fscanf(fin, "%*[^\n]");
    fgetc(fin); // consume the newline
    
    if (count == 0) {
        free(arr);
        return NULL;
    }
    
    arr[0] = count;
    return arr;

}


float compute_average(int* line) {
    // TODO: Compute the average of the integers in the vector
    // Recall that the first element of the vector is the number of integers
    float sum = 0.0;
    for ( int i = 1; i <= line[0]; i++ ) {
        sum += line[i];
    }
    return sum / line[0];

}


float compute_stdev(int* line) {
    // TODO: Compute the standard deviation of the integers in the vector
    // Recall that the first element of the vector is the number of integers
    float avg = compute_average(line);
    float sum = 0.0;
    for ( int i = 1; i <= line[0]; i++ ) {
        sum += (line[i] - avg) * (line[i] - avg);
    }
    return sqrt(sum / line[0]);


}