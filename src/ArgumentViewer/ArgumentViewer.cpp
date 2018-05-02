#include <ArgumentViewer/ArgumentViewer.h>
#include <ArgumentViewer/Exception.h>
#include <MealyMachine/MealyMachine.h>
#include <TxtUtils/TxtUtils.h>

#include <algorithm>
#include <cassert>
#include <map>
#include <set>
#include <sstream>

using namespace argumentViewer;

class Format {
 public:
  std::string comment;
  Format(std::string const &com) : comment(com) {}
  enum MatchStatus {
    // match was successful
    MATCH_SUCCESS,
    // match was unsuccessful at the beginning
    MATCH_FAILURE,
    // error was encountered in the middle of matching
    MATCH_ERROR
  };
  virtual std::string toStr(size_t indent      = 0,
                            size_t maxNameSize = 0,
                            size_t maxDataSize = 0,
                            size_t maxTypeSize = 0) const = 0;
  virtual MatchStatus match(std::vector<std::string> const &args,
                            size_t &                        index) const          = 0;
};

class argumentViewer::ArgumentViewerImpl {
 public:
  std::string              applicationName = "";
  std::vector<std::string> arguments;
  static std::string const contextBegin;
  static std::string const contextEnd;
  static std::string const fileSymbol;
  static size_t const      levelIndent;
  static size_t const      maxDataLineLength;
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
  static std::string parseEscapeSequence(std::string const &text) {
    if (text == "\\" + contextBegin) return contextBegin;
    if (text == "\\" + contextEnd) return contextEnd;
    if (text == "\\" + fileSymbol) return fileSymbol;
    return text;
  }
  static std::string alignString(std::string const &text,
                                 size_t             alignment  = 0,
                                 char               filler     = ' ',
                                 bool               fillerLeft = false) {
    std::stringstream ss;
    if (fillerLeft)
      for (size_t i = text.length(); i < alignment; ++i) ss << filler;
    ss << text;
    if (!fillerLeft)
      for (size_t i = text.length(); i < alignment; ++i) ss << filler;
    return ss.str();
  }
  template <typename TO, typename FROM>
  static bool isTypeOf(std::shared_ptr<FROM> const &value) {
    return std::dynamic_pointer_cast<TO>(value) != nullptr;
  }
  template <typename TYPE>
  static std::string typeName();
  template <typename TYPE>
  static bool isValueConvertibleTo(std::string const &text);
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

size_t ArgumentViewerImpl::getArgumentPosition(
    std::string const &argument) const {
  size_t       argumentIndex  = 0;
  size_t       contextCounter = 0;
  size_t const notFound       = arguments.size();
  for (auto x : arguments) {
    if (x == argument && contextCounter == 0) return argumentIndex;
    if (x == contextBegin) ++contextCounter;
    if (x == contextEnd) {
      if (contextCounter == 0) return notFound;
      --contextCounter;
    }
    ++argumentIndex;
  }
  return notFound;
}

bool ArgumentViewerImpl::getContext(std::vector<std::string> &contextArguments,
                                    std::string const &       argument) const {
  size_t argumentIndex = getArgumentPosition(argument);
  if (!isInRange(argumentIndex++)) return false;
  if (!isInRange(argumentIndex)) return false;
  if (getArgument(argumentIndex++) != contextBegin) return false;
  size_t contextCounter = 0;
  while (isInRange(argumentIndex)) {
    if (getArgument(argumentIndex) == ArgumentViewerImpl::contextEnd) {
      if (contextCounter == 0)
        return true;
      else
        contextCounter--;
    }
    if (getArgument(argumentIndex) == ArgumentViewerImpl::contextBegin)
      contextCounter++;
    contextArguments.push_back(getArgument(argumentIndex));
    ++argumentIndex;
  }
  contextArguments.clear();
  return false;
}

void ArgumentViewerImpl::loadArgumentFiles(
    std::vector<std::string> &args, std::set<std::string> &alreadyLoaded) {
  size_t argumentIndex = 0;
  while (argumentIndex < args.size()) {
    if (args.at(argumentIndex) != fileSymbol) {
      ++argumentIndex;
      continue;
    }
    if (argumentIndex + 1 >= args.size())
      throw ex::Exception(std::string("expected filename after ") + fileSymbol +
                          " not end of arguments/file");
    auto fileName = args.at(argumentIndex + 1);
    if (alreadyLoaded.count(fileName))
      throw ex::Exception(std::string("file: ") + fileName +
                          " contains file loading loop");
    std::vector<std::string> newArgs;
    std::string              fileContent = txtUtils::loadTextFile(fileName);
    splitFileToArguments(newArgs, fileContent);
    alreadyLoaded.insert(fileName);
    loadArgumentFiles(newArgs, alreadyLoaded);
    alreadyLoaded.erase(fileName);
    std::vector<std::string> withLoadedFile;
    for (size_t j = 0; j < argumentIndex; ++j)
      withLoadedFile.push_back(args.at(j));
    for (auto const &x : newArgs) withLoadedFile.push_back(x);
    for (size_t j = argumentIndex + 2; j < args.size(); ++j)
      withLoadedFile.push_back(args.at(j));
    args = withLoadedFile;
    argumentIndex--;
    argumentIndex += newArgs.size();
  }
}

void ArgumentViewerImpl::splitFileToArguments(std::vector<std::string> &args,
                                              std::string const &fileContent) {
  mealyMachine::MealyMachine mm;
  auto                       start        = mm.addState();
  auto                       space        = mm.addState();
  auto                       word         = mm.addState();
  auto                       comment      = mm.addState();
  auto                       startNewWord = [&](mealyMachine::MealyMachine *) {
    args.push_back("");
    args.back() += *(char *)mm.getCurrentSymbol();
  };
  auto addCharToWord = [&](mealyMachine::MealyMachine *) {
    args.back() += *(char *)mm.getCurrentSymbol();
  };
  mm.addTransition(start, " \t\n\r", space);
  mm.addTransition(start, "#", comment);
  mm.addElseTransition(start, word, startNewWord);
  mm.addEOFTransition(start);

  mm.addTransition(space, " \t\n\r", space);
  mm.addTransition(space, "#", comment);
  mm.addElseTransition(space, word, startNewWord);
  mm.addEOFTransition(space);

  mm.addTransition(comment, "\n\r", start);
  mm.addElseTransition(comment, comment);
  mm.addEOFTransition(comment);

  mm.addTransition(word, " \t\n\r", space);
  mm.addTransition(word, "#", comment);
  mm.addElseTransition(word, word, addCharToWord);
  mm.addEOFTransition(word);

  mm.match(fileContent.c_str());
}

class ValueFormat : public Format {
 public:
  std::string argumentName;
  ValueFormat(std::string const &argument, std::string const &com)
      : Format(com), argumentName(argument) {}
  std::string         getName() const { return argumentName; }
  virtual std::string getData() const = 0;
  virtual size_t      getDataLength() const { return getData().length(); }
  virtual std::string getType() const                     = 0;
  virtual MatchStatus match(std::vector<std::string> const &args,
                            size_t &index) const override = 0;
  virtual std::string toStr(size_t indent,
                            size_t maxNameSize,
                            size_t maxDataSize,
                            size_t maxTypeSize) const override final {
    std::stringstream ss;
    for (size_t i = 0; i < indent; ++i) ss << " ";
    ss << ArgumentViewerImpl::alignString(getName(), maxNameSize);
    ss << " = ";
    size_t indentation = ss.str().length();
    auto   data        = getData();
    bool   firstLine   = true;
    size_t lineEnd     = 0;
    size_t lineStart   = 0;
    while ((lineEnd = data.find("\n", lineStart)) != std::string::npos) {
      if (firstLine)
        firstLine = false;
      else
        for (size_t i = 0; i < indentation; ++i) ss << " ";
      ss << data.substr(lineStart, lineEnd - lineStart + 1);
      lineStart = lineEnd + 1;
    }
    if (!firstLine)
      for (size_t i = 0; i < indentation; ++i) ss << " ";
    ss << ArgumentViewerImpl::alignString(
        data.substr(lineStart, data.length() - lineStart + 1), maxDataSize);

    // ss<<ArgumentViewerImpl::alignString(getData(),maxDataSize);
    ss << " [" << ArgumentViewerImpl::alignString(getType(), maxTypeSize);
    ss << "]";
    if (comment != "") ss << " - " << comment;
    ss << std::endl;
    return ss.str();
  }
};

template <typename TYPE>
class SingleValueFormat : public ValueFormat {
 public:
  TYPE defaults;
  SingleValueFormat(std::string const &argument,
                    TYPE const &       def,
                    std::string const &com)
      : ValueFormat(argument, com), defaults(def) {}
  virtual std::string getData() const override {
    if (std::is_same<TYPE, std::string>::value) {
      auto x = txtUtils::valueToString(defaults);
      x      = x.substr(1);
      x      = x.substr(0, x.length() - 1);
      return x;
    }
    return txtUtils::valueToString(defaults);
  }
  virtual std::string getType() const override {
    return ArgumentViewerImpl::typeName<TYPE>();
  }
  virtual MatchStatus match(std::vector<std::string> const &args,
                            size_t &index) const override {
    size_t oldIndex = index;
    if (index >= args.size()) return MATCH_FAILURE;
    if (args.at(index) != argumentName) return MATCH_FAILURE;
    ++index;
    if (index >= args.size()) {
      index = oldIndex;
      return MATCH_ERROR;
    }
    if (!ArgumentViewerImpl::isValueConvertibleTo<TYPE>(args.at(index))) {
      index = oldIndex;
      return MATCH_ERROR;
    }
    ++index;
    return MATCH_SUCCESS;
  }
};

class LineSplitter {
 public:
  std::string get() const { return ss.str(); }
  void        addString(std::string const &text) {
    if (ss.str().length() - lineStart + 1 + text.length() >=
        ArgumentViewerImpl::maxDataLineLength) {
      ss << std::endl;
      lineStart = ss.str().length();
      if (text == " ") return;
    }
    ss << text;
  }

