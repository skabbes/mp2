/*
 *  key_gen_test.c
 *
 *  Description:
 *      This utility will display the key ID of the specified file
 *
 *  Portability Issues:
 *      None.
 */

#include <stdio.h>
#include <math.h>
#include <string.h>
#ifdef WIN32
#include <io.h>
#endif
#include <fcntl.h>
#include "sha1.h"

/*
 *  Function prototype
 */
void usage();


/*  
 *  main
 *
 *  Description:
 *      This is the entry point for the program
 *
 *  Parameters:
 *      argc: [in]
 *          This is the count of arguments in the argv array
 *      argv: [in]
 *          A filename for which to compute key ID
 *
 *  Returns:
 *      Nothing.
 *
 *  Comments:
 *
 */
int main(int argc, char *argv[])
{
    SHA1Context sha;                /* SHA-1 context                 */
    FILE        *fp;                /* File pointer for reading files*/
    char        c;                  /* Character read from file      */
    int         i;                  /* Counter                       */
    int         reading_stdin;      /* Are we reading standard in?   */
    int         read_stdin = 0;     /* Have we read stdin?           */
    int		key_id ;

    int		m = 10; // size of output (in bit). Let's try 10 for test.
    /*
     *  Check the program arguments and print usage information 
     */
    if (argc != 2)
    {
        usage();
        return 1;
    }

    /*
     *  For the filename passed in on the command line, calculate the
     *  SHA-1 value and display it.
     */

    /*
     *  Reset the SHA-1 context and process input
     */
    SHA1Reset(&sha);

    SHA1Input(&sha, argv[1], strlen(argv[1]));


    if (!SHA1Result(&sha))
    {
		fprintf(stderr, "key_gen_test: could not compute key ID for %s\n", argv[1]);
	}
	else
	{
		key_id = sha.Message_Digest[4] % ((int)pow(2,m)) ;
		printf( "Key ID for %s : %d\n", argv[1], key_id) ;
	}
    
    return 0;
}

/*  
 *
 *  Description:
 *      This function will display program usage information to the
 *      user.
 *
 *  Parameters:
 *      None.
 *
 *  Returns:
 *      Nothing.
 *
 *  Comments:
 *
 */
void usage()
{
    printf("usage: key_gen_test <file_name>\n");
    printf("\tThis program will display a key ID for an input file\n");
}
