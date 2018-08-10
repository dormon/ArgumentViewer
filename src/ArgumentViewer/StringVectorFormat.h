#pragma once

#include <ArgumentViewer/ValueFormat.h>

class StringVectorFormat : public ValueFormat {
 public:
  vector<string> defaults;
  StringVectorFormat(string const &        argument,
                     vector<string> const &defs,
                     string const &        com);
  virtual string      getData() const override;
  virtual size_t      getDataLength() const override;
  virtual string      getType() const override;
  virtual MatchStatus match(vector<string> const &args,
                            size_t &              index) const override;
};