 protected:
  size_t            lineStart = 0;
  std::stringstream ss;
};

template <typename TYPE>
class VectorFormat : public ValueFormat {
 public:
  std::vector<TYPE> defaults;
  VectorFormat(std::string const &      argument,
               std::vector<TYPE> const &defs,
               std::string const &      com)
      : ValueFormat(argument, com), defaults(defs) {}
  virtual std::string getData() const override {
    std::stringstream ss;
    LineSplitter      splitter;
    bool              first = true;
    for (auto const &x : defaults) {
      if (first)
        first = false;
      else
        splitter.addString(" ");
      splitter.addString(txtUtils::valueToString(x));
    }
    return splitter.get();
  }
  virtual size_t getDataLength() const override {
    auto   text      = getData();
    size_t maxLength = 0;
    size_t lineStart = 0;
    size_t lineEnd;
    while ((lineEnd = text.find("\n", lineStart)) != std::string::npos) {
      maxLength = std::max(maxLength, lineEnd - lineStart + 1);
      lineStart = lineEnd + 1;
    }
    maxLength = std::max(maxLength, text.length() - lineStart + 1);
    return maxLength;
  }
  virtual std::string getType() const override {
    return ArgumentViewerImpl::typeName<TYPE>() + "*";
  }
  virtual MatchStatus match(std::vector<std::string> const &args,
                            size_t &index) const override {
    if (index >= args.size()) return MATCH_FAILURE;
    if (args.at(index) != argumentName) return MATCH_FAILURE;
    ++index;
    while (index < args.size() &&
           ArgumentViewerImpl::isValueConvertibleTo<TYPE>(args.at(index)))
      ++index;
    return MATCH_SUCCESS;
  }
};

class StringVectorFormat : public ValueFormat {
 public:
  std::vector<std::string> defaults;
  StringVectorFormat(std::string const &             argument,
                     std::vector<std::string> const &defs,
                     std::string const &             com)
      : ValueFormat(argument, com), defaults(defs) {}
  virtual std::string getData() const override {
    LineSplitter splitter;
    splitter.addString(ArgumentViewerImpl::contextBegin);
    bool first = true;
    for (auto const &x : defaults) {
      if (first)
        first = false;
      else
        splitter.addString(" ");
      splitter.addString(x);
    }
    splitter.addString(ArgumentViewerImpl::contextEnd);
    return splitter.get();
  }
  virtual size_t getDataLength() const override {
    auto   data          = getData();
    size_t maxLineLength = 0;
    size_t lineStart     = 0;
    size_t lineEnd;
    while ((lineEnd = data.find("\n", lineStart)) != std::string::npos) {
      maxLineLength = std::max(maxLineLength, lineEnd - lineStart + 1);
      lineStart     = lineEnd + 1;
    }
    maxLineLength = std::max(maxLineLength, data.length() - lineStart + 1);
    return maxLineLength;
  }
  virtual std::string getType() const override { return "string*"; }
  virtual MatchStatus match(std::vector<std::string> const &args,
                            size_t &index) const override {
    if (index >= args.size()) return MATCH_FAILURE;
    size_t oldIndex = index;
    if (args.at(index) != argumentName) return MATCH_FAILURE;
    ++index;
    if (args.at(index) != ArgumentViewerImpl::contextBegin) {
      index = oldIndex;
      std::stringstream ss;
      ss << "Argument error:" << std::endl;
      ss << "expected " << ArgumentViewerImpl::contextBegin
         << " after argument: " << argumentName;
      ss << " not argument: " << args.at(index);
      throw ex::MatchError(ss.str());
      return MATCH_ERROR;
    }
    ++index;
    while (index < args.size() &&
           args.at(index) != ArgumentViewerImpl::contextEnd)
      ++index;
    if (index >= args.size()) {
      index = oldIndex;
      std::stringstream ss;
      ss << "Argument error:" << std::endl;
      ss << "expected " << ArgumentViewerImpl::contextEnd
         << " at the end of context argument: " << argumentName;
      ss << " not end of arguments";
      throw ex::MatchError(ss.str());
      return MATCH_ERROR;
    }
    ++index;
    return MATCH_SUCCESS;
  }
};

class IsPresentFormat : public Format {
 public:
  std::string argumentName;
  IsPresentFormat(std::string const &name, std::string const &com)
      : Format(com), argumentName(name) {}
  virtual std::string toStr(size_t indent,
                            size_t maxNameSize,
                            size_t maxDataSize,
                            size_t maxTypeSize) const override final {
    std::stringstream ss;
    for (size_t i = 0; i < indent; ++i) ss << " ";
    ss << ArgumentViewerImpl::alignString(argumentName, maxNameSize);
    ss << "   ";
    ss << ArgumentViewerImpl::alignString("", maxDataSize);
    ss << "  " << ArgumentViewerImpl::alignString("", maxTypeSize);
    ss << " ";
    if (comment != "") ss << " - " << comment << std::endl;
    return ss.str();
  }
  virtual MatchStatus match(std::vector<std::string> const &args,
                            size_t &index) const override {
    if (index >= args.size()) return MATCH_FAILURE;
    if (args.at(index) != argumentName) return MATCH_FAILURE;
    index++;
    return MATCH_SUCCESS;
  }
};

class ContextFormat;

class ArgumentListFormat : public Format {
 public:
  ArgumentListFormat(std::string const &com) : Format(com) {}
  std::map<std::string, std::shared_ptr<Format>> formats;
  virtual std::string                            toStr(size_t indent,
                                                       size_t = 0,
                                                       size_t = 0,
                                                       size_t = 0) const override {
    std::stringstream ss;
    size_t            largestNameLength = 0;
    size_t            largestDataLength = 0;
    size_t            largestTypeLength = 0;
    for (auto const &x : formats) {
      auto vf = std::dynamic_pointer_cast<ValueFormat>(x.second);
      if (!vf) continue;
      largestNameLength = std::max(largestNameLength, vf->getName().length());
      largestDataLength = std::max(largestDataLength, vf->getDataLength());
      largestTypeLength = std::max(largestTypeLength, vf->getType().length());
    }
    for (auto const &x : formats) {
      if (ArgumentViewerImpl::isTypeOf<ContextFormat>(x.second)) continue;
      ss << x.second->toStr(indent, largestNameLength, largestDataLength,
                            largestTypeLength);
    }
    for (auto const &x : formats) {
      if (!ArgumentViewerImpl::isTypeOf<ContextFormat>(x.second)) continue;
      ss << x.second->toStr(indent);
    }
    return ss.str();
  }
  virtual MatchStatus match(std::vector<std::string> const &args,
                            size_t &index) const override {
    size_t                oldIndex = index;
    std::set<std::string> unusedFormats;
    for (auto const &x : formats) unusedFormats.insert(x.first);
    while (index < args.size()) {
      if (unusedFormats.empty()) {
        index = oldIndex;
        std::stringstream ss;
        ss << "Argument error:" << std::endl;
        ss << "following arguments cannot be matched: " << std::endl;
        for (size_t i = index; i < args.size(); ++i) ss << args.at(i) << " ";
        throw ex::MatchError(ss.str());
        return MATCH_ERROR;
      }
      std::string formatForRemoval = "";
      for (auto const &f : unusedFormats) {
        auto status = formats.at(f)->match(args, index);
        if (status == MATCH_ERROR) {
          index = oldIndex;
          return MATCH_ERROR;
        }
        if (status == MATCH_SUCCESS) {
          formatForRemoval = f;
          break;
        }
      }
      if (formatForRemoval == "") {
        std::stringstream ss;
        ss << "Argument error:" << std::endl;
        ss << "argument: " << args.at(index) << " at index: " << index
           << " cannot be matched";
        throw ex::MatchError(ss.str());
        index = oldIndex;
        return MATCH_ERROR;
      }
    }
    return MATCH_SUCCESS;
  }
};

class ContextFormat : public ArgumentListFormat {
 public:
  std::string argumentName;
  ContextFormat(std::string const &argument, std::string const &com)
      : ArgumentListFormat(com), argumentName(argument) {}
  virtual std::string toStr(size_t indent,
                            size_t = 0,
                            size_t = 0,
                            size_t = 0) const override {
    std::stringstream ss;
    for (size_t i = 0; i < indent; ++i) ss << " ";
    ss << argumentName << " ";
    ss << ArgumentViewerImpl::contextBegin << " - " << comment << std::endl;
    ss << ArgumentListFormat::toStr(indent + 2);
    for (size_t i = 0; i < indent; ++i) ss << " ";
    ss << ArgumentViewerImpl::contextEnd << std::endl;
    return ss.str();
  }
  virtual MatchStatus match(std::vector<std::string> const &args,
                            size_t &index) const override {
    size_t oldIndex = index;
    if (index >= args.size()) return MATCH_FAILURE;
    if (args.at(index) != argumentName) return MATCH_FAILURE;
    ++index;
    if (index >= args.size()) {
      index = oldIndex;
      std::stringstream ss;
      ss << "Argument error:" << std::endl;
      ss << "expected " << ArgumentViewerImpl::contextBegin
         << " after context argument: " << argumentName;
      ss << " not end of arguments";
      throw ex::MatchError(ss.str());
      return MATCH_ERROR;
    }
    if (args.at(index) != ArgumentViewerImpl::contextBegin) {
      index = oldIndex;
      std::stringstream ss;
      ss << "Argument error:" << std::endl;
      ss << "expected " << ArgumentViewerImpl::contextBegin
         << " after context argument: " << argumentName;
      ss << " not: " << args.at(index);
      throw ex::MatchError(ss.str());
      return MATCH_ERROR;
    }
    ++index;
    std::set<std::string> unusedFormats;
    for (auto const &x : formats) unusedFormats.insert(x.first);
    while (index < args.size() &&
           args.at(index) != ArgumentViewerImpl::contextEnd) {
      if (unusedFormats.empty()) {
        index = oldIndex;
        std::stringstream ss;
        ss << "Argument error:" << std::endl;
        ss << "following arguments do not belong in context: " << argumentName
           << ": " << std::endl;
        for (size_t i = index; i < args.size(); ++i) ss << args.at(i) << " ";
        throw ex::MatchError(ss.str());
        return MATCH_ERROR;
      }
      std::string formatForRemoval = "";
      for (auto const &f : unusedFormats) {
        auto status = formats.at(f)->match(args, index);
        if (status == MATCH_ERROR) {
          index = oldIndex;
          return MATCH_ERROR;
        }
        if (status == MATCH_SUCCESS) {
          formatForRemoval = f;
          break;
        }
      }
      if (formatForRemoval == "") {
        std::stringstream ss;
        ss << "Argument error:" << std::endl;
        ss << "argument: " << args.at(index)
           << " does not belong in context: " << argumentName;
        throw ex::MatchError(ss.str());
        index = oldIndex;
        return MATCH_ERROR;
      }
    }
    if (index >= args.size()) {
      index = oldIndex;
      std::stringstream ss;
      ss << "Argument error:" << std::endl;
      ss << "expected " << ArgumentViewerImpl::contextEnd
         << " at the end of context: " << argumentName;
      ss << " not end of arguments";
      throw ex::MatchError(ss.str());
      return MATCH_ERROR;
    }
    ++index;
    return MATCH_SUCCESS;
  }
};

namespace argumentViewer {
template <>
std::string ArgumentViewerImpl::typeName<float>() {
  return "f32";
}
template <>
std::string ArgumentViewerImpl::typeName<double>() {
  return "f64";
}
template <>
std::string ArgumentViewerImpl::typeName<int32_t>() {
  return "i32";
}
template <>
std::string ArgumentViewerImpl::typeName<int64_t>() {
  return "i64";
}
template <>
std::string ArgumentViewerImpl::typeName<uint32_t>() {
  return "u32";
}
template <>
std::string ArgumentViewerImpl::typeName<uint64_t>() {
  return "u64";
}
template <>
std::string ArgumentViewerImpl::typeName<std::string>() {
  return "string";
}
template <>
bool ArgumentViewerImpl::isValueConvertibleTo<float>(std::string const &text) {
  return txtUtils::isFloatingPoint(text) || txtUtils::isIntegral(text);
}
template <>
bool ArgumentViewerImpl::isValueConvertibleTo<double>(std::string const &text) {
  return txtUtils::isFloatingPoint(text) || txtUtils::isIntegral(text);
}
template <>
bool ArgumentViewerImpl::isValueConvertibleTo<int32_t>(
    std::string const &text) {
  return txtUtils::isIntegral(text);
}
template <>
bool ArgumentViewerImpl::isValueConvertibleTo<int64_t>(
    std::string const &text) {
  return txtUtils::isIntegral(text);
}
template <>
bool ArgumentViewerImpl::isValueConvertibleTo<uint32_t>(
    std::string const &text) {
  return txtUtils::isUint(text);
}
template <>
bool ArgumentViewerImpl::isValueConvertibleTo<uint64_t>(
    std::string const &text) {
  return txtUtils::isUint(text);
}
template <>
bool ArgumentViewerImpl::isValueConvertibleTo<std::string>(
    std::string const &text) {
  return txtUtils::isString(text);
}
std::string const ArgumentViewerImpl::contextBegin      = "{";
std::string const ArgumentViewerImpl::contextEnd        = "}";
std::string const ArgumentViewerImpl::fileSymbol        = "<";
size_t const      ArgumentViewerImpl::levelIndent       = 2;
size_t const      ArgumentViewerImpl::maxDataLineLength = 15;
}  // namespace argumentViewer

/**
 * @brief Contructor of ArgumentViewer
 * This class can be used for simple argument manipulations
 * int main(int argc,char*argv[]){
 *   auto args = std::make_shared<ArgumentViewer>(argc,argv);
 * }
 *
 * @param argc number of arguments, it should always be one or greater
 * (application name is always first argument)
 * @param argv list of arguments, it should always contain application name as
 * the first argument
 */
ArgumentViewer::ArgumentViewer(int argc, char *argv[]) {
  if (argc <= 0)
    throw ex::Exception("number of arguments has to be greater than 0");
  _impl = std::unique_ptr<ArgumentViewerImpl>(new ArgumentViewerImpl);
  assert(_impl != nullptr);
  _impl->applicationName = std::string(argv[0]);
  std::vector<std::string> args;
  for (int i = 1; i < argc; ++i) args.push_back(std::string(argv[i]));
  std::set<std::string> alreadyLoaded;
  _impl->loadArgumentFiles(args, alreadyLoaded);
  _impl->arguments = args;
  _impl->format    = std::make_shared<ArgumentListFormat>("");
}

/**
 * @brief Destructor of ArgumentViewer instance
 */
ArgumentViewer::~ArgumentViewer() {}

/**
 * @brief Returns application name (zeroes argument)
 *
 * @return application name
 */
std::string ArgumentViewer::getApplicationName() const {
  assert(_impl != nullptr);
  return _impl->applicationName;
}

/**
 * @brief Returns number of arguments without application name - it can be zero
 *
 * @return number of arguments without inclusion of application name
 */
size_t ArgumentViewer::getNofArguments() const {
  assert(_impl != nullptr);
  return _impl->arguments.size();
}

/**
 * @brief Gets argument on certain index
 *
 * @param index index of argument, should always be lesser than
 * getNofArguments()
 *
 * @return argument
 */
std::string ArgumentViewer::getArgument(size_t const &index) const {
  assert(_impl != nullptr);
  assert(index < _impl->arguments.size());
  return _impl->arguments.at(index);
}

/**
 * @brief If argument is present, it returns true
 *
 * @param argument name of argument
 *
 * @return true if argument is present in arguments
 */
bool ArgumentViewer::isPresent(std::string const &argument,
                               std::string const &com) const {
  assert(_impl != nullptr);

  auto alf = std::dynamic_pointer_cast<ArgumentListFormat>(_impl->format);
  assert(alf != nullptr);

  auto subFormatIt = alf->formats.find(argument);
  if (subFormatIt != alf->formats.end()) {
    auto subFormat = subFormatIt->second;
    if (!ArgumentViewerImpl::isTypeOf<IsPresentFormat>(subFormat))
      throw ex::Exception(
          std::string("argument: ") + argument +
          " is already defined as something else than isPresent format");
  } else
    alf->formats[argument] = std::make_shared<IsPresentFormat>(argument, com);
  if (alf->formats[argument]->comment == "")
    alf->formats[argument]->comment = com;

  return _impl->getArgumentPosition(argument) < _impl->arguments.size();
}

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
                          ArgumentViewerImpl::typeName<TYPE>() + " value");
    if (singleValueFormat->defaults != def)
      throw ex::Exception(
          std::string("argument: ") + argument +
          " has already been defined with different default value: ");
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
          ArgumentViewerImpl::typeName<TYPE>() + " values");
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

