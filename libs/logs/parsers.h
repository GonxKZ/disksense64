#include <stdio.h>

#ifndef LIBS_LOGS_PARSERS_H
#define LIBS_LOGS_PARSERS_H

#include "logs.h"

#ifdef __cplusplus
extern "C" {
#endif

// Parse syslog format
int log_parse_syslog(const char* data, log_result_t* result);

// Parse Apache log format
int log_parse_apache(const char* data, log_result_t* result);

// Parse Nginx log format
int log_parse_nginx(const char* data, log_result_t* result);

// Parse JSON log format
int log_parse_json(const char* data, log_result_t* result);

// Parse CSV log format
int log_parse_csv(const char* data, log_result_t* result);

// Parse custom log format
int log_parse_custom(const char* data, log_result_t* result);

// Export to syslog format
int log_export_syslog(const log_result_t* result, FILE* file);

// Export to Apache log format
int log_export_apache(const log_result_t* result, FILE* file);

// Export to Nginx log format
int log_export_nginx(const log_result_t* result, FILE* file);

// Export to JSON log format
int log_export_json(const log_result_t* result, FILE* file);

// Export to CSV log format
int log_export_csv(const log_result_t* result, FILE* file);

// Export to custom log format
int log_export_custom(const log_result_t* result, FILE* file);

#ifdef __cplusplus
}
#endif

#endif // LIBS_LOGS_PARSERS_H