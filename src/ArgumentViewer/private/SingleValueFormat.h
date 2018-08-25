#pragma once

#include <ArgumentViewer/private/ValueFormat.h>
#include <ArgumentViewer/private/CommonFunctions.h>

template <typename TYPE>
class SingleValueFormat : public ValueFormat {
 public:
  SingleValueFormat(string const &argument, TYPE const &def, string const &com);
  virtual string      getDefaults() const override;
  virtual string      getType() const override;
  virtual MatchStatus match(vector<string> const &args,
                            size_t &              index) const override;
  TYPE                defaults;
};

template <typename TYPE>
SingleValueFormat<TYPE>::SingleValueFormat(string const &argument,
                                           TYPE const &  def,
                                           string const &com)
    : ValueFormat(argument, com), defaults(def) {}

template <typename TYPE>
string SingleValueFormat<TYPE>::getDefaults() const {
  if (is_same<TYPE, string>::value) {
    auto x = txtUtils::valueToString(defaults);
    return chopQuotes(x);
  }
  return txtUtils::valueToString(defaults);
}

template <typename TYPE>
string SingleValueFormat<TYPE>::getType() const {
  return typeName<TYPE>();
}

template <typename TYPE>
Format::MatchStatus SingleValueFormat<TYPE>::match(vector<string> const &args,
                                                   size_t &index) const {
  size_t oldIndex = index;
  if (index >= args.size()) return MATCH_FAILURE;
  if (args.at(index) != argumentName) return MATCH_FAILURE;
  ++index;
  if (index >= args.size()) {
    index = oldIndex;
    return MATCH_ERROR;
  }
  if (!isValueConvertibleTo<TYPE>(args.at(index))) {
    index = oldIndex;
    return MATCH_ERROR;
  }
  ++index;
  return MATCH_SUCCESS;
}
