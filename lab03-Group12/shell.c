#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "parser.h"

#define BUFLEN 1024
#define MAX_ARGS 4 // Max args -1 (includes null) that can be used in the shell --> based on lab, 4 seems like max, meaning 3 actual args
#define VERBOSE 0 // 1 if you want verbose mode (printed out searching for path), 0 for nothing else printed

//To Do: This base file has been provided to help you start the lab, you'll need to heavily modify it to implement all of the features

char* find_path(char* cmd, int debug); 
void parse_args(char* input, char* args[], int max_args);
void execute_pipe(char* left, char* right); 

int main() {
    char buffer[1024];
    char* parsedinput;
    char* args[MAX_ARGS];
    char newline;

    printf("Welcome to the Group12 shell! Enter commands, enter 'quit' to exit\n");
    do {
        //Print the terminal prompt and get input
        printf("$ ");
        char *input = fgets(buffer, sizeof(buffer), stdin);
        if(!input)
        {
            fprintf(stderr, "Error reading input\n");
            return -1;
        }
        
        //Clean and parse the input string
        parsedinput = (char*) malloc(BUFLEN * sizeof(char));
        size_t parselength = trimstring(parsedinput, input, BUFLEN);

        //Sample shell logic implementation
        if ( strcmp(parsedinput, "quit") == 0 ) {
            printf("Bye!!\n");
            return 0;
        }
        else {
            // Here goes the magic!


            char* pipe_pos = strchr(parsedinput, '|');

            if (pipe_pos) { // commands that require pipe
                *pipe_pos = '\0';
                char* left = parsedinput;
                char* right = pipe_pos + 1;

                // Clear leading whitspace
                while (*left == ' ' || *left == '\t') left++;
                while (*right == ' ' || *right == '\t') right++;

                // do our pipe --> feature 4
                execute_pipe(left, right);
            } 


            else{ // Regular commands that dont require pipe 

                pid_t pid = fork();

                //
                // Fork failed
                //
                if (pid < 0) {
                    perror("fork");
                    continue;
                }

                //
                // Child process --> run execve
                //
                else if (pid == 0) {
                


                char* cmd = strtok(parsedinput, " "); // Get the first token --> command to execute
                char* path = find_path(cmd, VERBOSE);     // Get path to command, see below main for implementation 

                parse_args(parsedinput, args, MAX_ARGS); // parses args and puts it int "args" to be passed into execve
                
                execve(path, args, NULL);
                perror("execve");

                free(path); 
                exit(1); 
    
                }

                //
                // Parent process --> wait for child
                //
                else { 
                    int status;
                    waitpid(pid, &status, 0);
                }

        }

            
        }

        //Remember to free any memory you allocate!
        free(parsedinput);
    } while ( 1 );

    return 0;
}





//=============== Feature 2 ====================//


//Function that parses the arguments of an input --> seperates them into command and parameters 
//[Input] char* input - the string we want to parse args of
//[Input] char* args[] - were the result of the the parsing is stored
//[Input] int max_args- the maximum args char* args[] can hold
void parse_args(char* input, char* args[], int max_args){
    int argc = 0; // argument count
    char *ptr = input; // another pointer to input 

    while ((*ptr != '\0') && (argc < max_args - 1)){
        
        // skip leading whitspaces 
        while (*ptr == ' ' || *ptr== '\n' || *ptr== '\t') ptr++; 


        // Encounters argument with qoutes --> need to treat it as one arg
        if (*ptr =='"'){
            ptr++;
            args[argc] = ptr; 
            argc++;
            
            while (*ptr != '\0' && *ptr != '"') ptr++; // go to end of qoutes 
            *ptr = '\0'; 
            ptr++;
        }


        // Encounters normal argument termintated with whitespace
        else {
            args[argc] = ptr; 
            argc++;

            while (*ptr != '\0' && *ptr != ' ' && *ptr != '\n' && *ptr != '\t') ptr++; // Go to end of arg
            *ptr = '\0'; 
            ptr++;
        }

        args[argc] = NULL; // last arg needs to be null for execve


    }
}





//=============== Feature 3 ====================//


//Function that finds the absolute path of an executable 
//[Input] char* cmd - the command to find absolute path for
//[Input] int debug - 0 nothing is printed, 1 prints where 
//[Return] char* found_path - the full path if found --> must be freed later
char* find_path(char* cmd, int debug){


    //
    // CASE 1: Absolute path
    //
    if (cmd[0] == '/'){ // First letter is '/'
        return strdup(cmd); 
    }

    //
    // CASE 2: Relative path 
    //
    if (strchr(cmd, '/')){ // contains '/'
        return strdup(cmd); 
    }

    //
    // CASE 3: Searching "$PATH" for path 
    //
    char full_path[BUFLEN];
    char* path_env = getenv("PATH"); 
    if (!path_env){
        return NULL; 
    }

    // Loop through all paths and find one that works using access()
    char* path_dir = strtok(path_env, ":"); // ":" is the delimiter
    while (path_dir != NULL) {
        
        full_path[0] = '\0'; // clear previous path 
        strcat(full_path, path_dir);
        strcat(full_path, "/");
        strcat(full_path, cmd);

        if (debug) printf("searching: %s \n", path_dir);

        if (access(full_path, X_OK) == 0) { // Check if the file exists and is executable
            if (debug) printf("found in:  %s \n", full_path);

            char* found_path = strdup(full_path); 
            return found_path; 
        }

        path_dir = strtok(NULL, ":");
    }

    return NULL; 

}


//=============== Feature 4 ====================//
//Function that executes two commands connected by a pipe  --> lab said only need 2
//[Input] char* left - the command on the left side of the pipe (writes to pipe)
//[Input] char* right - the command on the right side of the pipe (reads from pipe)

void execute_pipe(char* left, char* right) {
    int fd[2];
    
    // calling pipe to connect our file descriptors 
    if (pipe(fd) == -1) { 
        perror("pipe"); 
        return; 
    }

    pid_t p1 = fork();
    if (p1 == 0) { // Left command

        // write end of pipe into standard output --> to right command 
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]); 
        close(fd[1]);
        
        // run left command
        char* args[MAX_ARGS]; 
        // char* cmd = strtok(left, " "); // Get the first token --> command to execute
        // char* path = find_path(cmd, 1);

        parse_args(left, args, MAX_ARGS);
        char* path = find_path(args[0], VERBOSE);

        execve(path, args, NULL);
        perror("execve");
        free(path);
        exit(1);

    }

    pid_t p2 = fork();
    if (p2 == 0) { // Right command

        // take  in  standard input --> takes in left command
        dup2(fd[0], STDIN_FILENO);
        close(fd[1]); 
        close(fd[0]);

        // Run rigth command
        char* args[MAX_ARGS]; 
        //char* cmd = strtok(right, " "); // Get the first token --> command to execute
        //char* path = find_path(cmd, 1);

        parse_args(right, args, MAX_ARGS);
        char* path = find_path(args[0], VERBOSE);

        execve(path, args, NULL);
        perror("execve");
        free(path);
        exit(1); 

    }

    // wait for child processes to finish 
    close(fd[0]); 
    close(fd[1]);
    waitpid(p1, NULL, 0);
    waitpid(p2, NULL, 0);
}
