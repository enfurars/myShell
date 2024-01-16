// necessary headers
#include <unistd.h>
#include <string.h> 
#include <stdio.h>
#include <stdlib.h> 
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h>
#include <sys/sysinfo.h>
#include <dirent.h>
#include <stdbool.h> 
#include <fcntl.h> 

// alias structure to keep the list of given aliases
typedef struct {
    char name[256]; // shortname
    char command[512]; // command given in quotation marks

} alias;

// background structure to keep the list of background processes, maximum number of bg processes is 20
typedef struct{
    int pids[20]; // process ids of background processes
    char pnames[20][128]; // process names of background processes
    int number_of_bgps; // number of background processes

}background;

alias aliases[256]; // define the aliases list
int alias_count = 0; // number of aliases shell knows

background bg_processes; // define the background list
int bgp = 0; // set to 1 when input & is given, background process flag

int argc = 0; // number of arguments 

int len_path; // length of PATH array
char *PATH[256]; // PATH array, it keeps all paths

bool redirect_output = false; // a flag for redirecting output
char file_name[64]; // output file name
int redirection_type; // 1 for >, 2 for >> and 3 for >>> 

char lec[256] = "No command is given before"; // last executed command 

void redirect();
void reverseString();
void add_new_alias(char *argv[]);
void load_aliases();
char* find_alias(char* name); 

// this function prepares the prompt as described in the project description
void prompt_user(char prompt[]) {

    char *username = getenv("USER"); // current user in the system
    char *home_dir = getenv("HOME"); // home directory in the system
    char hostname[256]; // machine name of the system
    char current_dir[256]; // current directory including home
    getcwd(current_dir, sizeof(current_dir)); 
    gethostname(hostname, sizeof(hostname));
    
    // replacing home directory with symbol '~' in the current directory 
    char temp_dir[256];
    snprintf(temp_dir, sizeof(temp_dir), "~%s", current_dir + strlen(home_dir));
    strcpy(current_dir, temp_dir);

    // preparing the prompt for user input
    strcpy(prompt, username);
    strcat(prompt, "@");
    strcat(prompt, hostname);
    strcat(prompt, " ");
    strcat(prompt, current_dir);
    strcat(prompt, " ");
    strcat(prompt, "---");
    strcat(prompt, " ");

}

// this function counts the number of processes under myshell (inclusive)
// this function assumes ps command is under path /bin/ps and myshell is the only process running under current shell (except for myshell's own children)
int get_process_count() { 
    int number_of_processes;
    FILE *fp;
    char path[1035];

    // Open the command for reading.
    fp = popen("/bin/ps", "r");
    if (fp == NULL) {
        printf("Failed to run command\n");
        return 1;
    }

    // Read the output line by line.
    while (fgets(path, sizeof(path)-1, fp) != NULL) {
        number_of_processes++;
    }

    // Close the file pointer.
    pclose(fp);

    // Subtract 2 to exclude the header line and current shell process from the count.
    number_of_processes = number_of_processes -2;
    return number_of_processes;
}

