#pragma once

#include <ArgumentViewer/private/LineSplitter.h>
#include <ArgumentViewer/private/ValueFormat.h>

template <typename TYPE>
class VectorFormat : public ValueFormat {
 public:
  vector<TYPE> defaults;
  VectorFormat(string const &      argument,
               vector<TYPE> const &defs,
               string const &      com);
  virtual string      getData() const override;
  virtual size_t      getDataLength() const override;
  virtual string      getType() const override;
  virtual MatchStatus match(vector<string> const &args,
                            size_t &              index) const override;
};

template <typename TYPE>
VectorFormat<TYPE>::VectorFormat(string const &      argument,
                                 vector<TYPE> const &defs,
                                 string const &      com)
    : ValueFormat(argument, com), defaults(defs) {}

template <typename TYPE>
string VectorFormat<TYPE>::getData() const {
  stringstream ss;
  LineSplitter splitter;
  bool         first = true;
  for (auto const &x : defaults) {
    if (first)
      first = false;
    else
      splitter.addString(" ");
    splitter.addString(txtUtils::valueToString(x));
  }
  return splitter.get();
}

template <typename TYPE>
size_t VectorFormat<TYPE>::getDataLength() const {
  auto   text      = getData();
  size_t maxLength = 0;
  size_t lineStart = 0;
  size_t lineEnd;
  while ((lineEnd = text.find("\n", lineStart)) != string::npos) {
    maxLength = max(maxLength, lineEnd - lineStart + 1);
    lineStart = lineEnd + 1;
  }
  maxLength = max(maxLength, text.length() - lineStart + 1);
  return maxLength;
}

template <typename TYPE>
string VectorFormat<TYPE>::getType() const {
  return typeName<TYPE>() + "*";
}

template <typename TYPE>
Format::MatchStatus VectorFormat<TYPE>::match(vector<string> const &args,
                                      size_t &              index) const {
  if (index >= args.size()) return MATCH_FAILURE;
  if (args.at(index) != argumentName) return MATCH_FAILURE;
  ++index;
  while (index < args.size() && isValueConvertibleTo<TYPE>(args.at(index)))
    ++index;
  return MATCH_SUCCESS;
}
