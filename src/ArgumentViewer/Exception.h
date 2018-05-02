#pragma once

#include <ArgumentViewer/Fwd.h>
#include <stdexcept>
#include <string>

class argumentViewer::ex::Exception : public std::runtime_error {
 public:
  Exception(std::string const& msg) : std::runtime_error(msg) {}
  virtual ~Exception() throw() {}
  virtual char const* what() const throw() override {
    return std::string(std::string("ArgumentViewer - ") +
                       std::runtime_error::what())
        .c_str();
  }
};

class argumentViewer::ex::MatchError : public Exception {
 public:
  MatchError(std::string const& msg) : Exception(msg) {}
  virtual char const* what() const throw() override {
    return std::string(std::string("ArgumentViewer::match - ") +
                       std::runtime_error::what())
        .c_str();
  }
};
