#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#define CONTEXTS 16
#define SIZE 1000
#define BATCH 10

__uint32_t contexts[CONTEXTS];
FILE* output_file;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct Operation {
    char* type;
    int arg1;
    int arg2;
} Operation;

typedef struct ContextQueue {
    Operation* operations;
    int size;
    int capacity;
    int front;
    int rear;
} ContextQueue;

ContextQueue context_queues[CONTEXTS];

void init_queue(ContextQueue* queue) {
    queue->operations = malloc(SIZE * sizeof(Operation));
    queue->size = 0;
    queue->capacity = SIZE;
    queue->front = 0;
    queue->rear = -1;
}

void enqueue(ContextQueue* queue, Operation op) {
    if (queue->size == queue->capacity) {
        queue->capacity *= 2;
        queue->operations = realloc(queue->operations, queue->capacity * sizeof(Operation));
    }
    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->operations[queue->rear] = op;
    queue->size++;
}

Operation dequeue(ContextQueue* queue) {
    Operation op = queue->operations[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;
    queue->size--;
    return op;
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

void set(__uint32_t ctx, __uint32_t val){
    contexts[ctx] = val;
    fprintf(output_file, "ctx %02d: set to value %d\n", ctx, val);
}

void add(__uint32_t ctx, __uint32_t val){
    contexts[ctx] += val;
    fprintf(output_file, "ctx %02d: add %d (result: %d)\n", ctx, val, contexts[ctx]);
}

void sub(__uint32_t ctx, __uint32_t val){
    contexts[ctx] -= val;
    fprintf(output_file, "ctx %02d: sub %d (result: %d)\n", ctx, val, contexts[ctx]);
}

void mul(__uint32_t ctx, __uint32_t val){
    contexts[ctx] *= val;
    fprintf(output_file, "ctx %02d: mul %d (result: %d)\n", ctx, val, contexts[ctx]);
}

void divide(__uint32_t ctx, __uint32_t val){
    contexts[ctx] /= val;
    fprintf(output_file, "ctx %02d: div %d (result: %d)\n", ctx, val, contexts[ctx]);
}

bool is_prime(__uint32_t n){
    if (n == 1 || n == 0) return false;
    for (int i = 2; i <= sqrt(n); i++) {
        if (n % i == 0) return false;
    }
    return true;
}

void pri(__uint32_t context) {
    __uint32_t N = contexts[context];
    if (N < 2) {
        fprintf(output_file, "ctx %02d: primes (result:)\n", context);
        return;
    }

    bool* is_prime_arr = malloc((N + 1) * sizeof(bool));
    for (__uint32_t i = 0; i <= N; i++) is_prime_arr[i] = true;
    is_prime_arr[0] = false;
    is_prime_arr[1] = false;

    for (__uint32_t i = 2; i * i <= N; i++) {
        if (is_prime_arr[i]) {
            for (__uint32_t j = i * i; j <= N; j += i)
                is_prime_arr[j] = false;
        }
    }

    fprintf(output_file, "ctx %02d: primes (result:", context);
    int first = 1;
    for (__uint32_t i = 2; i <= N; i++) {
        if (is_prime_arr[i]) {
            if (!first) fprintf(output_file, ",");
            fprintf(output_file, " %u", i);
            first = 0;
        }
    }
    fprintf(output_file, ")\n");

    free(is_prime_arr);
}


__uint32_t fib(__uint32_t val) {
    if (val == 0) return 0;
    if (val == 1) return 1;

    __uint32_t a = 0, b = 1, c;
    for (__uint32_t i = 2; i <= val; i++) {
        c = a + b;
        a = b;
        b = c;
    }
    return b;
}
void pia(__uint32_t ctx) {
    __uint32_t iterations = contexts[ctx];
    double pi = 0.0;
    for (__uint32_t k = 0; k < iterations; k++) {
        if (k % 2 == 0) pi += 1.0 / (2 * k + 1);
        else pi -= 1.0 / (2 * k + 1);
    }
    pi *= 4.0;
    fprintf(output_file, "ctx %02d: pia (result %.15f)\n", ctx, pi);
}

void execute_operation(Operation op, int ctx) {
    if (strcmp(op.type, "set") == 0) set(ctx, op.arg2);
    else if (strcmp(op.type, "add") == 0) add(ctx, op.arg2);
    else if (strcmp(op.type, "sub") == 0) sub(ctx, op.arg2);
    else if (strcmp(op.type, "mul") == 0) mul(ctx, op.arg2);
    else if (strcmp(op.type, "div") == 0) divide(ctx, op.arg2);
    else if (strcmp(op.type, "fib") == 0) fprintf(output_file, "ctx %02d: fib (result: %u)\n", ctx, fib(contexts[ctx]));
    else if (strcmp(op.type, "pri") == 0) pri(ctx);
    else if (strcmp(op.type, "pia") == 0) pia(ctx);
}

void* process_operations(void* arg) {
    int ctx = *(int*)arg;
    while (context_queues[ctx].size > 0) {
        Operation op = dequeue(&context_queues[ctx]);
        pthread_mutex_lock(&log_mutex);
        execute_operation(op, ctx);
        pthread_mutex_unlock(&log_mutex);
        free(op.type);
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: mathserver.out <input trace> <output trace>\n");
        return 1;
    }

    char* input_trace = argv[1];
    char* output_trace = argv[2];

    FILE* input_file = fopen(input_trace, "r");
    output_file = fopen(output_trace, "w");

    for (int i = 0; i < CONTEXTS; i++) init_queue(&context_queues[i]);

    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), input_file)) {
        buffer[strcspn(buffer, "\n")] = '\0';
        if (buffer[0] == '%' || strlen(buffer) == 0) continue;

        char** tokens = tokenize_input(buffer);
        if (tokens[0] == NULL) continue;

        int ctx = atoi(tokens[1]);
        Operation op;
        op.type = strdup(tokens[0]);
        op.arg1 = ctx;
        op.arg2 = (tokens[2] != NULL) ? atoi(tokens[2]) : 0;
        enqueue(&context_queues[ctx], op);

        for (int i = 0; tokens[i] != NULL; i++) free(tokens[i]);
        free(tokens);
    }

    fclose(input_file);

    pthread_t threads[CONTEXTS];
    int ctx_ids[CONTEXTS];
    for (int i = 0; i < CONTEXTS; i++) {
        ctx_ids[i] = i;
        pthread_create(&threads[i], NULL, process_operations, &ctx_ids[i]);
    }

    for (int i = 0; i < CONTEXTS; i++) pthread_join(threads[i], NULL);

    fclose(output_file);
    return 0;
}