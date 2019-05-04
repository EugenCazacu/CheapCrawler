#ifndef _UTILS_LOOGER_H
#define _UTILS_LOOGER_H

// Specs:
// - list all registered components
// - on runtime only show specificly configured debug logs, default none
// - print all debug messages option
// - release build only compile with info messages
// - send all logs to a specified log file
// - debug messages have 0 overhead in release

#include <boost/program_options.hpp>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

class LoggerComponent {
public:
  LoggerComponent(const std::string componentName);
  std::ostream&      getInfoStream();
  std::ostream&      getErrorStream();
  std::ostream&      getDebugStream();
  const std::string& getName() const { return m_name; }
  std::mutex&        getMutex();

private:
  std::string m_name;
  size_t      m_index;
};

#define LOG_INIT(COMPONENT_NAME) static LoggerComponent componentLogger(#COMPONENT_NAME);

// TODO
// The temporary ostringstream is required to avoid any deadlocks caused by
// the evaluation of the mesg inside the componentLogger mutex lock
// Using stringstream is not efficient and a redesign should be performed
// Possible solutions:
// - use the Abseil library to concat strings, switch to a function based
//   interface and use C++17 if constexpr feature to achieve 0 runtine overhead
// - switch to a different logging library altogether, something like https://github.com/gabime/spdlog
//   but try to keep the existing features.
#define LOG_INFO(mesg)                                                             \
  {                                                                                \
    std::ostringstream output;                                                     \
    output << "[INFO] " << componentLogger.getName() << ": " << mesg << std::endl; \
    std::lock_guard<std::mutex> lck(componentLogger.getMutex());                   \
    componentLogger.getInfoStream() << output.str();                               \
  }

#define LOG_ERROR(mesg)                                                             \
  {                                                                                 \
    std::ostringstream output;                                                      \
    output << "[ERROR] " << componentLogger.getName() << ": " << mesg << std::endl; \
    std::lock_guard<std::mutex> lck(componentLogger.getMutex());                    \
    componentLogger.getErrorStream() << output.str();                               \
  }

#ifdef NDEBUG
#define LOG_DEBUG(mesg)

#define LOG_COND_INFO_DEBUG(cond, mesg) \
  if(cond) {                            \
    LOG_INFO(mesg);                     \
  }

#define LOG_COND_ERROR_DEBUG(cond, mesg) \
  if(cond) {                             \
    LOG_ERROR(mesg);                     \
  }

#else

#define LOG_DEBUG(mesg)                                                             \
  {                                                                                 \
    std::ostringstream output;                                                      \
    output << "[DEBUG] " << componentLogger.getName() << ": " << mesg << std::endl; \
    std::lock_guard<std::mutex> lck(componentLogger.getMutex());                    \
    componentLogger.getDebugStream() << output.str();                               \
  }

#define LOG_COND_INFO_DEBUG(cond, mesg) \
  if(cond) {                            \
    LOG_INFO(mesg);                     \
  }                                     \
  else {                                \
    LOG_DEBUG(mesg);                    \
  }

#define LOG_COND_ERROR_DEBUG(cond, mesg) \
  if(cond) {                             \
    LOG_ERROR(mesg);                     \
  }                                      \
  else {                                 \
    LOG_DEBUG(mesg);                     \
  }

#endif

#ifdef NDEBUG

#else

#endif

/**
 * Set log output to <filename>
 */
void setLoggingOutput(const std::string& filename);

/**
 * Enable/disable debugging for specific component
 * Empty component name means debug all components
 */
void enableDebugLogging(const std::string& componentName = "", bool enabled = true);

/**
 * Returns all logged components with the associated enabled/disabled log status for debug messages.
 * Info and error messages are always enabled
 */
const std::vector<std::tuple<std::string, bool>>& getLoggedComponents();

/**
 * Configures the program options with the logging
 * Will add the following options on all build configurations:
 *  ("logOutput", boost::program_options::value<std::string>(), "Redirect logging output to this file")
 *  ("listLoggedComponents,l", "Print all logged components")
 * The following are also added when NDEBUG is not defined, i.e. release build
 *  ("debug", boost::program_options::value<std::vector<std::string>>(), "Sets which components are being debuged")
 *  ("debugAll", "Debug all components")
 */
void configureLogging(boost::program_options::options_description& optionsDescription);

/**
 * Processes the variablesMap in regard to the above configured logging options
 * and configures the logging accordingly.
 * Returns true when the program should continue to run and false otherwise.
 */
bool processLogging(const boost::program_options::variables_map& variablesMap);

inline std::ostream&
operator<<(std::ostream& out, const std::tuple<std::string, int>& in) {
  out << '[' << std::get<0>(in) << ',' << std::get<1>(in) << ']';
  return out;
}

template<typename T>
inline std::ostream&
operator<<(std::ostream& out, const std::vector<T>& input) {
  for(auto& elem: input) {
    out << elem << std::endl;
  }
  return out;
}

template<typename T>
inline std::ostream&
operator<<(std::ostream& out, const std::vector<std::vector<T>>& input) {
  for(auto& row: input) {
    out << row.size() << ": ";
    for(auto& elem: row) {
      out << elem << ' ';
    }
    out << std::endl;
  }
  return out;
}

#endif // _UTILS_LOOGER_H
