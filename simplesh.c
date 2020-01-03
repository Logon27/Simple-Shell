#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>

char* readInputLine(void);
char** splitInputLine(char* line);
int executeCommand(char** args);
int numbOfCommands(char** args);
int maxNumbOfArgs(char** args);
char* inputRedirection(char** args);
char* outputRedirection(char** args);

int main(int argc, char* argv[])
{

    char* inputLine;
    char** args;
    int status;

    while(1)
    {
        write(1, "=> ", 3);
        inputLine = readInputLine();
        if(inputLine == NULL)
        {
            write(1, "Failed to read input...\n", 24);
            continue;
        }
		//split our input into an array of arguments
        args = splitInputLine(inputLine);
		//pass the arguments for execution
        status = executeCommand(args);
		//status 2 means "quit" was typed and we need to free.
        if(status != 0)
        {
			//if the user typed "exit" then the return status is 2
			if(status == 2)
			{
				write(1, "Exiting shell program...\n", 25);
				free(inputLine);
				free(args);
				_exit(0);
			}
            write(1, "Command execution failed...\n", 28);
        }
		free(inputLine);
        free(args);
    }

}

//this function just continually reads input from stdin until a newline char is found
char* readInputLine(void)
{
    int ch = '\0';
    int bufferSize = 256;
    int pos = 0;
	//preallocate a buffer of 256 for the command
    char* buffer = malloc(sizeof(char) * bufferSize);
    
    if(buffer == NULL)
    {
        write(1,"malloc allocation failure\n", 26);
        _exit(1);
    }

	//loop through and read until the newline char is found
    while(read(0,&ch, 1) > 0)
    {
        if(ch == '\n')
        {
			//terminate our string and return
            buffer[pos] = '\0';
            return buffer;
        }
        else
        {
			//add char to the buffer
            buffer[pos] = ch;
            pos++;
        }

		//if we run out of space in the buffer then reallocate 2x the size
        if(pos >= bufferSize)
        {
            bufferSize *= 2;
            buffer = realloc(buffer, bufferSize);
            if(buffer == NULL)
            {
                write(1,"realloc allocation failure\n", 27);
                _exit(1);
            }
        }
    }
    return NULL;
}

//this method splits the input line into an array of arguments
char** splitInputLine(char* inputLine)
{
    int bufferSize = 256;
    int pos = 0;
	//preallocate a buffer of 256 for the args
    char** args = malloc(sizeof(char*) * bufferSize);
    char* currentArg;

    if(args == NULL)
    {
        write(1,"malloc allocation failure\n", 26);
        _exit(1);
    }

	//split the input line by spaces
    currentArg = strtok(inputLine, " ");

	//loop through all the arguments and add them to the args array
    while(currentArg != NULL)
    {
        args[pos] = currentArg;
        pos++;

		//reallocate to 2x the buffer size if necessary
        if(pos >= bufferSize)
        {
            bufferSize *= 2;
            args = realloc(args, sizeof(char*) * bufferSize);
            if(args == NULL)
            {
                write(1,"malloc allocation failure\n", 26);
                _exit(1);
            }
        }
        currentArg = strtok(NULL, " ");
    }
	//set the last position to null
    args[pos] = NULL;
    return args;
}

