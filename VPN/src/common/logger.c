// src/common/logger.c

#include "logger.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

log_level_t g_log_level = LOG_INFO;

void log_set_level(log_level_t level) {
    g_log_level = level;
}

static void log_print(log_level_t level, const char *level_str, const char *format, va_list args) {
    if (level > g_log_level) {
        return;
    }
    
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_buf[64];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);
    
    printf("[%s] [%s] ", time_buf, level_str);
    vprintf(format, args);
    printf("\n");
}

void log_error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_print(LOG_ERROR, "ERROR", format, args);
    va_end(args);
}

void log_warn(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_print(LOG_WARN, "WARN", format, args);
    va_end(args);
}

void log_info(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_print(LOG_INFO, "INFO", format, args);
    va_end(args);
}

void log_debug(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_print(LOG_DEBUG, "DEBUG", format, args);
    va_end(args);
}
