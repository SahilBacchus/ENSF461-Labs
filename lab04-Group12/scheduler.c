#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <time.h>

// imported these extra: 
#include <stdbool.h>



#define min(a,b) (((a)<(b))?(a):(b))

// total jobs
int numofjobs = 0;

struct job {
    // job id is ordered by the arrival; jobs arrived first have smaller job id, always increment by 1
    int id;
    int arrival; // arrival time; safely assume the time unit has the minimal increment of 1
    int length;
    int tickets; // number of tickets for lottery scheduling
    
    // TODO: add any other metadata you need to track here
    bool is_completed;
    int time_left; 


    int responseTime;
    int turnaroundTime;
    int waitTime;
    int startTime;




    struct job *next;
};

// the workload list
struct job *head = NULL;


void append_to(struct job **head_pointer, int arrival, int length, int tickets){ 
    // TODO: create a new job and init it with proper data
    
    struct job *cursor = *head_pointer;
    struct job *new_job = malloc(sizeof(struct job));
    new_job -> id = numofjobs;
    new_job -> arrival = arrival;
    new_job -> length = length;
    new_job -> tickets = tickets;
    new_job -> next = NULL; 

    new_job -> is_completed = false; 
    new_job -> time_left = length; 

    new_job -> startTime = -1;
    

    // printf("adding job %d: %d %d\n", numofjobs, arrival, length); 

    if(cursor == NULL){
        *head_pointer = new_job; 
        numofjobs++; 
        return;
    }
    
    while(cursor -> next != NULL) cursor = cursor -> next; 
    cursor -> next = new_job; 
    numofjobs++; 
    
    return;
}


