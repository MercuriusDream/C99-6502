#include <stdio.h>
#include "types.h"
#include "logging.h"

static LOGGING_TYPES g_min_level = LOG_INFO;

void logging_set_level(LOGGING_TYPES level) {
    g_min_level = level;
}

void logging(char* log_text, int file_write, int newline, int type_prefix, FILE* file, LOGGING_TYPES type) {
    char file_buffer[MAX_LOG_LENGTH];
    char console_buffer[MAX_LOG_LENGTH];

    /* Log-level filtering: drop messages below the configured minimum. */
    if (type < g_min_level) {
        return;
    }

    /* For file output: always include prefix and type. */
    if (file_write && file) {
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

    /* For console output: only show prefix if type_prefix is set. */
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

    /* WARN/ERROR go to stderr; INFO stays on stdout. */
    if (type == LOG_WARN || type == LOG_ERROR) {
        fprintf(stderr, "%s", console_buffer);
    } else {
        printf("%s", console_buffer);
    }
}