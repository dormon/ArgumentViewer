#pragma once

#include <string>
#include <vector>

using namespace std;

class Format {
 public:
  string comment;
  Format(string const &com) : comment(com) {}
  enum MatchStatus {
    // match was successful
    MATCH_SUCCESS,
    // match was unsuccessful at the beginning
    MATCH_FAILURE,
    // error was encountered in the middle of matching
    MATCH_ERROR
  };
  virtual string      toStr(size_t indent      = 0,
                            size_t maxNameSize = 0,
                            size_t maxDataSize = 0,
                            size_t maxTypeSize = 0) const = 0;
  virtual MatchStatus match(vector<string> const &args,
                            size_t &              index) const          = 0;
};