/**
 * @brief gets float value after argument
 *
 * @param argument argument name that has to be followed by float
 * @param def default value
 * @param comment comment
 *
 * @return next value after argument. if argument is not found, it returns def
 */
float ArgumentViewer::getf32(std::string const &argument,
                             float const &      def,
                             std::string const &com) const {
  assert(_impl != nullptr);
  return _impl->getArgumentWithFormat<float>(argument, def, com);
}

/**
 * @brief gets double value after argument
 *
 * @param argument argument name that has to be followed by double
 * @param def default value
 * @param comment comment
 *
 * @return next value after argument. if argument is not found, it returns def
 */
double ArgumentViewer::getf64(std::string const &argument,
                              double const &     def,
                              std::string const &com) const {
  assert(_impl != nullptr);
  return _impl->getArgumentWithFormat<double>(argument, def, com);
}

/**
 * @brief gets int32_t value after argument
 *
 * @param argument argument name that has to be followed by int32_t
 * @param def default value
 * @param comment comment
 *
 * @return newxt value after argument. if argument is not found, it returns def
 */
int32_t ArgumentViewer::geti32(std::string const &argument,
                               int32_t const &    def,
                               std::string const &com) const {
  assert(_impl != nullptr);
  return _impl->getArgumentWithFormat<int32_t>(argument, def, com);
}

