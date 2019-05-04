#include "Logger.h"

#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <tuple>
#include <vector>

LOG_INIT(Logger);

class Logger {
public:
  static Logger& getInstance() {
    static Logger instance;
    return instance;
  }

  size_t add(const std::string& componentName) {
    m_components.push_back(std::make_tuple(componentName, false));
    return m_components.size() - 1;
  }

  std::ostream& getInfoStream(const size_t componentIndex) { return getOutputStream(); }

  std::ostream& getErrorStream(const size_t componentIndex) { return getOutputStream(); }

  std::ostream& getDebugStream(const size_t componentIndex) {
    if(componentIndex < m_components.size() && !std::get<1>(m_components[componentIndex])) {
      return m_nullSink;
    }
    else {
      return getOutputStream();
    }
  }

  std::mutex& getMutex() { return m_mutex; }

  void setOutput(const std::string& filename) {
    LOG_INFO("Setting log output to file: " << filename);
    std::ofstream newOutput(filename);
    if(newOutput.is_open() && newOutput.good()) {
      swap(m_fout, newOutput);
    }
    else {
      LOG_ERROR("Could not open file: " << filename);
    }
  }

  void enable(const std::string& componentName, const bool enabled) {
    if(componentName.empty()) {
      for(auto& component: m_components) {
        std::get<1>(component) = enabled;
      }
    }
    else {
      auto componentPos = std::find_if(
          m_components.begin(), m_components.end(), [&componentName](const std::tuple<std::string, bool>& elem) {
            return componentName == std::get<0>(elem);
          });
      if(componentPos == m_components.end()) {
        throw std::runtime_error("[Logger] the following component does not exist: " + componentName);
      }
      std::replace(componentPos,
                   m_components.end(),
                   std::make_tuple(componentName, !enabled),
                   std::make_tuple(componentName, enabled));
    }
  }

  const std::vector<std::tuple<std::string, bool>>& getLoggedComponents() { return m_components; }

private:
  Logger() : m_components(), m_fout(), m_nullSink(nullptr) {}

  std::ostream& getOutputStream() {
    if(m_fout.is_open() && m_fout.good()) {
      return m_fout;
    }
    else {
      return std::cout;
    }
  }

  std::vector<std::tuple<std::string, bool>> m_components;
  std::ofstream                              m_fout;
  std::ostream                               m_nullSink;
  std::mutex                                 m_mutex;
};

LoggerComponent::LoggerComponent(const std::string componentName)
    : m_name(componentName), m_index(Logger::getInstance().add(m_name)) {}

std::ostream&
LoggerComponent::getInfoStream() {
  return Logger::getInstance().getInfoStream(m_index);
}

std::ostream&
LoggerComponent::getErrorStream() {
  return Logger::getInstance().getErrorStream(m_index);
}

std::ostream&
LoggerComponent::getDebugStream() {
  return Logger::getInstance().getDebugStream(m_index);
}

std::mutex&
LoggerComponent::getMutex() {
  return Logger::getInstance().getMutex();
}

void
setLoggingOutput(const std::string& filename) {
  Logger::getInstance().setOutput(filename);
}

void
enableDebugLogging(const std::string& componentName, bool enabled) {
  Logger::getInstance().enable(componentName, enabled);
}

const std::vector<std::tuple<std::string, bool>>&
getLoggedComponents() {
  return Logger::getInstance().getLoggedComponents();
}

void
configureLogging(boost::program_options::options_description& optionsDescription) {
  optionsDescription.add_options()
#ifndef NDEBUG
      ("debug", boost::program_options::value<std::vector<std::string>>(), "Sets which components are being debuged")(
          "debugAll", "Debug all components")
#endif
          ("logOutput", boost::program_options::value<std::string>(), "Redirect logging output to this file")(
              "listLoggedComponents,l", "Print all logged components");
}

bool
processLogging(const boost::program_options::variables_map& variablesMap) {
  if(variablesMap.count("listLoggedComponents")) {
    for(auto& component: getLoggedComponents()) {
      std::cout << std::get<0>(component) << std::endl;
    }
    return false;
  }

#ifndef NDEBUG
  if(variablesMap.count("debugAll")) {
    enableDebugLogging();
  }
  else if(variablesMap.count("debug")) {
    const auto& debuggedComponents = variablesMap["debug"].as<std::vector<std::string>>();
    for(auto& componentName: debuggedComponents) {
      enableDebugLogging(componentName);
    }
  }
#endif

  if(variablesMap.count("logOutput")) {
    setLoggingOutput(variablesMap["logOutput"].as<std::string>());
  }

  return true;
}
