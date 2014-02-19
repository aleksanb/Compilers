#include "symtab.h"
#include <stdlib.h>

/*
 * Macro for creating a heap-allocated duplicate of a string.
 * This macro mirrors the function 'strdup' (which is itself a pretty
 * common standard extension by GCC and many others). The function is not
 * part of the C99 standard because it allocates heap memory as a
 * side-effect, so it is reimplemented here in terms of std. calls.
 */
#define STRDUP(s) strncpy ( (char*)malloc ( strlen(s)+1 ), s, strlen(s)+1 )

// These variables is located in vslc.c
extern int arch;
extern int outputStage; 

static char **strings;
static int strings_size = 16, strings_index = -1;


void symtab_init ( void )
{
    strings = malloc(sizeof(char *) * strings_size);
}


void symtab_finalize ( void )
{
    for (int i=0; i<strings_index; i++) {
        free(strings[i]);
    }
    free(strings);

    strings_index = -1;
}


int strings_add ( char *str )
{

    // Increment string pointer
    strings_index++;

    /*// Allocate more memory if needed
    if (strings_index >= strings_size) {
        int new_strings_size = (strings_size * 3)/2 + 1;
        strings = realloc(strings, sizeof(char *) * new_strings_size);
        //printf("Alloccing moar memories, now %d up from %d", new_strings_size, strings_size);
        strings_size = new_strings_size;
    }

    // Insert new string
    strings[strings_index] = STRDUP(str);*/

    if(outputStage == 7)
        printf("\n Meanwhile inside: %d, %s", strings_index, str);
        printf ( stderr, "Add strings (%s), index: %d \n", str, strings_index);

    return strings_index;
}

// Prints the data segment of the assembly code
// which includes all the string constants found
// ARM and x86 have different formats
void strings_output ( FILE *stream )
{
    if(arch == 1) { //ARM
    	 fputs (
			".syntax unified\n"
			".cpu cortex-a15\n"
			".fpu vfpv3-d16\n"
			".data\n"
			".align	2\n"
			".DEBUG: .ascii \"Hit Debug\\n\\000\"\n"
			".DEBUGINT: .ascii \"Hit Debug, r0 was: %d\\n\\000\"\n"
		    ".INTEGER: .ascii \"%d \\000\"\n"
			".FLOAT: .ascii \"%f \\000\"\n"
			".NEWLINE: .ascii \"\\n \\000\"\n",
		    stream
		);
		for ( int i=0; i<=strings_index; i++ ) {
		    fprintf ( stream, ".STRING%d: .ascii %s\n", i, strings[i] );
		    fprintf ( stream, ".ascii \"\\000\"\n", i, strings[i] ); // ugly hack
		}
		fputs ( ".globl main\n", stream );
		fputs ( ".align	2\n", stream );
    }
    else { //x86
		fputs (
		    ".data\n"
		    ".INTEGER: .string \"%d \"\n"
			".FLOAT: .string \"%f \"\n"
			".NEWLINE: .string \"\\n \"\n",
		    stream
		);
		for ( int i=0; i<=strings_index; i++ )
		    fprintf ( stream, ".STRING%d: .string %s\n", i, strings[i] );
		fputs ( ".globl main\n", stream );
    }
}

