#include "errors.h"

#include <stdio.h>

void parser_error(const char* message)
{
    fprintf(stderr, "Parser error: %s\n", message);
}