void read_job_config(const char* filename)
{
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int tickets  = 0;

    char* delim = ",";
    char *arrival = NULL;
    char *length = NULL;

    // TODO, error checking
    fp = fopen(filename, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    // TODO: if the file is empty, we should just exit with error

    // check if the end of file is at the 0 position if not go back to beginning
    fseek(fp, 0, SEEK_END); 
    if (ftell(fp) == 0){
        printf("error: file empty \n"); 
        fclose(fp);
        exit(EXIT_FAILURE); 
    }
    fseek(fp, 0, SEEK_SET); 

    while ((read = getline(&line, &len, fp)) != -1)
    {
        if( line[read-1] == '\n' )
            line[read-1] =0;
        arrival = strtok(line, delim);
        length = strtok(NULL, delim);
        tickets += 100;

        append_to(&head, atoi(arrival), atoi(length), tickets);
    }

    fclose(fp);
    if (line) free(line);
}



//====== Scheduling Polcies =========//


// Shortets Job First
void policy_SJF()
{
    printf("Execution trace with SJF:\n");

    // TODO: implement SJF policy

    struct job *temp = head;
    int min_len;
    int min_id = 0; 
    int time = 0;
    int completed = 0; 
        
    while (completed < numofjobs){

        // Get shortest job:
        temp = head; 
        min_len = INT_MAX; 
        while (temp != NULL){
            const int len = temp->length;
            const int arrival = temp -> arrival; 

            if (temp -> is_completed == false && len < min_len && arrival <= time){ // only update min_len if it has not been completed 
                min_len = len; 
                min_id = temp->id; 
            }
            temp = temp -> next; 
        }

        // printf("%d ", min_len);
        // printf("%d ", min_id);

        // traverse to min job
        temp = head; 
        while (temp != NULL && temp->id != min_id) temp = temp -> next; 

        // Skip if the job hasnt arrived yet and incrment time
        if (min_len == INT_MAX || temp->arrival > time){
            time++; 
            continue; 
        }
        
        // print min job
        printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n", time, temp->id, temp->arrival, temp->length);
 

        // Analysis stuff
        if (temp->startTime == -1) {
            temp->startTime = time;
            temp->responseTime = temp->startTime - temp->arrival;
        }

        // run job
        temp -> is_completed = true; 
        time += temp -> length;
        completed++;  


        temp->turnaroundTime = time - temp->arrival;
        temp->waitTime = temp->turnaroundTime - temp->length;

    }   



    printf("End of execution with SJF.\n");

}


// Shortest To Completion First
void policy_STCF()
{
    printf("Execution trace with STCF:\n");

    struct job *temp;
    struct job *shortest = NULL;
    struct job *current_job = NULL;

    int time = 0;
    int completed = 0;
    int run_start = 0;

    while (completed < numofjobs) {


        // Get shortest time left:
        temp = head; 
        int shortest_time = INT_MAX;
        shortest = NULL;
        while (temp != NULL) {
            if (temp->is_completed == false && temp->arrival <= time && temp->time_left < shortest_time) {
                shortest_time = temp->time_left;
                shortest = temp;
            }
            temp = temp->next;
        }

        // printf("%d ", min_len);
        // printf("%d ", min_id);

        // Skip if the job hasnt arrived yet and incrment time
        if (shortest == NULL){
            time++; 
            continue; 
        }

        // When job changes for shorter one (preemption)
        if (current_job != shortest) {

            // print job before switching info
            if (current_job != NULL) {
                int ran_for = time - run_start;
                printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n", run_start, current_job->id, current_job->arrival, ran_for);
            }

            // Start new job
            current_job = shortest;
            run_start = time;

            // analysis stuff
            if (current_job->startTime == -1) {
                current_job->startTime = time;
                current_job->responseTime = current_job->startTime - current_job->arrival;
            }


        }

        // Increment time 
        current_job->time_left--;
        time++;

        // Completed a job
        if (current_job->time_left == 0) {
            current_job->is_completed = true;
            completed++;

            current_job->turnaroundTime = time - current_job->arrival;
            current_job->waitTime = current_job->turnaroundTime - current_job->length;

            // Print job completed info 
            int ran_for = time - run_start;
            printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n", run_start, current_job->id, current_job->arrival, ran_for);

            current_job = NULL;
        }

    }

    printf("End of execution with STCF.\n");
}


// Round Robin
void policy_RR(int slice)
{
    printf("Execution trace with RR:\n");

    // TODO: implement RR policy
    int time = 0;
    int jobsCompleted = 0;
    struct job *temp;

    while (jobsCompleted < numofjobs) {
        int progress = 0;
        temp = head;

        while (temp != NULL) {
            if (temp->arrival <= time && temp->time_left > 0 && temp->is_completed == false) {
                if(temp->startTime == -1){
                    temp->startTime = time;
                    temp->responseTime = temp->startTime - temp->arrival;
                    // totalResponseTime += temp->responseTime;
                }
                int runtime = min(slice, temp->time_left);
                printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n",
                       time, temp->id, temp->arrival, runtime);
                temp->time_left -= runtime;
                time += runtime;
                progress = 1;

                if (temp->time_left == 0){
                    jobsCompleted++;
                    temp->is_completed = true; 
                    temp->turnaroundTime = time - temp->arrival;
                    temp->waitTime = temp->turnaroundTime - temp->length;
                    // totalTurnaroundTime += temp->turnaroundTime;
                    // totalWaitTime += temp->waitTime;
                }
            }
            temp = temp->next;
        }

        if (!progress)
            time++;
    }

    printf("End of execution with RR.\n");
}


// Lottery Ticket
void policy_LT(int slice)
{
    printf("Execution trace with LT:\n");

    // Leave this here, it will ensure the scheduling behavior remains deterministic
    srand(42);

    // In the following, you'll need to:
    // Figure out which active job to run first
    // Pick the job with the shortest remaining time
    // Considers jobs in order of arrival, so implicitly breaks ties by choosing the job with the lowest ID

    // To achieve consistency with the tests, you are encouraged to choose the winning ticket as follows:
    // int winning_ticket = rand() % total_tickets;
    // And pick the winning job using the linked list approach discussed in class, or equivalent

    struct job *temp = head;
    struct job *winning_job = NULL; 

    int winning_ticket; 
    int total_tickets = 0; 
    int time = 0;
    int completed = 0; 

    while (completed < numofjobs){

        // Get total tickets
        temp = head;
        total_tickets = 0; 
        winning_job = NULL; 
        while (temp != NULL){
            if (temp->is_completed == false && temp->arrival <= time){
                total_tickets += temp -> tickets; 
            }

            temp = temp -> next; 
        }

        // Case: no jobs have arrived yet 
        if (total_tickets == 0){
            temp = head; 
            while(temp != NULL && (temp->is_completed == true || temp->arrival <= time)) temp = temp -> next; 

            if (temp == NULL) break; 
            
            time = temp -> arrival;
            continue; 
        }
     

        // get winning ticket 
        winning_ticket = rand() % total_tickets;


        // Pick winning job
        temp = head; 
        int counter = 0; 
        while (temp != NULL){
            if (temp->is_completed == false && temp->arrival <= time){
                counter += temp->tickets; 
            }
            if (counter > winning_ticket){
                winning_job = temp;
                break;  
            }
        
            temp = temp -> next;
        }
        

        // run  winning job
        if (winning_job != NULL){
            printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n", time, winning_job->id, winning_job->arrival, slice);
            
            // anaysis stuff
            if (winning_job->startTime == -1) {
                winning_job->startTime = time;
                winning_job->responseTime = winning_job->startTime - winning_job->arrival;
            }


            // Increment time
            winning_job->time_left -= slice;
            time += slice; 

            // completed a job
            if (winning_job -> time_left <= 0 ){
                winning_job -> is_completed = true; 
                completed++; 
                winning_job->turnaroundTime = time - winning_job->arrival;
                winning_job->waitTime = winning_job->turnaroundTime - winning_job->length;
            }
        }

    }

    printf("End of execution with LT.\n");

}


