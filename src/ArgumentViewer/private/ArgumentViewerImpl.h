#pragma once

#include <memory>
#include <cassert>
#include <set>
#include <sstream>
#include <TxtUtils/TxtUtils.h>
#include <ArgumentViewer/Fwd.h>
#include <ArgumentViewer/Exception.h>
#include <ArgumentViewer/private/Format.h>
#include <ArgumentViewer/private/CommonFunctions.h>
#include <ArgumentViewer/private/ArgumentListFormat.h>
#include <ArgumentViewer/private/SingleValueFormat.h>
#include <ArgumentViewer/private/VectorFormat.h>

using namespace argumentViewer;

class argumentViewer::ArgumentViewerImpl {
 public:
  std::string              applicationName = "";
  std::vector<std::string> arguments;
  ArgumentViewer const *   parent = nullptr;
  std::shared_ptr<Format>  format = nullptr;
  size_t getArgumentPosition(std::string const &argument) const;
  bool   getContext(std::vector<std::string> &contextArguments,
                    std::string const &       argument) const;
  bool   isInRange(size_t index) const {
    assert(this != nullptr);
    return index < arguments.size();
  }
  std::string getArgument(size_t index) const {
    assert(this != nullptr);
    assert(index < arguments.size());
    return arguments.at(index);
  }
  template <typename TYPE,
            typename std::enable_if<std::is_same<TYPE, int32_t>::value ||
                                        std::is_same<TYPE, int64_t>::value ||
                                        std::is_same<TYPE, uint32_t>::value ||
                                        std::is_same<TYPE, uint64_t>::value ||
                                        std::is_same<TYPE, float>::value ||
                                        std::is_same<TYPE, double>::value,
                                    unsigned>::type = 0>
  TYPE str2val(std::string const &value) const {
    return txtUtils::str2Value<TYPE>(value);
  }
  template <typename TYPE,
            typename std::enable_if<std::is_same<TYPE, std::string>::value,
                                    unsigned>::type = 0>
  TYPE str2val(std::string const &value) const {
    return value;
  }
  template <typename TYPE>
  TYPE getArgument(std::string const &argument, TYPE const &def) const {
    size_t i = getArgumentPosition(argument);
    if (!isInRange(i++)) return def;
    if (!isInRange(i)) return def;
    auto value = getArgument(i);
    if (!isValueConvertibleTo<TYPE>(value)) return def;
    return str2val<TYPE>(value);
  }
  template <typename TYPE>
  std::vector<TYPE> getArguments(std::string const &      argument,
                                 std::vector<TYPE> const &def) const {
    size_t argumentIndex = getArgumentPosition(argument);
    if (!isInRange(argumentIndex++)) return def;
    if (!isInRange(argumentIndex)) return def;
    std::vector<TYPE> result;
    while (isInRange(argumentIndex) &&
           isValueConvertibleTo<TYPE>(getArgument(argumentIndex)))
      result.push_back(txtUtils::str2Value<TYPE>(getArgument(argumentIndex++)));
    while (result.size() < def.size()) result.push_back(def.at(result.size()));
    return result;
  }
  void loadArgumentFiles(std::vector<std::string> &args,
                         std::set<std::string> &   alreadyLoaded);
  void splitFileToArguments(std::vector<std::string> &args,
                            std::string const &       fileContent);
  template <typename TYPE>
  TYPE getArgumentWithFormat(std::string const &argument,
                             TYPE const &       def,
                             std::string const &com) const;
  template <typename TYPE>
  std::vector<TYPE> getArgumentsWithFormat(std::string const &      argument,
                                           std::vector<TYPE> const &def,
                                           std::string const &      com) const;
};

template <typename TYPE>
TYPE ArgumentViewerImpl::getArgumentWithFormat(std::string const &argument,
                                               TYPE const &       def,
                                               std::string const &com) const {
  auto alf = std::dynamic_pointer_cast<ArgumentListFormat>(format);
  assert(alf != nullptr);

  auto subFormatIt = alf->formats.find(argument);
  if (subFormatIt != alf->formats.end()) {
    auto subFormat = subFormatIt->second;
    auto singleValueFormat =
        std::dynamic_pointer_cast<SingleValueFormat<TYPE>>(subFormat);
    if (!singleValueFormat)
      throw ex::Exception(std::string("argument: ") + argument +
                          " is already defined as something else than single " +
                          typeName<TYPE>() + " value");
    if (singleValueFormat->defaults != def)
      throw ex::Exception(
          std::string("argument: ") + argument +
          " has already been defined with different default value: "+txtUtils::valueToString(singleValueFormat->defaults));
  } else
    alf->formats[argument] =
        std::make_shared<SingleValueFormat<TYPE>>(argument, def, com);
  if (alf->formats[argument]->comment == "")
    alf->formats[argument]->comment = com;

  return getArgument<TYPE>(argument, def);
}

template <typename TYPE>
std::vector<TYPE> ArgumentViewerImpl::getArgumentsWithFormat(
    std::string const &      argument,
    std::vector<TYPE> const &def,
    std::string const &      com) const {
  auto alf = std::dynamic_pointer_cast<ArgumentListFormat>(format);
  assert(alf != nullptr);

  auto subFormatIt = alf->formats.find(argument);
  if (subFormatIt != alf->formats.end()) {
    auto subFormat = subFormatIt->second;
    auto vectorFormat =
        std::dynamic_pointer_cast<VectorFormat<TYPE>>(subFormat);
    if (!vectorFormat)
      throw ex::Exception(
          std::string("argument: ") + argument +
          " is already defined as something else than vector of " +
          typeName<TYPE>() + " values");
    if (vectorFormat->defaults != def)
      throw ex::Exception(
          std::string("argument: ") + argument +
          " has already been defined with different default values: " +
          txtUtils::valueToString(vectorFormat->defaults));
  } else
    alf->formats[argument] =
        std::make_shared<VectorFormat<TYPE>>(argument, def, com);
  if (alf->formats[argument]->comment == "")
    alf->formats[argument]->comment = com;

  return getArguments<TYPE>(argument, def);
}

