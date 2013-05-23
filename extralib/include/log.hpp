/* MPL2.0 HEADER START
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MPL2.0 HEADER END
 *
 * Copyright 2010-2013 Michael Himbeault and Travis Friesen
 *
 */

/// Provides facilities for logging information to files or standard output/error
/// @file log.hpp

#ifndef LOG_HPP
#define LOG_HPP

#include <stdio.h>
#include <time.h>

/// For things that get called MANY times per chunk of work.
#define LOG_LEVEL_DEBUG3 0
/// DEBUG2 is for things that happen many times per chunk of work.
#define LOG_LEVEL_DEBUG2 1
/// DEBUG1 is for things that happen more than once per chunk of work.
#define LOG_LEVEL_DEBUG1 2
/// DEBUG is reserved for things that are dev-oriented messages, like INFO but for devs not users.
#define LOG_LEVEL_DEBUG 3
/// INFO is for things that happen once, and are sub-milestone progress or are overly verbose
/// INFO marks the (inclusive) cutoff for verbosity that is parsible by a person
///when processing an average sized work load. Anything that would balloon
//output past human-readable in real-time for an average work load needs DEBUG1 - DEBUG3
#define LOG_LEVEL_INFO 4
/// NORMAL is for things that happen once or are milestone (that is, progress indicator) related.
#define LOG_LEVEL_NORMAL 5
/// WARN is for non-critical errors
#define LOG_LEVEL_WARN 6
/// CRITICAL is for errors that halt the normal operation of the program.
#define LOG_LEVEL_CRITICAL 7

#ifndef LOG_LEVEL
/// Define the default log level to be INFO if we haven't already defined it.
#define LOG_LEVEL LOG_LEVEL_INFO
#endif

#if LOG_LEVEL <= LOG_LEVEL_DEBUG
/// Define DEBUG if our log level is defined to be DEBUG or lower.
#define DEBUG
#endif

/// Define the logging message header that gets put into the message line.
/// @param[in] loglvl One of LOG_LEVEL_*
/// @return A short string of text indicating the logging level
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

/// Determine the descriptor (stderr or stdout) to log to based on the logging level.
/// @param[in] loglvl One of LOG_LEVEL_*
/// @return The file pointer to write the logging message to.
/// @retval stdout For all levels except warn, critical and unknown logging levels.
/// @retval stderr For warn, critical and unknown logging levels.
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

/// Log a message with the given log level to the given file descriptor.
/// Timestamps and log level header information is automatically added to the output.
/// Reference: http://stackoverflow.com/questions/679979/c-c-how-to-make-a-variadic-macro-variable-number-of-arguments
/// @param[in] loglvl One of LOG_LEVEL_*
/// @param[in] fp File descriptor to write to.
/// @param[in] ... Arguments including a format string and additional values as
///would be passed to fprintf
#define LOG_MESSAGE_F(loglvl, fp, ...) \
if (LOG_LEVEL <= loglvl) \
{ \
print_cur_time_stamp(LOG_LEVEL_MESSAGE_DESCRIPTOR(loglvl));\
fprintf(fp, LOG_LEVEL_MESSAGE_HEADER(loglvl)); \
fprintf(fp, ##__VA_ARGS__);\
}

/// The typical function used to log a message, chooses file descriptor automatically.
/// @param[in] loglvl One of LOG_LEVEL_*
/// @param[in] ... Arguments including a format string and additional values as
///would be passed to fprintf
#define LOG_MESSAGE(loglvl, ...) LOG_MESSAGE_F(loglvl, LOG_LEVEL_MESSAGE_DESCRIPTOR(loglvl), ##__VA_ARGS__)

/// Print the furrent time stamp out to the given file descriptor.
/// Reference: http://www.cplusplus.com/reference/clibrary/ctime/strftime/
/// @param[in] f File descriptor to write the timestamp to
static inline void print_cur_time_stamp(FILE* f)
{
    time_t rawtime;
    struct tm* timeinfo;
    // Assume that we'll never have more than 80 characters of timestamp data.
    /// That seems safe.
    char buffer[80];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, 80, "%b %d %H:%M:%S", timeinfo);
    fprintf(f, "%s ", buffer);
}

#endif
