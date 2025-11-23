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
                    snprintf(file_buffer, sizeof(file_buffer), "[INFO] %s\n", log_text);
                    break;
                case LOG_WARN:
                    snprintf(file_buffer, sizeof(file_buffer), "[WARN] %s\n", log_text);
                    break;
                case LOG_ERROR:
                    snprintf(file_buffer, sizeof(file_buffer), "[ERROR] %s\n", log_text);
                    break;
                default:
                    snprintf(file_buffer, sizeof(file_buffer), "[UNSPEC] %s\n", log_text);
            }
        } else {
            switch (type) {
                case LOG_INFO:
                    snprintf(file_buffer, sizeof(file_buffer), "[INFO] %s", log_text);
                    break;
                case LOG_WARN:
                    snprintf(file_buffer, sizeof(file_buffer), "[WARN] %s", log_text);
                    break;
                case LOG_ERROR:
                    snprintf(file_buffer, sizeof(file_buffer), "[ERROR] %s", log_text);
                    break;
                default:
                    snprintf(file_buffer, sizeof(file_buffer), "[UNSPEC] %s", log_text);
            }
        }
        fprintf(file, "%s", file_buffer);
    }

    // For console output: only show prefix if type_prefix is set
    if (type_prefix) {
        if (newline) {
            switch (type) {
                case LOG_INFO:
                    snprintf(console_buffer, sizeof(console_buffer), "[INFO] %s\n", log_text);
                    break;
                case LOG_WARN:
                    snprintf(console_buffer, sizeof(console_buffer), "[WARN] %s\n", log_text);
                    break;
                case LOG_ERROR:
                    snprintf(console_buffer, sizeof(console_buffer), "[ERROR] %s\n", log_text);
                    break;
                default:
                    snprintf(console_buffer, sizeof(console_buffer), "[UNSPEC] %s\n", log_text);
            }
        } else {
            switch (type) {
                case LOG_INFO:
                    snprintf(console_buffer, sizeof(console_buffer), "[INFO] %s", log_text);
                    break;
                case LOG_WARN:
                    snprintf(console_buffer, sizeof(console_buffer), "[WARN] %s", log_text);
                    break;
                case LOG_ERROR:
                    snprintf(console_buffer, sizeof(console_buffer), "[ERROR] %s", log_text);
                    break;
                default:
                    snprintf(console_buffer, sizeof(console_buffer), "[UNSPEC] %s", log_text);
            }
        }
    } else {
        if (newline)
            snprintf(console_buffer, sizeof(console_buffer), "%s\n", log_text);
        else
            snprintf(console_buffer, sizeof(console_buffer), "%s", log_text);
    }

    printf("%s", console_buffer);
}