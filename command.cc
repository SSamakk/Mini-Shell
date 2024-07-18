
/*
 * CS354: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>

#include "command.h"

SimpleCommand::SimpleCommand()
{
	// Creat available space for 5 arguments
	_numberOfAvailableArguments = 5;
	_numberOfArguments = 0;
	_arguments = (char **) malloc( _numberOfAvailableArguments * sizeof( char * ) );
}

void
SimpleCommand::insertArgument( char * argument )
{
	if ( _numberOfAvailableArguments == _numberOfArguments  + 1 ) {
		// Double the available space
		_numberOfAvailableArguments *= 2;
		_arguments = (char **) realloc( _arguments,
				  _numberOfAvailableArguments * sizeof( char * ) );
	}
	
	_arguments[ _numberOfArguments ] = argument;

	// Add NULL argument at the end
	_arguments[ _numberOfArguments + 1] = NULL;
	
	_numberOfArguments++;
}

Command::Command()
{
	// Create available space for one simple command
	_numberOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)
		malloc( _numberOfSimpleCommands * sizeof( SimpleCommand * ) );

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
}

void
Command::insertSimpleCommand( SimpleCommand * simpleCommand )
{
	if ( _numberOfAvailableSimpleCommands == _numberOfSimpleCommands ) {
		_numberOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **) realloc( _simpleCommands,
			 _numberOfAvailableSimpleCommands * sizeof( SimpleCommand * ) );
	}
	
	_simpleCommands[ _numberOfSimpleCommands ] = simpleCommand;
	_numberOfSimpleCommands++;
}

void
Command:: clear()
{
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		for ( int j = 0; j < _simpleCommands[ i ]->_numberOfArguments; j ++ ) {
			free ( _simpleCommands[ i ]->_arguments[ j ] );
		}
		
		free ( _simpleCommands[ i ]->_arguments );
		free ( _simpleCommands[ i ] );
	}

	if ( _outFile ) {
		free( _outFile );
	}

	if ( _inputFile ) {
		free( _inputFile );
	}

	if ( _errFile ) {
		free( _errFile );
	}

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
}

void
Command::print()
{
	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  --- ----------------------------------------------------------\n");
	
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		printf("  %-3d ", i );
		for ( int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++ ) {
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[ j ] );
		}
	}

	printf( "\n\n" );
	printf( "  Output       Input        Error        Background\n" );
	printf( "  ------------ ------------ ------------ ------------\n" );
	printf( "  %-12s %-12s %-12s %-12s\n", _outFile?_outFile:"default",
		_inputFile?_inputFile:"default", _errFile?_errFile:"default",
		_background?"YES":"NO");
	printf( "\n\n" );
	
}

void Command::execute() {
    // Execution logic for Internal Commands
    if (_numberOfSimpleCommands == 1 && strcmp(_simpleCommands[0]->_arguments[0], "EXIT") == 0) {
        printf("Exiting the shell.\n");
        exit(0); // Exiting the shell without forking a new process
    } 
    else if (_numberOfSimpleCommands == 1 && strcmp(_simpleCommands[0]->_arguments[0], "cd") == 0) {
        if (_simpleCommands[0]->_numberOfArguments == 1) {
            // Change to the home directory
            chdir(getenv("HOME"));
        } 
        else if (_simpleCommands[0]->_numberOfArguments == 2) {
            // Change to the specified directory
            if (chdir(_simpleCommands[0]->_arguments[1]) != 0) {
                perror("cd");
            }
        } 
        else {
            // Invalid usage of cd (wrong grammer)
            printf("Usage: cd [directory]\n");
        }

        // Display the prompt after processing "cd" command
        prompt();
    }
    else {
        // Execution logic for external commands
        int status;
        int prev_pipe[2] = {0, 0}; // File descriptors for the previous pipe

        for (int i = 0; i < _numberOfSimpleCommands; i++) {
            int pid;
            int current_pipe[2] = {0, 0};

            if (i < _numberOfSimpleCommands - 1) {
                if (pipe(current_pipe) == -1) {
                    perror("Pipe creation failed");
                    exit(1);
                }
            }

            pid = fork();

            if (pid < 0) {
                perror("Fork failed");
                exit(1);
            } else if (pid == 0) {
                // Child process

                // Connect input from the previous pipe (if not the first command)
                if (i != 0) {
                    dup2(prev_pipe[0], 0);
                    close(prev_pipe[0]); //Closes the read end of the previous pipe in the child process.
                    close(prev_pipe[1]); //Closes the write end of the previous pipe in the child process.
                }

                // Connect output to the current pipe (if not the last command)
                if (i != _numberOfSimpleCommands - 1) {
                    close(current_pipe[0]);
                    dup2(current_pipe[1], 1);
                    close(current_pipe[1]);
                }

                // Redirect input/output/error files if specified
                if (_inputFile) {
                    int infd = open(_inputFile, O_RDONLY);
                    if (infd < 0) {
                        perror("Error opening input file");
                        exit(1);
                    }
                    dup2(infd, 0);
                    close(infd);
                }

                if (i == _numberOfSimpleCommands -1 && _outFile) {
                    int outfd;
                    if (_outFileAppend) {
                        outfd = open(_outFile, O_WRONLY | O_CREAT | O_APPEND, 0666);
                    } else {
                        outfd = open(_outFile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                    }
                    if (outfd < 0) {
                        perror("Error opening output file");
                        exit(1);
                    }
                    dup2(outfd, 1);
                    close(outfd);
                }

                if (_errFile) {
                    int errfd;
                    if (_errFileAppend) {
                        errfd = open(_errFile, O_WRONLY | O_CREAT | O_APPEND, 0666);
                    } else {
                        errfd = open(_errFile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                    }
                    if (errfd < 0) {
                        perror("Error opening error file");
                        exit(1);
                    }
                    dup2(errfd, 2);
                    close(errfd);
                }

                // Check if the command is a built-in command
                if (strcmp(_simpleCommands[i]->_arguments[0], "cd") == 0 || strcmp(_simpleCommands[i]->_arguments[0], "EXIT") == 0) {
                    // Handle built-in commands without invoking execvp
                    exit(0); // Ensure the child process exits
                }

                // Execute external command
                execvp(_simpleCommands[i]->_arguments[0], _simpleCommands[i]->_arguments);
                perror("Execvp failed");
                exit(1);
            } else {
                // Parent process

                if (i != 0) {
                    close(prev_pipe[0]);
                    close(prev_pipe[1]);
                }
                if (i < _numberOfSimpleCommands - 1) {
                    prev_pipe[0] = current_pipe[0]; //It's essentially setting up the connection between the output of the current command and the input of the next command in the pipeline.
                    prev_pipe[1] = current_pipe[1]; //This ensures that the output of the current command will be written to the input of the next command.
                }

                if (!_background && i == _numberOfSimpleCommands - 1) {
                    waitpid(pid, &status, 0);
                }
            }
        }

        for (int i = 0; i < _numberOfSimpleCommands; i++) {
            for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++) {
                free(_simpleCommands[i]->_arguments[j]);
            }
            free(_simpleCommands[i]->_arguments);
            free(_simpleCommands[i]);
        }

        if (_outFile) {
            free(_outFile);
        }

        if (_inputFile) {
            free(_inputFile);
        }

        if (_errFile) {
            free(_errFile);
        }

        _numberOfSimpleCommands = 0;
        _outFile = 0;
        _inputFile = 0;
        _errFile = 0;
        _background = 0;

        prompt();
    }
}



volatile sig_atomic_t signal_interrupt = 0;

void sigint_handler(int sig) {
    signal_interrupt = 1;
    printf("\n");
    Command::_currentCommand.prompt();
}

void sigchld_handler(int signum) {
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        // Child process with PID 'pid' terminated
        FILE *logFile = fopen("child_termination_log.txt", "a");
        if (logFile != NULL) {
            fprintf(logFile, "Child process with PID %d terminated.\n", pid);
            fclose(logFile);
        }
        else {
            perror("Error opening log file");
        }
    }
}


void ctrlc_handler(int signum) {
    printf("\n");
    Command::_currentCommand.prompt();
}

void
Command::prompt()
{
	printf("myshell>");
	fflush(stdout);
}

Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;


int yyparse(void);

int 
main()
{
    signal(SIGINT, ctrlc_handler);
    signal(SIGCHLD, sigchld_handler);
    while (true) {
        Command::_currentCommand.prompt();
        yyparse();
    }
    return 0;
}

