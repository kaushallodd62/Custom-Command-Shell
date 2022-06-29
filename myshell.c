/********************************************************************************************
Name: Kaushal Lodd
Subject: Operating Systems
Topic: Create your own command shell using OS system calls to execute built-in Linux commands.
*********************************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>			// exit()
#include <unistd.h>			// fork(), getpid(), exec()
#include <sys/wait.h>		// wait()
#include <signal.h>			// signal()
#include <fcntl.h>			// close(), open()

#define TRUE 1
#define FALSE 0

size_t MAX_BUF = 200;
size_t DEL_SIZE = 3;
size_t MAX_COMMAND_LEN = 30;
size_t MAX_COMMANDS = 5;
size_t MAX_FILE_SZ = 128;

void executeCommandRedirection(char**, int, char*, int);

// This function will parse the input string into multiple commands or a single command with arguments depending on the delimiter (&&, ##, >, or spaces).
char** parseInput(char** inputStr, int* n, char** del)
{
    char* str = (char*)malloc(sizeof(char)*MAX_BUF);
    strcpy(str, *inputStr);
    int count=0, len=strlen(str);

    
    for(int i=0; i<len; i++) {
        if(str[i] == ' ')
            count++;
    }
    int argc = count+2;             // Number of arguments = Number of delimiters + 1; One extra for last argument = NULL
    char** argv = (char**)malloc(sizeof(char*)*argc);
    for(int i=0; i<argc-1; i++) {
        argv[i] = (char*)malloc(sizeof(char)*MAX_COMMAND_LEN);
        strcpy(argv[i], strsep(&str, *del));
    }
    argv[argc-1] = (char*)NULL;


    for(int i=1; i<argc-1; i++) {
        if(!strcmp(argv[i], "&&")) {
            strcpy(*del, "&&");
            break;
        }
        else if(!strcmp(argv[i], "##")) {
            strcpy(*del, "##");
            break;
        }
        else if(!strcmp(argv[i], ">")) {
            strcpy(*del, ">");
        }
    }


    *n = argc;
    return argv;
}

// This function will fork a new process to execute a command
void executeCommand(char** argv, int waitFlag, int redirectionFlag) 
{
    if(!strcmp(argv[0], "cd")) {
        if(chdir(argv[1])!=0) {
            printf("No such directory!\n");
        }
        return;
    }
	int rc = fork();
    if(rc<0) {  // Fork Failed
        fprintf(stderr, "Fork Failed\n");
        exit(1);
    }
    else if(rc==0) {    // Child Process

        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);

        if(redirectionFlag) {
            int i=0;
            while(argv[i] != NULL) {
                i++;
            }
            i--;
            close(STDOUT_FILENO);
            open(argv[i], O_CREAT | O_RDWR | O_APPEND, 0666);
            argv[i] = NULL;
            if(execvp(argv[0], argv) == -1) {
                printf("Shell: Incorrect command\n");
                exit(1);
            }
        }
        else {
            if(execvp(argv[0], argv) == -1) {
                printf("Shell: Incorrect command\n");
                exit(1);
            }
        }
        
    }
    else {  // Parent Process
        if(waitFlag) {
            int rc_wait = waitpid(rc, NULL, WUNTRACED);
            //int rc_wait = wait(NULL);
        }
    }

}

// This function will run multiple commands in parallel
void executeParallelCommands(char** argv, int argc, const char* del)
{
    char** arr = (char**)malloc(sizeof(char*)*MAX_COMMANDS);

    int del_count=0;
    for(int i=0; i<argc-1; i++) {
        if(!strcmp(argv[i], del)) {
            del_count++;
        }
    }

    int i, j=0, k, count=0;
    int redirectionFlag;
    for(i=0; i<del_count+1; i++) {
        for(k=0; k<MAX_COMMANDS; k++) {
            arr[k] = (char*)malloc(sizeof(char)*MAX_COMMAND_LEN);
        }
        k=0;redirectionFlag = FALSE;
        while(j < argc-1 && strcmp(argv[j], del)) {
            strcpy(arr[k], argv[j]);
            if(redirectionFlag == FALSE && !strcmp(arr[k], ">")) {
                redirectionFlag = TRUE;
            }
            j++; k++;
        }
        arr[k] = NULL; j++;

        if(redirectionFlag) {
            char* delim = (char*)malloc(sizeof(char)*DEL_SIZE);
            strcpy(delim, ">");
            executeCommandRedirection(arr, k+1, delim, 0);
        }
        else {
            executeCommand(arr, FALSE, FALSE);   // No wait for Parallel
        }

        for(k=0; k<MAX_COMMANDS; k++) {
            free(arr[k]);
        }
    }
    for(i=0; i<del_count+1; i++) {
        int rc_wait = wait(NULL);
    }
}

// This function will run multiple commands in parallel
void executeSequentialCommands(char** argv, int argc, char* del)
{	
    char** arr = (char**)malloc(sizeof(char*)*MAX_COMMANDS);

    int del_count=0;
    for(int i=0; i<argc-1; i++) {
        if(!strcmp(argv[i], del)) {
            del_count++;
        }
    }

    int i, j=0, k, count=0;
    int redirectionFlag;
    for(i=0; i<del_count+1; i++) {
        for(k=0; k<MAX_COMMANDS; k++) {
            arr[k] = (char*)malloc(sizeof(char)*MAX_COMMAND_LEN);
        }
        k=0;redirectionFlag = FALSE;
        while(j < argc-1 && strcmp(argv[j], del)) {
            strcpy(arr[k], argv[j]);
            if(redirectionFlag == FALSE && !strcmp(arr[k], ">")) {
                redirectionFlag = TRUE;
            }
            j++; k++;
        }
        arr[k] = NULL; j++;

        if(redirectionFlag) {
            char* delim = (char*)malloc(sizeof(char)*DEL_SIZE);
            strcpy(delim, ">");
            executeCommandRedirection(arr, k+1, delim, 0);
        }
        else {
            executeCommand(arr, TRUE, FALSE);   // Wait for sequential
        }

        for(k=0; k<MAX_COMMANDS; k++) {
            free(arr[k]);
        }
    }

}

// This function will run a single command with output redirected to an output file specificed by user
void executeCommandRedirection(char** argv, int argc, char* del, int flag)  // flag = 0 if should not wait, else 1 
{
    char** arr = (char**)malloc(sizeof(char*)*MAX_COMMANDS);

    int del_count=0;
    for(int i=0; i<argc-1; i++) {
        if(!strcmp(argv[i], del)) {
            del_count++;
        }
    }

    if(del_count != 1) {
        fprintf(stderr, "Too few/many arguments!\nUsage: command > file\n");
        return;
    }

    int i=0;
    while(i < argc-1 && strcmp(argv[i], del)) {
        arr[i] = (char*)malloc(sizeof(char)*MAX_COMMAND_LEN);
        strcpy(arr[i], argv[i]);
        i++;
    }
    arr[i] = (char*)malloc(sizeof(char)*MAX_COMMAND_LEN);
    stpcpy(arr[i], argv[i+1]);
    arr[i+1] = (char*)malloc(sizeof(char)*MAX_COMMAND_LEN);
    arr[i+1] = NULL;
    
    if(flag == 0) {
        executeCommand(arr, FALSE, TRUE);
    } else {
        executeCommand(arr, TRUE, TRUE);
    }
}

int main()
{
	// Initial declarations
    char* currWorkingDirectory = (char*)malloc(sizeof(char)*MAX_BUF);
    char* inputStr = (char*)malloc(sizeof(char)*MAX_BUF);
    char** argv;
    int argc=0;
    char* del = (char*)malloc(sizeof(char)*DEL_SIZE);
	
	while(1)	// Infinite loop until user exits
	{
       
        //Printing prompt on shell
        getcwd(currWorkingDirectory, MAX_BUF);
		printf("%s$",currWorkingDirectory);

        signal(SIGINT, SIG_IGN);    // Ignore the signal
        signal(SIGTSTP, SIG_IGN);   // Ignore the signal
		
        getline(&inputStr, &MAX_BUF, stdin);
        //strcpy(inputStr, "ls > output.txt\n");
        int n = strlen(inputStr);
        if(n == 1) {continue;}
        inputStr[n-1] = '\0';
        

		// Parse input with 'strsep()' for different symbols (&&, ##, >) and for spaces.
        strcpy(del, " ");
		argv = parseInput(&inputStr, &argc, &del);

		
		if(!strcmp(argv[0], "exit"))	// When user uses exit command.
		{
			printf("Exiting shell...\n");
			break;
		}
		
		if(!strcmp(del, "&&"))
			executeParallelCommands(argv, argc, del);		// This function is invoked when user wants to run multiple commands in parallel (commands separated by &&)
		else if(!strcmp(del, "##"))
			executeSequentialCommands(argv, argc, del);	// This function is invoked when user wants to run multiple commands sequentially (commands separated by ##)
		else if(!strcmp(del, ">"))
			executeCommandRedirection(argv, argc, del, 1);	// This function is invoked when user wants redirect output of a single command to and output file specificed by user
		else
			executeCommand(argv, TRUE, FALSE);		// This function is invoked when user wants to run a single command
				
	}
	
	return 0;
}
