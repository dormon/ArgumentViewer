#pragma once

#include <ArgumentViewer/private/Format.h>
#include <sstream>

class ValueFormat : public Format {
 public:
  string argumentName;
  ValueFormat(string const &argument, string const &com);
  string              getName() const;
  virtual string      getData() const = 0;
  virtual size_t      getDataLength() const;
  virtual string      getType() const                     = 0;
  virtual MatchStatus match(vector<string> const &args,
                            size_t &              index) const override = 0;
  virtual string      toStr(size_t indent,
                            size_t maxNameSize,
                            size_t maxDataSize,
                            size_t maxTypeSize) const override final;
};
