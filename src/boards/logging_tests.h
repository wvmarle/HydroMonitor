#ifndef HYDROMONITOR_LOGGING_TESTS_h
#define HYDROMONITOR_LOGGING_TESTS_h

// How to do the logging.
#define LOG_SERIAL  // Send log info to the Serial console.
#define LOG_MYSQL   // Send log info to the MySQL database.
#define USE_SERIAL

#define OTA_PASSWORD "esp"
#define LOGGING_USERNAME "sql_username"           // MySQL user name. It's a string, so needs the quote marks.
#define LOGGING_PASSWORD "sql_password"           // MySQL password. It's a string, so needs the quote marks.

// Set the log level.
#define LOGLEVEL LOG_TRACE
//#define LOGLEVEL LOG_INFO
//#define LOGLEVEL LOG_WARNING
//#define LOGLEVEL LOG_ERROR
//#define LOGLEVEL LOG_OFF

#endif
