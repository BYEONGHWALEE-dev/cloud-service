// include/logger.h

#ifndef LOGGER_H
#define LOGGER_H

typedef enum {
    LOG_ERROR = 0,
    LOG_WARN  = 1,
    LOG_INFO  = 2,
    LOG_DEBUG = 3
} log_level_t;

// 전역 로그 레벨
extern log_level_t g_log_level;

// 로그 레벨 설정
void log_set_level(log_level_t level);

// 로그 함수들
void log_error(const char *format, ...);
void log_warn(const char *format, ...);
void log_info(const char *format, ...);
void log_debug(const char *format, ...);

// 매크로 (조건부 로깅)
#define LOG_ERROR(...) log_error(__VA_ARGS__)
#define LOG_WARN(...)  log_warn(__VA_ARGS__)
#define LOG_INFO(...)  log_info(__VA_ARGS__)
#define LOG_DEBUG(...) log_debug(__VA_ARGS__)

#endif // LOGGER_H
