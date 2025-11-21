#include <stdio.h>
#include "types.h"
#include "logging.h"

char log_buffer[MAX_LOG_LENGTH];

void logging(char* log_text, int file_write, int newline, int type_prefix, FILE* file, LOGGING_TYPES type) {
    char file_buffer[MAX_LOG_LENGTH];
    char console_buffer[MAX_LOG_LENGTH];

    // For file output: always include prefix and type
    if (file_write) {
        if (newline) {
            switch (type) {
                case LOG_INFO:
                    sprintf(file_buffer, "[INFO] %s\n", log_text);
                    break;
                case LOG_WARN:
                    sprintf(file_buffer, "[WARN] %s\n", log_text);
                    break;
                case LOG_ERROR:
                    sprintf(file_buffer, "[ERROR] %s\n", log_text);
                    break;
                default:
                    sprintf(file_buffer, "[UNSPEC] %s\n", log_text);
            }
        } else {
            switch (type) {
                case LOG_INFO:
                    sprintf(file_buffer, "[INFO] %s", log_text);
                    break;
                case LOG_WARN:
                    sprintf(file_buffer, "[WARN] %s", log_text);
                    break;
                case LOG_ERROR:
                    sprintf(file_buffer, "[ERROR] %s", log_text);
                    break;
                default:
                    sprintf(file_buffer, "[UNSPEC] %s", log_text);
            }
        }
        fprintf(file, "%s", file_buffer);
    }

    // For console output: only show prefix if type_prefix is set
    if (type_prefix) {
        if (newline) {
            switch (type) {
                case LOG_INFO:
                    sprintf(console_buffer, "[INFO] %s\n", log_text);
                    break;
                case LOG_WARN:
                    sprintf(console_buffer, "[WARN] %s\n", log_text);
                    break;
                case LOG_ERROR:
                    sprintf(console_buffer, "[ERROR] %s\n", log_text);
                    break;
                default:
                    sprintf(console_buffer, "[UNSPEC] %s\n", log_text);
            }
        } else {
            switch (type) {
                case LOG_INFO:
                    sprintf(console_buffer, "[INFO] %s", log_text);
                    break;
                case LOG_WARN:
                    sprintf(console_buffer, "[WARN] %s", log_text);
                    break;
                case LOG_ERROR:
                    sprintf(console_buffer, "[ERROR] %s", log_text);
                    break;
                default:
                    sprintf(console_buffer, "[UNSPEC] %s", log_text);
            }
        }
    } else {
        if (newline)
            sprintf(console_buffer, "%s\n", log_text);
        else
            sprintf(console_buffer, "%s", log_text);
    }

    printf("%s", console_buffer);
}