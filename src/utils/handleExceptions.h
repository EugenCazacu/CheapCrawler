#ifndef UTILS_HANDLEEXCEPTIONS_H_0BMFQ2X8
#define UTILS_HANDLEEXCEPTIONS_H_0BMFQ2X8

#include <stdexcept>

/**
 * !! Before including this header, initialize the logger component.
 * This error handler logs the error via the error logger.
 * @returns 0 if no exception is caught, 1 otherwise
 */
template<class Predicate>
int
handleExceptions(const Predicate& p) {
  try {
    p();
  }
  catch(const std::exception& exception) {
    LOG_ERROR("Error occured: " << exception.what());
    return 1;
  }
  catch(...) {
    LOG_ERROR("Unknown error occured.");
    return 1;
  }
  return 0;
}

template<class Predicate, typename RetType>
RetType
handleExceptionsReturnFuncValue(Predicate p, RetType errorRetValue) {
  try {
    return p();
  }
  catch(const std::exception& exception) {
    LOG_ERROR("Error occured: " << exception.what());
    return errorRetValue;
  }
  catch(...) {
    LOG_ERROR("Unknown error occured.");
    return errorRetValue;
  }
}

#endif /* end of include guard: UTILS_HANDLEEXCEPTIONS_H_0BMFQ2X8 */