/**
 * @brief gets int64_t value after argument
 *
 * @param argument argument name that has to be followed by int64_t
 * @param def default value
 * @param comment comment
 *
 * @return next value after argument. if argument is not found, it returns def
 */
int64_t ArgumentViewer::geti64(std::string const &argument,
                               int64_t const &    def,
                               std::string const &com) const {
  assert(_impl != nullptr);
  return _impl->getArgumentWithFormat<int64_t>(argument, def, com);
}

/**
 * @brief gets uint32_t value after argument
 *
 * @param argument argument name that has to be followed by uint32_t
 * @param def default value
 * @param comment comment
 *
 * @return next value after argument. if argument is not found, it returns def
 */
uint32_t ArgumentViewer::getu32(std::string const &argument,
                                uint32_t const &   def,
                                std::string const &com) const {
  assert(_impl != nullptr);
  return _impl->getArgumentWithFormat<uint32_t>(argument, def, com);
}

/**
 * @brief gets uint64_t value after argument
 *
 * @param argument argument name that has to be followed by uint64_t
 * @param def default value
 * @param comment comment
 *
 * @return next value after argument. if argument is not found, it returns def
 */
uint64_t ArgumentViewer::getu64(std::string const &argument,
                                uint64_t const &   def,
                                std::string const &com) const {
  assert(_impl != nullptr);
  return _impl->getArgumentWithFormat<uint64_t>(argument, def, com);
}

