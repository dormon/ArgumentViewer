#pragma once

#include <ArgumentViewer/ValueFormat.h>
#include <map>
#include <memory>

class ContextFormat;
class ArgumentListFormat : public Format {
 public:
  ArgumentListFormat(string const &com);
  virtual string                  toStr(size_t indent,
                                        size_t = 0,
                                        size_t = 0,
                                        size_t = 0) const override;
  virtual MatchStatus             match(vector<string> const &args,
                                        size_t &              index) const override;
  map<string, shared_ptr<Format>> formats;
};
