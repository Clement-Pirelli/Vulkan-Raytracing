#include "Logger.h"
#include <iostream>
#include <cstdarg>

#ifdef NDEBUG
Logger::Verbosity Logger::verbosity = Logger::Verbosity::WARNING;
#else
Logger::Verbosity Logger::verbosity = Logger::Verbosity::TRIVIAL;
#endif

#ifdef _WIN64

#define COLOR_ERROR {std::cout << "\033[1;31m";}
#define COLOR_WARNING {std::cout << "\033[1;33m";}
#define COLOR_MESSAGE {std::cout << "\033[1;35m";}
#define COLOR_TRIVIAL {std::cout << "\033[1;33m";}
#define RESET_COLOR {std::cout << "\033[0m";}

#elif

#define COLOR_ERROR
#define COLOR_WARNING
#define COLOR_MESSAGE
#define COLOR_TRIVIAL
#define RESET_COLOR

#endif


#define CHECK_VERBOSITY(against) if(verbosity < against) return;



#define LOG(prefix, message) std::cout << "[" << prefix << "] " << message << '\n'; RESET_COLOR

#define LOG_FORMATTED(prefix, format)	\
std::cout << "[" << prefix << "] ";		\
va_list list;							\
va_start(list, format);					\
vprintf(format, list);					\
va_end(list);							\
std::cout << '\n';						\
RESET_COLOR


void Logger::setVerbosity(Verbosity givenVerbosity)
{
	Logger::verbosity = givenVerbosity;
}

void Logger::logMessage(const char *message)
{
	CHECK_VERBOSITY(Logger::Verbosity::MESSAGE);
	COLOR_MESSAGE LOG("message", message);
}

void Logger::logMessageFormatted(const char * const format, ...)
{
	CHECK_VERBOSITY(Logger::Verbosity::MESSAGE);
	COLOR_MESSAGE LOG_FORMATTED("message", format);
}

void Logger::logError(const char *error)
{
	COLOR_ERROR LOG("error!!!", error);
}

void Logger::logErrorFormatted(const char *format, ...)
{
	COLOR_ERROR LOG_FORMATTED("error!!!", format);
}
void Logger::logWarning(const char *message)
{
	CHECK_VERBOSITY(Logger::Verbosity::WARNING);

	COLOR_WARNING LOG("warning!", message);
}

void Logger::logWarningFormatted(const char *format, ...)
{
	CHECK_VERBOSITY(Logger::Verbosity::WARNING); 
	COLOR_WARNING LOG_FORMATTED("warning!", format);
}

void Logger::logTrivial(const char *message)
{
	CHECK_VERBOSITY(Logger::Verbosity::TRIVIAL);
	COLOR_TRIVIAL LOG("trivial", message);
}

void Logger::logTrivialFormatted(const char *format, ...)
{
	CHECK_VERBOSITY(Logger::Verbosity::TRIVIAL);
	COLOR_TRIVIAL LOG_FORMATTED("trivial", format);
}