/**
 * @brief gets string value after argument
 *
 * @param argument argument name that has to be followed by string
 * @param def default value
 * @param comment comment
 *
 * @return next value after argument. if argument is not found, it returns def
 */
std::string ArgumentViewer::gets(std::string const &argument,
                                 std::string const &def,
                                 std::string const &com) const {
  assert(_impl != nullptr);
  return ArgumentViewerImpl::parseEscapeSequence(
      _impl->getArgumentWithFormat<std::string>(argument, def, com));
}

/**
 * @brief gets vector of float values after argument
 *
 * @param argument argument name that has to be followed by vector of floats
 * @param def default values
 * @param comment comment
 *
 * @return next values after argument. if number of next values is less that
 * number of def values, returned values are extended by def values
 */
std::vector<float> ArgumentViewer::getf32v(std::string const &       argument,
                                           std::vector<float> const &def,
                                           std::string const &com) const {
  assert(_impl != nullptr);
  return _impl->getArgumentsWithFormat<float>(argument, def, com);
}

/**
 * @brief gets vector of double values after argument
 *
 * @param argument argument name that has to be followed by vector of doubles
 * @param def default values
 * @param comment comment
 *
 * @return next values after argument. if number of next values is less that
 * number of def values, returned values are extended by def values
 */
std::vector<double> ArgumentViewer::getf64v(std::string const &        argument,
                                            std::vector<double> const &def,
                                            std::string const &com) const {
  assert(_impl != nullptr);
  return _impl->getArgumentsWithFormat<double>(argument, def, com);
}

