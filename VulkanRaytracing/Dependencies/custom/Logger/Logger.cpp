#include "Logger.h"
#include <iostream>
#include <cstdarg>

#ifdef NDEBUG
Logger::Verbosity Logger::verbosity = Logger::Verbosity::WARNING;
#else
Logger::Verbosity Logger::verbosity = Logger::Verbosity::TRIVIAL;
#endif

#define CHECK_VERBOSITY(against) if(verbosity < against) return;

#define LOG(prefix, message) std::cout << "[" << prefix << "] " << message << '\n';

#define LOG_FORMATTED(prefix, format)	\
std::cout << "[" << prefix << "] ";		\
va_list list;							\
va_start(list, format);					\
vprintf(format, list);					\
va_end(list);							\
std::cout << '\n';


void Logger::setVerbosity(Verbosity givenVerbosity)
{
	Logger::verbosity = givenVerbosity;
}

void Logger::logMessage(const char *message)
{
	CHECK_VERBOSITY(Logger::Verbosity::MESSAGE);
	LOG("message", message);
}

void Logger::logMessageFormatted(const char * const format, ...)
{
	CHECK_VERBOSITY(Logger::Verbosity::MESSAGE);
	LOG_FORMATTED("message", format);
}

void Logger::logError(const char *error)
{
	LOG("error!!!", error);
}

void Logger::logErrorFormatted(const char *format, ...)
{
	LOG_FORMATTED("error!!!", format);
}
void Logger::logWarning(const char *message)
{
	CHECK_VERBOSITY(Logger::Verbosity::WARNING);
	LOG("warning!", message);
}

void Logger::logWarningFormatted(const char *format, ...)
{
	CHECK_VERBOSITY(Logger::Verbosity::WARNING); 
	LOG_FORMATTED("warning!", format);
}

void Logger::logTrivial(const char *message)
{
	CHECK_VERBOSITY(Logger::Verbosity::TRIVIAL);
	LOG("trivial", message);
}

void Logger::logTrivialFormatted(const char *format, ...)
{
	CHECK_VERBOSITY(Logger::Verbosity::TRIVIAL);
	LOG_FORMATTED("trivial", format);
}