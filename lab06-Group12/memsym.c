#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define TRUE 1
#define FALSE 0
#define TLB_SIZE 8
#define NUM_PROCESSES 4

typedef struct {
    u_int32_t pfn;
    int valid;
} pt_entry_t;

// TLB entry
typedef struct {
    u_int32_t vpn;
    u_int32_t pfn;
    int valid;
    int pid;
    unsigned int timestamp;
} tlb_entry_t;

// Globals
u_int32_t* physical_memory = NULL;
pt_entry_t* page_tables[NUM_PROCESSES] = {NULL};
tlb_entry_t tlb[TLB_SIZE];

u_int32_t OFF;
u_int32_t PFN;
u_int32_t VPN;
int defined = 0;
int current_pid = 0;
unsigned int global_timestamp = 0;

typedef struct {
    u_int32_t PTBR;
    u_int32_t TLB_LRU;
} cpu_state_t;

cpu_state_t saved_state[4];
cpu_state_t current_state;

// Output file
FILE* output_file;

// TLB replacement strategy (FIFO or LRU)
char* strategy;

u_int32_t r1[NUM_PROCESSES] = {0};
u_int32_t r2[NUM_PROCESSES] = {0};

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


// define <OFF> <PFN> <VPN> Initialize the simulator with the memory parameters
void define(u_int32_t off, u_int32_t pfn, u_int32_t vpn) {
    if (defined) {
        fprintf(output_file, "Current PID: %d. Error: multiple calls to define in the same trace\n", current_pid);
        exit(0);
    }

    OFF = off;
    PFN = pfn;
    VPN = vpn;
    defined = 1;

    u_int32_t phys_size = 1 << (OFF + PFN);
    physical_memory = (u_int32_t*)calloc(phys_size, sizeof(u_int32_t));
    if (!physical_memory) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    u_int32_t pt_size = 1 << VPN;
    for (int i = 0; i < NUM_PROCESSES; i++) {
        page_tables[i] = (pt_entry_t*)malloc(pt_size * sizeof(pt_entry_t));
        if (!page_tables[i]) {
            fprintf(stderr, "Page table allocation failed\n");
            exit(1);
        }
        for (u_int32_t j = 0; j < pt_size; j++) {
            page_tables[i][j].valid = 0;
            page_tables[i][j].pfn = 0;
        }
    }

    for (int i = 0; i < TLB_SIZE; i++) {
        tlb[i].valid = 0;
        tlb[i].vpn = 0;
        tlb[i].pfn = 0;
        tlb[i].pid = -1;
    }

    global_timestamp++;

    fprintf(output_file, "Current PID: %d. Memory instantiation complete. OFF bits: %u. PFN bits: %u. VPN bits: %u\n", current_pid, OFF, PFN, VPN);
}




// ctxswitch <pid> Set the process <pid> as the one currently executing, saving existing state (e.g. register values)
void ctxswitch(int pid) {
    if (pid < 0 || pid > 3) {
        fprintf(output_file, "Current PID: %d. Invalid context switch to process %d\n", current_pid, pid);
        exit(0);
    }
    saved_state[current_pid] = current_state;
    current_state = saved_state[pid];

    current_pid = pid;
    
    global_timestamp++;

    fprintf(output_file, "Current PID: %d. Switched execution context to process: %d\n", current_pid, pid);
}




//---- helper functions ----// 

u_int32_t* get_register(char* reg) {
    if (strcmp(reg, "r1") == 0) return &r1[current_pid];
    if (strcmp(reg, "r2") == 0) return &r2[current_pid];

    fprintf(output_file, "Current PID: %d. Error: invalid register operand %s\n", current_pid, reg);
    exit(0);
}