// First In First Out
void policy_FIFO(){
    printf("Execution trace with FIFO:\n");

    // TODO: implement FIFO policy


    struct job *temp = head;
    int time = head->arrival;
    while(temp){
        // print job
        printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n", time, temp->id, temp->arrival, temp->length);
        
        // Analysis stuff
        if (temp->startTime == -1) {
            temp->startTime = time;
            temp->responseTime = temp->startTime - temp->arrival;
        }
        temp->turnaroundTime = (time + temp->length) - temp->arrival;
        temp->waitTime = temp->turnaroundTime - temp->length;
        

        time += temp->length;
        temp = temp->next;

        if(temp != NULL){
            if(temp->arrival > time){
                time = temp->arrival;
            }
        }
    }


    printf("End of execution with FIFO.\n");
    return; 
}



//========= Analysis =============//

void analyze_policy(const char *policy_name) {
    printf("Begin analyzing %s:\n", policy_name);

    struct job *temp = head;
    double total_response = 0;
    double total_turnaround = 0;
    double total_wait = 0;
    int count = 0;

    while (temp != NULL) {
        temp->waitTime = temp->turnaroundTime - temp->length;

        printf("Job %d -- Response time: %d  Turnaround: %d  Wait: %d\n",
               temp->id,
               temp->responseTime,
               temp->turnaroundTime,
               temp->waitTime);

        total_response += temp->responseTime;
        total_turnaround += temp->turnaroundTime;
        total_wait += temp->waitTime;
        count++;

        temp = temp->next;
    }

    printf("Average -- Response: %.2f  Turnaround %.2f  Wait %.2f\n",
           total_response / count,
           total_turnaround / count,
           total_wait / count);

    printf("End analyzing %s.\n", policy_name);
}



int main(int argc, char **argv){

    static char usage[] = "usage: %s analysis policy slice trace\n";

    int analysis;
    char *pname;
    char *tname;
    int slice;


    if (argc < 5)
    {
        fprintf(stderr, "missing variables\n");
        fprintf(stderr, usage, argv[0]);
		exit(1);
    }

    // if 0, we don't analysis the performance
    analysis = atoi(argv[1]);

    // policy name
    pname = argv[2];

    // time slice, only valid for RR
    slice = atoi(argv[3]);

    // workload trace
    tname = argv[4];

    read_job_config(tname);

    if (strcmp(pname, "FIFO") == 0){
        policy_FIFO();
        if (analysis == 1){
            // TODO: perform analysis
            analyze_policy("FIFO");
        }
    }
    else if (strcmp(pname, "SJF") == 0)
    {
        // TODO
        policy_SJF(); 
        if (analysis == 1) analyze_policy("SJF");
        
    }
    else if (strcmp(pname, "STCF") == 0)
    {
        // TODO
        policy_STCF(); 
        if (analysis == 1) analyze_policy("STCF");

    }
    else if (strcmp(pname, "RR") == 0)
    {
        // TODO
        policy_RR(slice); 
        if (analysis == 1) analyze_policy("RR");

    }
    else if (strcmp(pname, "LT") == 0)
    {
        // TODO
        policy_LT(slice);
        if (analysis == 1) analyze_policy("LT");
 
    }

	exit(0);
}