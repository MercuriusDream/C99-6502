#ifndef LOGGING_H
#define LOGGING_H

#include <stdio.h>
#include "types.h"

void logging_set_level(LOGGING_TYPES level);

void logging(char* log_text, int file_write, int newline, int type_prefix, FILE* file, LOGGING_TYPES type);

#endif