// this function is written according to the project description. 
void bello() {

    char *username = getenv("USER"); // current user in the system
    char *shellname = getenv("SHELL"); // current shell name in use
    char *home_dir = getenv("HOME"); // home directory
    char hostname[256]; // machine name of the system
    gethostname(hostname, sizeof(hostname));
    char *tty = ttyname(STDIN_FILENO); // tty
    int number_of_processes; // under myshell(myshell, ps and bello included)
    number_of_processes = get_process_count();
    // time and date
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    
    if(redirection_type == 3) { // >>> is given
        char buffer[4096];
        char temp[30];

        // concat bello output to one string
        strcpy(buffer, username);
        strcat(buffer, " ");
        strcat(buffer, hostname);
        strcat(buffer, " ");
        strcat(buffer, lec);
        strcat(buffer, " ");
        strcat(buffer, tty);
        strcat(buffer, " ");
        strcat(buffer, shellname);
        strcat(buffer, " ");
        strcat(buffer, home_dir);
        strcat(buffer, " ");
        sprintf(temp, "%d-%02d-%02d %02d:%02d:%02d",tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,tm.tm_hour, tm.tm_min, tm.tm_sec);
        strcat(buffer, temp);
        strcat(buffer, " ");
        sprintf(temp, "%d", number_of_processes);
        strcat(buffer, temp);

        // reverse the final string
        reverseString(buffer); 

        // redirect the output 
        redirection_type = 2;
        redirect(); 
        
        // Print the reversed content
        printf("%s", buffer);
        // turn back to console for output after redirection
        freopen(tty, "w", stdout); 

    } else { // >>> is not given
        if(redirect_output) { // > or >> may be given 
            redirect(); 
        }
        printf("Username: %s\n", username);
        printf("Hostname: %s\n", hostname);
        printf("Last Executed Command: %s\n", lec); 
        printf("TTY: %s\n", tty);
        printf("Current Shell Name: %s\n", shellname); 
        printf("Home Location: %s\n", home_dir);
        printf("Current Time and Date: %d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        printf("Current Number of processes being executed: %d\n", number_of_processes); // this will give at least 3 because it counts myshell, bello and ps too.
    
        // turn back to console for output after redirection
        if(redirect_output) {
            freopen(tty, "w", stdout); 
        }
    }
    
}

// this function fills the global variable PATH to execute commands in fork-exec manner
void fill_PATH(){
    char *path_variable = getenv("PATH"); // get the PATH variable
    char *token = NULL; 

    // tokenize the paths and fill the PATH global variable
    token = strtok(path_variable, ":");
    while(token){ 
        PATH[len_path++] = token;
        token = strtok(NULL, ":");
    }
}

// this function detects if any output redirection is needed by examining all parameters
void handle_output_redirections(char *argv[]) {

    for(int arg_ct = 0; arg_ct < argc; arg_ct++) {
        if(strcmp(argv[arg_ct], ">") == 0) { 
            if(argc > arg_ct + 1) {
                redirect_output = true; 
                redirection_type = 1;
                strcpy(file_name, argv[arg_ct + 1]); 
                // extract redirection operator and file name from arguments
                for(int i = arg_ct + 2, j = arg_ct; i < argc; i++, j++) {
                    argv[j] = strdup(argv[i]);
                }
                argc = argc - 2;
                arg_ct--;
            } else {
                printf("Enter a file name after redirection operator!\n");
                argv[0] = NULL; // set arguments to NULL in order to take another input from user
                return; 
            }
        } else if(strcmp(argv[arg_ct], ">>") == 0) {
            if(argc > arg_ct + 1) {
                redirect_output = true;
                redirection_type = 2; 
                strcpy(file_name, argv[arg_ct + 1]); 
                // extract redirection operator and file name from arguments
                for(int i = arg_ct + 2, j = arg_ct; i < argc; i++, j++) {
                    argv[j] = strdup(argv[i]);
                }
                argc = argc - 2;
                arg_ct--;
            } else {
                printf("Enter a file name after redirection operator!\n");
                argv[0] = NULL; // set arguments to NULL in order to take another input from user
                return; 
            }
        } else if(strcmp(argv[arg_ct], ">>>") == 0) {
            if(argc > arg_ct + 1) {
                redirect_output = true;
                redirection_type = 3; 
                strcpy(file_name, argv[arg_ct + 1]); 
                // extract redirection operator and file name from arguments
                for(int i = arg_ct + 2, j = arg_ct; i < argc; i++, j++) {
                    argv[j] = strdup(argv[i]);
                }
                argc = argc - 2;
                arg_ct--;
            } else {
                printf("Enter a file name after redirection operator!\n");
                argv[0] = NULL; // set arguments to NULL in order to take another input from user
                return; 
            }
        }
    }

    argv[argc] = NULL; // last argument should be NULL again 

}

// this function returns the invested version of the given string, it is used for >>> redirection
void reverseString(char* str) {
    int length = strlen(str);
    int i, j;
    char temp;

    for (i = 0, j = length - 1; i < j; i++, j--) {
        temp = str[i];
        str[i] = str[j];
        str[j] = temp;
    }
}

// this function handles the output redirection by opening files setting stdout
void redirect() {

    if(redirection_type == 1) { // > type redirection file is opened with O_TRUNC
        int fd = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            perror("File open error");
            exit(EXIT_FAILURE);
        }
        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("Duplication error");
            exit(EXIT_FAILURE);
        }
        close(fd);
    }else if (redirection_type == 2){ // >> and >>> type redirection file is opened with O_APPEND
        int fd = open(file_name, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd == -1) {
            perror("File open error");
            exit(EXIT_FAILURE);
        }
        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("Duplication error");
            exit(EXIT_FAILURE);
        }
        close(fd);
    }
}

