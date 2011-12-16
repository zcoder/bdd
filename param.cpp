#include <getopt.h>
#include <stdlib.h>

#include "param.h"

const char*  p_if = NULL;
const char*  p_of = NULL;
const char*  p_mf = NULL;

bool         p_truncate = false;

const static char options[] = "i:o:m:t";
const static struct option long_options[] = {
        { "infile",     1,  0,  'i' },
        { "outfile",    1,  0,  'o' },
        { "mapfile",    1,  0,  'm' },
        { "truncate",   0,  0,  't' },
        { 0,            0,  0,   0  }
    };

void params(int argc, char* argv[])
{
int option;
int option_index;

while( 1 )
    {
    option = getopt_long(argc, argv, options, long_options, &option_index);
    if ( option == -1 )
        break;
    switch( option )
        {
        case 'i':
            {
            p_if = optarg;
            }
            break;
        case 'o':
            {
            p_of = optarg;
            }
            break;
        case 'm':
            {
            p_mf = optarg;
            }
            break;
        case 't':
            {
            p_truncate = true;
            }
            break;
        default:
            {
            exit( EXIT_FAILURE );
            }
            break;
        }
    }
if ( optind < argc )
    {
    exit( EXIT_FAILURE );
    }
}

