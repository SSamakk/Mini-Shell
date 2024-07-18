
/*
 * CS-413 Spring 98
 * shell.y: parser for shell
 */

%union	{
		char   *string_val;
	}

%{
extern "C" 
{
	int yylex();
	void yyerror (char const *s);
}

#define yylex yylex
#include <stdio.h>
#include "command.h"
%}

%token	<string_val> WORD
%token NOTOKEN GREAT LESS APPEND PIPE AMPERSAND NEWLINE
%type <Command> goal
%token EXIT

%%

goal:	
	commands
	| EXIT { exit(0); }
	;

commands: 
	command
	| commands command 
	;

command:
    simple_command io_redirections_opt background_opt NEWLINE {
        printf("   Yacc: Executing command\n");
        Command::_currentCommand.execute();
        // Add execution logic for the Command
    }
    | error NEWLINE {
        yyerrok;
    }
    ;

io_redirections_opt:
    | io_redirections
    ;

io_redirections:
    io_redirection
    | io_redirections io_redirection
    ;

io_redirection:
    GREAT WORD {
        Command::_currentCommand._outFile = $2;
        Command::_currentCommand._outFileAppend = false; // Overwrite mode
    }
    | LESS WORD { Command::_currentCommand._inputFile = $2; }
    | APPEND WORD { 
        // Assign the _outFile and set the append flag
        Command::_currentCommand._outFile = $2;
        Command::_currentCommand._outFileAppend = true; // Append mode
    }
    | PIPE WORD {
        char pipeSymbol[] = "|"; // Create a buffer for the pipe symbol
        Command::_currentSimpleCommand->insertArgument(pipeSymbol);
        Command::_currentSimpleCommand->insertArgument($2);
    }
    ;

background_opt:
    | AMPERSAND { Command::_currentCommand._background = 1; }
    ;

simple_command:
    command_and_args
    | command_and_args PIPE simple_command
    ;

command_and_args:
	command_word arg_list {
		Command::_currentCommand.
			insertSimpleCommand( Command::_currentSimpleCommand );
	}
	;

arg_list:
    | arg_list argument
    ;

argument:
	WORD {
               printf("   Yacc: insert argument \"%s\"\n", $1);

	       Command::_currentSimpleCommand->insertArgument( $1 );
	}
	;

command_word:
	WORD {
               printf("   Yacc: insert command \"%s\"\n", $1);
	       
	       Command::_currentSimpleCommand = new SimpleCommand();
	       Command::_currentSimpleCommand->insertArgument( $1 );
	}
	;

%%

void yyerror(const char *s) {
    fprintf(stderr, "%s\n", s);
}