// this function checks if any of the background processes is terminated and updates the background processes list
void update_background() {
    if(bg_processes.number_of_bgps < 1) { // there is no bg process, we can return safely
        return;
    }else {
        pid_t terminated_pid; // pid of terminated process
        int status; // exit status of terminated process

        // check if it is terminated for all background processes
        for(int i = 0; i < 20 && bg_processes.number_of_bgps != 0; i++) {
            if(bg_processes.pids[i] != 0) { 
                terminated_pid = waitpid(bg_processes.pids[i], &status, WNOHANG);
                if(terminated_pid > 0) {
                    printf("The background process \"%s\" is terminated!\n", bg_processes.pnames[i]);
                    // delete the terminated process from the list
                    bg_processes.pids[i] = 0;
                    bg_processes.number_of_bgps -= 1; 
                }
            }
        }
    }
}

// this function takes the argv array which includes command and arguments then executes the command by iterating in the PATH variables
void execute(char *argv[]) {
    char pth[256]; 

    // open a pipe, it will be needed if >>> operator is given
    int pipe_fd[2];
    pipe(pipe_fd); 
    
    //fork and execute
    pid_t pid = fork(); 
    
    if(pid == -1) { // FORK FAIL
        printf("Child could not created with fork!\n");
        return;
    }
    
    else if (pid == 0) { // CHILD

        // redirect the output if necessary
        if(redirect_output) {
            if(redirection_type == 3) { // >>> redirection
                close(pipe_fd[0]); // Close the read end of the pipe
                dup2(pipe_fd[1], STDOUT_FILENO); 
            } else { // > and >> redirections
                redirect(); 
            }
        }

        // check all paths and execute the command when first executable is found
        for(int p = 0; p < len_path; p++) {
            strcpy(pth, PATH[p]);
            strcat(pth, "/");
            strcat(pth, argv[0]);
            execv(pth, argv);
        }
        // execution failed
        perror("Invalid command!\n");
        exit(EXIT_FAILURE);

    }else { // PARENT
        if (bgp == 0) { // child is not a background process
            if(redirection_type == 3) { // >>> handling
                close(pipe_fd[1]); // Close the write end of the pipe
                char buffer[4096];
                // Read the output of ls from the pipe
                read(pipe_fd[0], buffer, sizeof(buffer));
                // Close the read end of the pipe
                close(pipe_fd[0]);
                reverseString(buffer);

                redirection_type = 2;
                redirect();
                // Print the reversed content
                printf("%s", buffer);
                char *tty = ttyname(STDIN_FILENO); // tty
                freopen(tty, "w", stdout); 
            } 
            // wait the child
            waitpid(pid, NULL, 0);
            return;

        } else { // child is a background process  
            if(bg_processes.number_of_bgps < 20) {// we have space for new background process
                // find an empty index from the list
                for(int i = 0; i < 20; i++) {
                    if(bg_processes.pids[i] == 0){

                    // add to background process list
                    bg_processes.number_of_bgps++;
                    bg_processes.pids[i] = pid;
                    strcpy(bg_processes.pnames[i], argv[0]);

                    break;
                    
                    }
                }
            }
        }
    }
}