u_int32_t translate(u_int32_t vaddr) {
    u_int32_t vpn = vaddr >> OFF;
    u_int32_t offset = vaddr & ((1 << OFF) - 1);
    u_int32_t pfn = 0;
    int tlb_hit = 0;
    int tlb_index = -1;

    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].valid && tlb[i].vpn == vpn && tlb[i].pid == current_pid) {
            tlb_hit = 1;
            pfn = tlb[i].pfn;
            tlb_index = i;
            break;
        }
    }

    if (tlb_hit) {
        fprintf(output_file, "Current PID: %d. Translating. Lookup for VPN %u hit in TLB entry %d. PFN is %u\n",
                current_pid, vpn, tlb_index, pfn);
        
        global_timestamp++;

        if (strcmp(strategy, "LRU") == 0) {
            tlb[tlb_index].timestamp = global_timestamp;
        }
    } else {
        fprintf(output_file, "Current PID: %d. Translating. Lookup for VPN %u caused a TLB miss\n", current_pid, vpn);

        if (!page_tables[current_pid][vpn].valid) {
            fprintf(output_file, "Current PID: %d. Translating. Translation for VPN %u not found in page table\n",
                    current_pid, vpn);
            exit(1);
        }

        pfn = page_tables[current_pid][vpn].pfn;
        fprintf(output_file, "Current PID: %d. Translating. Successfully mapped VPN %u to PFN %u\n",
                current_pid, vpn, pfn);

        int free_index = -1;
        for (int i = 0; i < TLB_SIZE; i++) {
            if (!tlb[i].valid) {
                free_index = i;
                break;
            }
        }

        if (free_index == -1) {
            free_index = 0;
            for (int i = 1; i < TLB_SIZE; i++) {
                if (tlb[i].timestamp < tlb[free_index].timestamp) {
                    free_index = i;
                }
            }
        }

        tlb[free_index].valid = 1;
        tlb[free_index].vpn = vpn;
        tlb[free_index].pfn = pfn;
        tlb[free_index].pid = current_pid;

        global_timestamp++;
        tlb[free_index].timestamp = global_timestamp;

        fprintf(output_file, "Current PID: %d. Translating. Loaded PFN %u into TLB entry %d\n",
                current_pid, pfn, free_index);
    }

    u_int32_t phys_addr = (pfn << OFF) | offset;
    return phys_addr;

}






// load <dst> <src> Load the value of <src> into register <dst>
void load(char* dst, char* src) {
    u_int32_t* reg = get_register(dst);
    if (src[0] == '#') {
        *reg = atoi(src + 1);
        fprintf(output_file, "Current PID: %d. Loaded immediate %u into register %s\n",
                current_pid, *reg, dst);
    } else {
        u_int32_t vaddr = atoi(src);
        u_int32_t phys_addr = translate(vaddr);
        *reg = physical_memory[phys_addr];
        fprintf(output_file, "Current PID: %d. Loaded value of location %u (%u) into register %s\n",
                current_pid, vaddr, *reg, dst);
    }

}


// add Add the values in registers r1 and r2 and stores the result in r1
void add() {
    global_timestamp++;
    
    u_int32_t val1 = r1[current_pid];
    u_int32_t val2 = r2[current_pid];
    u_int32_t result = val1 + val2;

    fprintf(output_file, "Current PID: %d. Added contents of registers r1 (%u) and r2 (%u). Result: %u\n", current_pid, val1, val2, result);

    r1[current_pid] = result;
}

// map <VPN> <PFN> Maps a virtual page number to a physical frame number.
void map(u_int32_t vpn, u_int32_t pfn) {
    page_tables[current_pid][vpn].pfn = pfn;
    page_tables[current_pid][vpn].valid = 1;

    int tlb_index = -1;
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].valid && tlb[i].vpn == vpn && tlb[i].pid == current_pid) {
            tlb_index = i;
            break;
        }
    }

    if (tlb_index == -1) {
        for (int i = 0; i < TLB_SIZE; i++) {
            if (!tlb[i].valid) {
                tlb_index = i;
                break;
            }
        }

        if (tlb_index == -1) {
            tlb_index = 0;
            for (int i = 1; i < TLB_SIZE; i++) {
                if (tlb[i].timestamp < tlb[tlb_index].timestamp) {
                    tlb_index = i;
                }
            }
        }
    }

    tlb[tlb_index].vpn = vpn;
    tlb[tlb_index].pfn = pfn;
    tlb[tlb_index].valid = 1;
    tlb[tlb_index].pid = current_pid;

    global_timestamp++;
    tlb[tlb_index].timestamp = global_timestamp;

    fprintf(output_file, "Current PID: %d. Mapped virtual page number %u to physical frame number %u\n", current_pid, vpn, pfn);
}



// unmap <VPN> Unmaps a virtual page number
void unmap(u_int32_t vpn){
    global_timestamp++;
    
    page_tables[current_pid][vpn].valid = 0;
    page_tables[current_pid][vpn].pfn = 0;

    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].valid && tlb[i].vpn == vpn && tlb[i].pid == current_pid) {
            tlb[i].valid = 0;
            tlb[i].vpn = 0;
            tlb[i].pfn = 0;
            tlb[i].pid = -1;
            break;
        }
    }

    fprintf(output_file, "Current PID: %d. Unmapped virtual page number %u\n", current_pid, vpn);
}


