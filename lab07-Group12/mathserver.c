#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>


#define CONTEXTS 16

__uint32_t contexts[CONTEXTS];
__uint8_t current_context = 00;
FILE* output_file;



char** tokenize_input(char* input);
void set(__uint32_t context, __uint32_t val);
void add(__uint32_t context, __uint32_t val);
void sub(__uint32_t context, __uint32_t val);
void mul(__uint32_t context, __uint32_t val);
void divide(__uint32_t context, __uint32_t val);
__uint32_t fib(__uint32_t context);
void pri(__uint32_t context);
void pia(__uint32_t context);




int main(int argc, char* argv[]) {
    const char usage[] = "Usage: mathserver.out <input trace> <output trace>\n";
    char* input_trace;
    char* output_trace;
    char buffer[1024];

    // Parse command line arguments
    if (argc != 3) {
        printf("%s", usage);
        return 1;
    }
    
    input_trace = argv[1];
    output_trace = argv[2];


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

        // TODO: Implement your memory simulator
        if (tokens[0] == NULL || tokens[0][0] == '%') {
            continue;
        }

        if(strcmp(tokens[0], "set") == 0){
            set(atoi(tokens[1]), atoi(tokens[2]));
        }
        else if(strcmp(tokens[0], "sub") == 0){
            sub(atoi(tokens[1]), atoi(tokens[2]));
        }

        else if(strcmp(tokens[0], "add") == 0){
            add(atoi(tokens[1]), atoi(tokens[2]));
        }
               
        else if(strcmp(tokens[0], "div") == 0){
            divide(atoi(tokens[1]), atoi(tokens[2]));
        }
                
        else if(strcmp(tokens[0], "mul") == 0){
            mul(atoi(tokens[1]), atoi(tokens[2]));
        }
        else if(strcmp(tokens[0], "fib") == 0){
            fprintf(output_file, "ctx %02d: fib (result: %d)\n", current_context, fib(contexts[atoi(tokens[1])]));
        }

        else if(strcmp(tokens[0], "pri") == 0){
            pri(atoi(tokens[1]));
        }
        else if(strcmp(tokens[0], "pia") == 0){
            pia(atoi(tokens[1]));
        }

        for (int i = 0; tokens[i] != NULL; i++){
            free(tokens[i]);
        }
        free(tokens);
    }

    fclose(input_file);
    fclose(output_file);


    return 0;
}


void set(__uint32_t context, __uint32_t val){
    contexts[context] = val;
    fprintf(output_file, "ctx %02d: set to value %d\n", context, val);
}

void add(__uint32_t context, __uint32_t val){
    contexts[context] += val;
    fprintf(output_file, "ctx %02d: add %d (result: %d)\n", context, val, contexts[context]);


}

void sub(__uint32_t context, __uint32_t val){
    contexts[context] -= val;
    fprintf(output_file, "ctx %02d: sub %d (result: %d)\n", context, val, contexts[context]);
}


void mul(__uint32_t context, __uint32_t val){
    contexts[context] *= val;
    fprintf(output_file, "ctx %02d: mul %d (result: %d)\n", context, val, contexts[context]);
}



void divide(__uint32_t context, __uint32_t val){
    contexts[context] = contexts[context]/val;
    fprintf(output_file, "ctx %02d: div %d (result: %d)\n", context, val, contexts[context]);
}

__uint32_t fib(__uint32_t val){
    if(val <= 0){
        return 0;
    }
    else if(val == 1){
        return 1;
    }
    else{
        return fib(val - 1) + fib(val - 2);
    }
}

void pri(__uint32_t context) {
    __uint32_t N = contexts[context];

    fprintf(output_file, "ctx %02d: pri (result:", context);

    int first = 1;
    for (__uint32_t n = 2; n <= N; n++) {
        int prime = 1;

        for (__uint32_t d = 2; d * d <= n; d++) {
            if (n % d == 0) {
                prime = 0;
                break;
            }
        }
        if (prime) {
            if (!first)
                fprintf(output_file, ",");
            fprintf(output_file, " %u", n);
            first = 0;
        }
    }

    fprintf(output_file, ")\n");
}


void pia(__uint32_t ctx) {
    __uint32_t iterations = contexts[ctx];
    double pi = 0.0;

    for (__uint32_t k = 0; k < iterations; k++) {
        if (k % 2 == 0) {
            pi += 1.0 / (2 * k + 1);
        } else {
            pi -= 1.0 / (2 * k + 1);
        }
    }

    pi *= 4.0;

    fprintf(output_file, "ctx %02d: pia (result %.15f)\n", ctx, pi);
}


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
