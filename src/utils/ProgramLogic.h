#ifndef h_programLogic
#define h_programLogic

#include <functional>

template<typename TProgramOptions> class ProgramLogic {
public:
  ProgramLogic(int                                               argc,
               const char**                                      argv,
               std::function<TProgramOptions(int, const char**)> cmdLineOptionsReader,
               std::function<void(TProgramOptions)>              programLogic)
      : m_argc(argc), m_argv(argv), m_readCommandLineArgs(cmdLineOptionsReader), m_programLogic(programLogic) {}

  void operator()() const {
    auto programOptions = m_readCommandLineArgs(m_argc, m_argv);
    if(programOptions.shouldContinue) {
      m_programLogic(programOptions);
    }
  }

private:
  int                                               m_argc;
  const char**                                      m_argv;
  std::function<TProgramOptions(int, const char**)> m_readCommandLineArgs;
  std::function<void(TProgramOptions)>              m_programLogic;
};

#endif // h_programLogic