// store <dst> <src> Store the value of <src> into memory location <dst>
void store(u_int32_t dst, char* src) {
    u_int32_t value;
    if (src[0] == '#') {
        value = atoi(src + 1);
        u_int32_t phys_addr = translate(dst);
        physical_memory[phys_addr] = value;
        fprintf(output_file, "Current PID: %d. Stored immediate %u into location %u\n",
                current_pid, value, dst);
    } else {
        u_int32_t* reg = get_register(src);
        value = *reg;
        u_int32_t phys_addr = translate(dst);
        physical_memory[phys_addr] = value;
        fprintf(output_file, "Current PID: %d. Stored value of register %s (%u) into location %u\n",
                current_pid, src, value, dst);
    }
}

// rinspect <rN> Return content of register <rN> (r1 or r2)
void rinspect(char* reg) {
    global_timestamp++;
    
    u_int32_t* reg_ptr = get_register(reg);
    fprintf(output_file, "Current PID: %d. Inspected register %s. Content: %u\n",
            current_pid, reg, *reg_ptr);
}


// pinspect <VPN> Return content of the page table entry for virtual page number <VPN>
void pinspect(u_int32_t vpn) {
    global_timestamp++;

    if (vpn >= (1U << VPN)) {
        fprintf(output_file, "Current PID: %d. Error: invalid VPN %u\n", current_pid, vpn);
        exit(1);
    }

    pt_entry_t entry = page_tables[current_pid][vpn];
    fprintf(output_file, "Current PID: %d. Inspected page table entry %u. Physical frame number: %u. Valid: %d\n",
            current_pid, vpn, entry.pfn, entry.valid);
}

// linspect <PL> Return content of the memory word at physical location <PL>
void linspect(u_int32_t pl) {
    global_timestamp++;

    u_int32_t phys_size = 1U << (OFF + PFN);
    if (pl >= phys_size) {
        fprintf(output_file, "Current PID: %d. Error: invalid physical location %u\n", current_pid, pl);
        exit(1);
    }

    fprintf(output_file, "Current PID: %d. Inspected physical location %u. Value: %u\n",
            current_pid, pl, physical_memory[pl]);
}

// tinspect <TLBN> Return content of the page table entry for TLB entry <TLBN>
void tinspect(int tlbn) {
    global_timestamp++;

    if (tlbn < 0 || tlbn >= TLB_SIZE) {
        fprintf(output_file, "Current PID: %d. Error: invalid TLB entry number %d\n", current_pid, tlbn);
        exit(1);
    }

    tlb_entry_t entry = tlb[tlbn];
    fprintf(output_file, "Current PID: %d. Inspected TLB entry %d. VPN: %u. PFN: %u. Valid: %d. PID: %d. Timestamp: %u\n",
                current_pid, tlbn, entry.vpn, entry.pfn, entry.valid, entry.pid, entry.timestamp);

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

        // TODO: Implement your memory simulator
        if (tokens[0] == NULL || tokens[0][0] == '%') {
            continue;
        }

        if (!defined && strcmp(tokens[0], "define") != 0) {
            fprintf(output_file, "Current PID: %d. Error: attempt to execute instruction before define\n", current_pid);
            exit(0);
        }

        if(strcmp(tokens[0], "define") == 0){
            define(atoi(tokens[1]), atoi(tokens[2]), atoi(tokens[3]));
        }

        else if (strcmp(tokens[0], "ctxswitch") == 0) {
            int pid = atoi(tokens[1]);
            ctxswitch(pid);
        }

        else if (strcmp(tokens[0], "load") == 0) {
            load(tokens[1], tokens[2]);
        }

        else if (strcmp(tokens[0], "store") == 0) {
            u_int32_t addr = atoi(tokens[1]);
            store(addr, tokens[2]);
        }

        else if (strcmp(tokens[0], "add") == 0) {
            add();
        }
        else if (strcmp(tokens[0], "map") == 0) {
            u_int32_t vpn = atoi(tokens[1]);
            u_int32_t pfn = atoi(tokens[2]);
            map(vpn, pfn);
        }

        else if (strcmp(tokens[0], "unmap") == 0) {
            u_int32_t vpn = atoi(tokens[1]);
            unmap(vpn);
        }

        else if (strcmp(tokens[0], "rinspect") == 0) {
            rinspect(tokens[1]);
        }

        else if (strcmp(tokens[0], "pinspect") == 0) {
            u_int32_t vpn = atoi(tokens[1]);
            pinspect(vpn);
        }

        else if (strcmp(tokens[0], "linspect") == 0) {
            u_int32_t pl = atoi(tokens[1]);
            linspect(pl);
        }
        else if (strcmp(tokens[0], "tinspect") == 0) {
            int tlbn = atoi(tokens[1]);
            tinspect(tlbn);
        }

        // Deallocate tokens
        for (int i = 0; tokens[i] != NULL; i++){
            free(tokens[i]);
        }
        free(tokens);
    }

    // Close input and output files
    fclose(input_file);
    fclose(output_file);

    return 0;
}