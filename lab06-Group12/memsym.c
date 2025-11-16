#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#define TRUE 1
#define FALSE 0

// Output file
FILE* output_file;

// TLB replacement strategy (FIFO or LRU)
char* strategy;


// Global variables
__uint32_t OFF;
__uint32_t PFN;
__uint32_t VPN;

void define(__uint32_t off,__uint32_t pfn, __uint32_t vpn);

char** tokenize_input(char* input) {
    char** tokens = NULL;
    char* token = strtok(input, " ");
    int num_tokens = 0;

    while (token != NULL) {
        num_tokens++;
        tokens = realloc(tokens, num_tokens * sizeof(char*));
        tokens[num_tokens - 1] = malloc(strlen(token) + 1);
        strcpy(tokens[num_tokens - 1], token);
        token = strtok(NULL, " ");
    }

    num_tokens++;
    tokens = realloc(tokens, num_tokens * sizeof(char*));
    tokens[num_tokens - 1] = NULL;

    return tokens;
}

int main(int argc, char* argv[]) {
    const char usage[] = "Usage: memsym.out <strategy> <input trace> <output trace>\n";
    char* input_trace;
    char* output_trace;
    char buffer[1024];

    // Parse command line arguments
    if (argc != 4) {
        printf("%s", usage);
        return 1;
    }
    strategy = argv[1];
    input_trace = argv[2];
    output_trace = argv[3];

    // Open input and output files
    FILE* input_file = fopen(input_trace, "r");
    output_file = fopen(output_trace, "w");  

    while ( !feof(input_file) ) {
        // Read input file line by line
        char *rez = fgets(buffer, sizeof(buffer), input_file);
        if ( !rez ) {
            fprintf(stderr, "Reached end of trace. Exiting...\n");
            return -1;
        } else {
            // Remove endline character
            buffer[strlen(buffer) - 1] = '\0';
        }
        char** tokens = tokenize_input(buffer);

        
        //
        // TODO: Implement your memory simulator
        //

        if (strcmp(tokens[0], "empty trace") == 0) return -1; 


        // Search for define key word
        char** temp = tokens; 
        // while (strcmp(*temp, "define") != 0) temp++;

        if (strcmp(*temp, "define") == 0){
            define(atoi(temp[1]), atoi(temp[2]), atoi(temp[3]));
            
        }
        
        
        




        // Deallocate tokens
        for (int i = 0; tokens[i] != NULL; i++)
            free(tokens[i]);
        free(tokens);
    }

    // Close input and output files
    fclose(input_file);
    fclose(output_file);

    return 0;
}



void define(__uint32_t off,__uint32_t pfn, __uint32_t vpn){
    OFF = off; 
    PFN = pfn; 
    VPN = vpn; 

    fprintf(output_file, "Current PID: 0. Memory instantiation complete. OFF bits: %d. PFN bits: %d. VPN bits: %d\n", off, pfn, vpn);
}