/**
 * @brief gets vector of int32_t values after argument
 *
 * @param argument argument name that is followed by vector of int32_t values
 * @param def default values
 * @param comment comment
 *
 * @return next values after argument. if number of next values is less that
 * number of def values, returned values are extended by def values
 */
std::vector<int32_t> ArgumentViewer::geti32v(std::string const &argument,
                                             std::vector<int32_t> const &def,
                                             std::string const &com) const {
  assert(_impl != nullptr);
  return _impl->getArgumentsWithFormat<int32_t>(argument, def, com);
}

/**
 * @brief gets vector of int64_t values after argument
 *
 * @param argument argument name that is followed by vector of int64_t values
 * @param def default values
 * @param comment comment
 *
 * @return next values after argument. if number of next values is less that
 * number of def values, returned values are extended by def values
 */
std::vector<int64_t> ArgumentViewer::geti64v(std::string const &argument,
                                             std::vector<int64_t> const &def,
                                             std::string const &com) const {
  assert(_impl != nullptr);
  return _impl->getArgumentsWithFormat<int64_t>(argument, def, com);
}

/**
 * @brief gets vector of uint32_t values after argument
 *
 * @param argument argument name that is followed by vector of uint32_t values
 * @param def default values
 * @param comment comment
 *
 * @return next values after argument. if number of next values is less that
 * number of def values, returned values are extended by def values
 */