int main() {

    char *argv[512]; // array of arguments given including the command, size 512 is sufficient
    
    char input[512]; // input given
    
    char prompt[512]; // prompt for each command

    // load saved aliases
    load_aliases();
    
    // fill the path array
    fill_PATH();

    // initialize all background process ids to 0
    for(int i = 0; i < 20; i++) {
        bg_processes.pids[i] = 0;
    }

    while(1) {
        // set redirect output flag to false at the beginning
        redirect_output = false; 
        redirection_type = -1;
        argc = 0; 

        // prompting the user to get new input
        prompt_user(prompt);
        printf("%s", prompt);
        fflush(stdout);

        // reading the input 
        int length = (int) read(STDIN_FILENO, input, 512);
        //printf("%s", input);

        int quote = 0;
        bgp = 0; // background process flag, 1 if & is given
        int param_pointer = -1; // to indicate error
        int param_number = 0; // to place parameters to argv
        
        for(int letter = 0; letter < length; letter++) {
            if(input[letter] == '\t' || input[letter] == ' ') { // letter is space or tab
                if(quote == 1) { // if we are in quotation continue to next letter
                    continue;
                }
                // place the next argument from input
                if(param_pointer >= 0) { 
                    argv[param_number++] = &input[param_pointer];
                }
                param_pointer = -1; 
                input[letter] = '\0';
            }else if(input[letter] == '\n') { // end of input
                // place the next argument from input
                if(param_pointer >= 0 ) {
                    argv[param_number++] = &input[param_pointer];
                }
                // last parameter is given so NULL end the array
                argv[param_number] = NULL; 
                input[letter] = '\0';

            }else { // letter is something else
                if(param_pointer < 0) {
                    param_pointer = letter; // set pointer
                }
                switch(input[letter]) {
                    case '&': // command will be executed as background process since ampersand is given
                        bgp = 1;
                        input[letter - 1] = '\0';
                        letter = letter + 1; // extracting the & operator
                        param_pointer = letter;
                        break;
                    case '"': // letter is quotation mark
                        if(quote == 0) {
                            quote++; // begin quotation
                        }else if(quote == 1){
                            quote--; // end quotation 
                        }
                        break;
                    default:
                } 
            } 
        }
        argc = param_number; // set argument count

        // check for redirection 
        handle_output_redirections(argv);
        
        /* print arguments
        for(int c = 0; c <= param_number; c++) {
            printf("argv %d = %s\n", c, argv[c]);
        }  */

        // SELECT THE EXECUTION TYPE OF THE COMMAND

        if (argv[0] == NULL) { 

            continue; // continue to take command if null command is given

        } else if (strcmp(argv[0], "alias") == 0){ // save the given alias

            add_new_alias(argv); 

        }else if (strcmp(argv[0], "exit") == 0) { // terminate the myShell if exit command is given

            if (argv[1]) {
                printf("\'exit\' command does not take any arguments!\n");
                continue; 
            }
            // check background processes before exiting
            update_background();
            if(bg_processes.number_of_bgps > 0) {
                printf("There are still processes running in the background!\n");
                continue;
            }
            printf("There is no process running in the background.\n");
            exit(0); 

        } else if(strcmp(argv[0], "bello") == 0) { // execute the bello command

            if (argv[1]) {
                printf("\'bello\' command does not take any arguments!\n");
                continue; 
            }
            bello(); 

        }else if (find_alias(argv[0])){ // alias exists with the given input
            // check for alias_name > o.txt cases
            handle_output_redirections(argv);
            
            char *alias_input = find_alias(argv[0]); // quotetaion mark part of the alias
            char *token = NULL; 

            // tokenize the alias's command and fill the argv array
            int i = 0;
            token = strtok(alias_input, " ");
            while (token) {
                argv[i++] = strdup(token); 
                token = strtok(NULL, " ");
            }
            argv[i] = NULL; // null terminate the argv
            argc = i; // set argument number

            // check for redirection
            handle_output_redirections(argv);

            execute(argv); // alias will be searched in executables in the path

        } else { // input will be searched in executables in the path
            execute(argv);
        }
    
        strcpy(lec, input); // update last executed command
        
        // at the end of each loop check if any background process is terminated
        update_background();

    }

    return 0; 
} 

// load the aliases from the aliases.txt file, this function is called at the beginning of the shell to remember previous aliases even after a restart of the shell
// if no alias is given before shell starts with a warning which is in the else part
void load_aliases() {
    FILE* file = fopen("aliases.txt", "r"); // open aliases file
    if (file != NULL) {
        while (fscanf(file, "%99[^=]=%199[^\n]\n", aliases[alias_count].name, aliases[alias_count].command) == 2) { // read the aliases from the file and fill the aliases list
            alias_count++;
        }
        fclose(file); // close file
    } else {
        printf("No alias file found. Starting with an empty alias list.\n");
    }
}

// this function takes a command name and scans the aliases list if there exist an alias with that name
// returns the corresponding command or NULL if not found
char* find_alias(char* name) {
    for (int i = 0; i < alias_count; ++i) {
        if (strcmp(aliases[i].name, name) == 0) {
            return aliases[i].command;
        }
    }
    return NULL;
}

// this function saves the current aliases list to aliases file for future use 
void save_aliases() {
    FILE *file = fopen("aliases.txt", "w"); // open the aliases file

    if(file != NULL) {
        for(int i = 0; i < alias_count; i++) {
            fprintf(file, "%s=%s\n", aliases[i].name, aliases[i].command); // save the alias as name="command"
        }
        fclose(file); // close the file
    } else {
        printf("Error: Unable to save aliases to file.\n");
    }
}

// this function add new alias to the aliases list and then calls the save_aliases() function
void add_new_alias(char *argv[]) {
    if(alias_count < 512) { // if we have space                                                    
        size_t command_len = strlen(argv[3]); // length of the command, remember alias is given as alias enes = "echo This is myShell" 
        char command[512]; 
        memcpy(command, &argv[3][1], command_len - 2); // copy the command part to command variable
        command[strlen(argv[3]) - 2] = '\0'; // end of the command will be null string

        // store the alias to list
        strcpy(aliases[alias_count].name, argv[1]); 
        strcpy(aliases[alias_count].command, command); 
        alias_count++;
        // save the alias to file
        save_aliases();
    }

}
