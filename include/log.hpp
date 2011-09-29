#ifndef LOG_HPP
#define LOG_HPP

#include <stdio.h>
#include <time.h>

// For things that get called MANY times per chunk of work.
#define LOG_LEVEL_DEBUG3 0
// DEBUG2 is for things that happen many times per chunk of work.
#define LOG_LEVEL_DEBUG2 1
// DEBUG1 is for things that happen more than once per chunk of work.
#define LOG_LEVEL_DEBUG1 2
// DEBUG is reserved for things that are dev-oriented messages, like INFO but for devs not users.
#define LOG_LEVEL_DEBUG 3
// NOTE: INFO marks the (inclusive) cutoff for verbosity that is parsible by a person when processing an average sized work load. Anything that would balloon output past human-readable for an average work load needs DEBUG1-DEBUG3
// INFO is for things that happen once, and are sub-milestone progress or are overly verbose
#define LOG_LEVEL_INFO 4
// NORMAL is for things that happen once or are milestone (that is, progress indicator) related.
#define LOG_LEVEL_NORMAL 5
// WARN is for non-critical errors
#define LOG_LEVEL_WARN 6
// CRITICAL is for errors that halt the normal operation of the program.
#define LOG_LEVEL_CRITICAL 7

#define LOG_LEVEL LOG_LEVEL_INFO

#if LOG_LEVEL <= LOG_LEVEL_DEBUG
#define DEBUG
#endif

#define LOG_LEVEL_MESSAGE_HEADER(loglvl) \
(loglvl == LOG_LEVEL_DEBUG3 ? "[DEBUG3] " : \
(loglvl == LOG_LEVEL_DEBUG2 ? "[DEBUG2] " : \
(loglvl == LOG_LEVEL_DEBUG1 ? "[DEBUG1] " : \
(loglvl == LOG_LEVEL_DEBUG ? "[DEBUG] " : \
(loglvl == LOG_LEVEL_INFO ? "[INFO] " : \
(loglvl == LOG_LEVEL_NORMAL ? "[NORMAL] " : \
(loglvl == LOG_LEVEL_WARN ? "[WARN] " : \
(loglvl == LOG_LEVEL_CRITICAL ? "[CRITICAL] " : "[SUPERCRITICAL] "\
))))))))

#define LOG_LEVEL_MESSAGE_DESCRIPTOR(loglvl) \
(loglvl == LOG_LEVEL_DEBUG3 ? stdout : \
(loglvl == LOG_LEVEL_DEBUG2 ? stdout : \
(loglvl == LOG_LEVEL_DEBUG1 ? stdout : \
(loglvl == LOG_LEVEL_DEBUG ? stdout : \
(loglvl == LOG_LEVEL_INFO ? stdout : \
(loglvl == LOG_LEVEL_NORMAL ? stdout : \
(loglvl == LOG_LEVEL_WARN ? stderr : \
(loglvl == LOG_LEVEL_CRITICAL ? stderr : stderr\
))))))))

//Reference: http://stackoverflow.com/questions/679979/c-c-how-to-make-a-variadic-macro-variable-number-of-arguments
#define LOG_MESSAGE_F(loglvl, fp, ...) \
if (LOG_LEVEL <= loglvl) \
{ \
print_cur_time_stamp(LOG_LEVEL_MESSAGE_DESCRIPTOR(loglvl));\
fprintf(fp, LOG_LEVEL_MESSAGE_HEADER(loglvl)); \
fprintf(fp, ##__VA_ARGS__);\
}

#define LOG_MESSAGE(loglvl, ...) LOG_MESSAGE_F(loglvl, LOG_LEVEL_MESSAGE_DESCRIPTOR(loglvl), ##__VA_ARGS__)

// Reference: http://www.cplusplus.com/reference/clibrary/ctime/strftime/
static inline void print_cur_time_stamp(FILE* f)
{
    time_t rawtime;
    struct tm* timeinfo;
    // Assume that we'll never have more than 80 characters of timestamp data.
    char buffer[80];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, 80, "%b %d %H:%M:%S", timeinfo);
    fprintf(f, "%s ", buffer);
}

#endif