std::vector<uint32_t> ArgumentViewer::getu32v(std::string const &argument,
                                              std::vector<uint32_t> const &def,
                                              std::string const &com) const {
  assert(_impl != nullptr);
  return _impl->getArgumentsWithFormat<uint32_t>(argument, def, com);
}

/**
 * @brief gets vector of uint64_t values after argument
 *
 * @param argument argument name that is followed by vector of uint64_t values
 * @param def default values
 * @param comment comment
 *
 * @return next values after argument. if number of next values is less that
 * number of def values, returned values are extended by def values
 */
std::vector<uint64_t> ArgumentViewer::getu64v(std::string const &argument,
                                              std::vector<uint64_t> const &def,
                                              std::string const &com) const {
  assert(_impl != nullptr);
  return _impl->getArgumentsWithFormat<uint64_t>(argument, def, com);
}

/**
 * @brief gets context of argument with format: argument { arg0, ... }
 *
 * @param name context name - argument that is followed by { arg0, ... }
 * @param comment comment
 *
 * @return ArgumentViewer that contains arg*
 */
std::shared_ptr<ArgumentViewer> ArgumentViewer::getContext(
    std::string const &name, std::string const &com) const {
  assert(_impl != nullptr);
  auto alf = std::dynamic_pointer_cast<ArgumentListFormat>(_impl->format);
  assert(alf != nullptr);

  auto constructEmptyContext = [&]() {
    char const *argv[]    = {_impl->applicationName.c_str()};
    auto        result    = std::make_shared<ArgumentViewer>(1, (char **)argv);
    result->_impl->parent = this;
    result->_impl->format = alf->formats[name];
    return result;
  };

  auto subFormatIt = alf->formats.find(name);
  if (subFormatIt != alf->formats.end()) {
    auto subFormat = subFormatIt->second;
    if (!ArgumentViewerImpl::isTypeOf<ContextFormat>(subFormat))
      throw ex::Exception(std::string("argument: ") + name +
                          " is already defined as something else than context");
  } else
    alf->formats[name] = std::make_shared<ContextFormat>(name, com);
  if (alf->formats[name]->comment == "") alf->formats[name]->comment = com;

  std::vector<std::string> subArguments;
  if (!_impl->getContext(subArguments, name)) return constructEmptyContext();
  char const *appName[] = {_impl->applicationName.c_str()};
  auto        result    = std::make_shared<ArgumentViewer>(1, (char **)appName);
  result->_impl->parent = this;
  result->_impl->arguments = subArguments;
  result->_impl->format    = alf->formats.at(name);
  return result;
}

