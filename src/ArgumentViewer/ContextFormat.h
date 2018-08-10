#pragma once

#include <ArgumentViewer/ArgumentListFormat.h>
#include <ArgumentViewer/Globals.h>

class ContextFormat : public ArgumentListFormat {
 public:
  std::string argumentName;
  ContextFormat(std::string const &argument, std::string const &com);

  virtual std::string toStr(size_t indent,
                            size_t = 0,
                            size_t = 0,
                            size_t = 0) const override;
  virtual MatchStatus match(std::vector<std::string> const &args,
                            size_t &index) const override;
};