//this method is for executing the command given the arguments
int executeCommand(char** args)
{
    int pos = 0;

	//check if no input was given
    if(args[pos] == NULL)
    {
        return 1;
    }
	//check if quit was typed
    if(strcmp(args[pos], "quit") == 0)
    {
        return 2;
    }
	
	//parse the input and output redirection arguments
	char* inputRedir = inputRedirection(args);
	char* outputRedir = outputRedirection(args);
	
	//count the number of commands and the max number of arguments out of all the commands
	int numbCMDS = numbOfCommands(args);
	int numbARGS = maxNumbOfArgs(args);
	
	//parse all arguments into the command array
	char *cmd[numbCMDS][numbARGS];
	int i = 0;
	int j = 0;
	int k = 0;
	while(args[i] != NULL)
	{
		//skip any input/output redirection since we already parsed it
		if(strcmp(args[i], "<") == 0 || strcmp(args[i], ">") == 0)
		{
			i += 2;
			continue;
		}
		//if there is a pipe then its a new command
		if(strcmp(args[i], "|") == 0)
		{
			cmd[j][k] = NULL;
			i++;
			j++;
			k = 0;
			continue;
		}
		cmd[j][k] = args[i];
		i++;
		k++;
	}
	//make sure the last command is null terminated
	cmd[j][k] = NULL;

	
	//piping / execution code
	int apipe[2];
	int isParent;
	int lastChild, fd1, fd2, saveStdout;
	int status;
	saveStdout = dup(1);
	//loop through all the commands and generate the pipe structure
	for(i = (numbCMDS-1); i >= 0; i--) {
		pipe(apipe);
		isParent = fork();

		if (!isParent) {  // execute the sub-programs
			close(apipe[1]); // close write end of pipe for children
			close(0); // close stdin for all processes
			if(i != 0) { dup(apipe[0]); } // if not the first command, clone pipe read end into stdin
			if(i == 0 && inputRedir != NULL) { //check if there is input redirection
				fd1 = open(inputRedir, O_RDONLY);
				if(fd1 == -1)
				{
					write(1,"open() has failed...\n", 21);
					_exit(1);
				}
				dup(fd1); // clone fd of inputRedir.c to stdin for first command
			}
			
			if(i == (numbCMDS-1) && outputRedir != NULL) { //check if there is output redirection
				close(1);
				fd2 = open(outputRedir, O_RDWR | O_CREAT | O_TRUNC, 0666);
				if(fd2 == -1)
				{
					write(1,"open() has failed...\n", 21);
					_exit(1);
				}
				dup(fd2); // clone fd of outputRedir to stdout for first command
			}
			
			close(apipe[0]); // close extra read end of pipe

			execvp(cmd[i][0],cmd[i]);
			write(1, "Command execution failed...\n", 28);
			_exit(0);
		}
		else 
		{
			if(i==(numbCMDS-1)) lastChild = isParent; // save last child's pid 
			close(apipe[0]); // close read end of pipe for parent
			close(1); // close stdout
			if(i!=0) { 
				dup(apipe[1]); 	
			} // if not first command, duplicate pipe write end into stdout
			close(apipe[1]); // close extra pipe end					
			if(i==0){ 
				dup2(saveStdout, 1); // if in shell, restore stdout to terminal	
				waitpid(lastChild,&status,0);
				//grab exit status incase it is needed
				status = WEXITSTATUS(status);
			}
		}
	}
	
    return 0;
}

//method to parse the number of commands
int numbOfCommands(char** args)
{
	//we simply count the number of pipes + 1
	int i = 1;
	int numbOfCommands = 1;
	while(args[i] != NULL)
	{
		if(strcmp(args[i], "|") == 0)
		{
			numbOfCommands += 1;	
		}
		i++;
	}
	return numbOfCommands;
}

//method to parse the max number of args
int maxNumbOfArgs(char** args)
{
	int i = 0;
	int max = 1;
	
	int maxNumbOfArgs = 0;
	while(args[i] != NULL)
	{
		//skip any redirection operators
		if(strcmp(args[i], "<") == 0 || strcmp(args[i], ">") == 0)
		{
			i += 2;
			continue;
		}
		//if we have a pipe then we compare the number of args to the previous max
		if(strcmp(args[i], "|") == 0)
		{
			if(maxNumbOfArgs > max)
			{
				max = maxNumbOfArgs;
			}
			maxNumbOfArgs = 0;
			i++;
		}
		else
		{
			maxNumbOfArgs += 1;
			i++;	
		}
	}
	
	//this is in case no pipes are in the command
	if(maxNumbOfArgs > max)
	{
		max = maxNumbOfArgs;
	}
	//returning +1 because of null termination
	return (max + 1);
}

//method to parse the input redirection path/name
char* inputRedirection(char** args)
{
	int pos = 0;
	while(args[pos] != NULL)
	{
		if(strcmp(args[pos], "<") == 0 && args[pos+1] != NULL)
		{
			return args[pos+1];
		}
		pos++;
	}
	return NULL;
}

//method to parse the output redirection path/name
char* outputRedirection(char** args)
{
	int pos = 0;
	while(args[pos] != NULL)
	{
		if(strcmp(args[pos], ">") == 0 && args[pos+1] != NULL)
		{
			return args[pos+1];
		}
		pos++;
	}
	return NULL;
}