/**
 * @brief gets vector of string values
 *
 * @param argument argument that is followed by { arg0, arg1, ... }
 * @param def default values
 * @param comment comment
 *
 * @return return arg*, if size of args is less that def, it is
 * extended by def
 */
std::vector<std::string> ArgumentViewer::getsv(
    std::string const &             argument,
    std::vector<std::string> const &def,
    std::string const &             com) const {
  assert(_impl != nullptr);

  auto alf = std::dynamic_pointer_cast<ArgumentListFormat>(_impl->format);
  assert(alf != nullptr);

  auto subFormatIt = alf->formats.find(argument);
  if (subFormatIt != alf->formats.end()) {
    auto subFormat = subFormatIt->second;
    auto stringVectorFormat =
        std::dynamic_pointer_cast<StringVectorFormat>(subFormat);
    if (!stringVectorFormat)
      throw ex::Exception(
          std::string("argument: ") + argument +
          " is already defined as something else than vector of string values");
    if (stringVectorFormat->defaults != def)
      throw ex::Exception(
          std::string("argument: ") + argument +
          " has already been defined with different default values: ");
  } else
    alf->formats[argument] =
        std::make_shared<StringVectorFormat>(argument, def, com);
  if (alf->formats[argument]->comment == "")
    alf->formats[argument]->comment = com;

  std::vector<std::string> subArguments;
  if (!_impl->getContext(subArguments, argument)) return def;
  while (def.size() > subArguments.size())
    subArguments.push_back(def[subArguments.size()]);
  for (auto &x : subArguments) x = ArgumentViewerImpl::parseEscapeSequence(x);
  return subArguments;
}

std::string ArgumentViewer::toStr() const {
  assert(_impl != nullptr);
  return _impl->format->toStr();
}

bool ArgumentViewer::validate() const {
  assert(_impl != nullptr);
  assert(_impl->format != nullptr);
  if (_impl->parent != nullptr)
    throw ex::Exception("validation cannot be run on sub ArgumentViewer");
  size_t index = 0;
  return _impl->format->match(_impl->arguments, index) == Format::MATCH_SUCCESS;
}
