#include <ArgumentViewer/Exception.h>
#include <ArgumentViewer/private/ContextFormat.h>
#include <set>

using namespace argumentViewer::ex;

ContextFormat::ContextFormat(std::string const &argument,
                             std::string const &com)
    : ArgumentListFormat(com), argumentName(argument) {}

string ContextFormat::toStr(size_t indent, size_t, size_t, size_t) const {
  std::stringstream ss;
  for (size_t i = 0; i < indent; ++i) ss << " ";
  ss << argumentName << " ";
  ss << contextBegin << " - " << comment << std::endl;
  ss << ArgumentListFormat::toStr(indent + 2);
  for (size_t i = 0; i < indent; ++i) ss << " ";
  ss << contextEnd << std::endl;
  return ss.str();
}

Format::MatchStatus ContextFormat::match(std::vector<std::string> const &args,
                                         size_t &index) const {
  size_t oldIndex = index;
  if (index >= args.size()) return MATCH_FAILURE;
  if (args.at(index) != argumentName) return MATCH_FAILURE;
  ++index;
  if (index >= args.size()) {
    index = oldIndex;
    std::stringstream ss;
    ss << "Argument error:" << std::endl;
    ss << "expected " << contextBegin
       << " after context argument: " << argumentName;
    ss << " not end of arguments";
    throw MatchError(ss.str());
    return MATCH_ERROR;
  }
  if (args.at(index) != contextBegin) {
    index = oldIndex;
    std::stringstream ss;
    ss << "Argument error:" << std::endl;
    ss << "expected " << contextBegin
       << " after context argument: " << argumentName;
    ss << " not: " << args.at(index);
    throw MatchError(ss.str());
    return MATCH_ERROR;
  }
  ++index;
  std::set<std::string> unusedFormats;
  for (auto const &x : formats) unusedFormats.insert(x.first);
  while (index < args.size() &&
         args.at(index) != contextEnd) {
    if (unusedFormats.empty()) {
      index = oldIndex;
      std::stringstream ss;
      ss << "Argument error:" << std::endl;
      ss << "following arguments do not belong in context: " << argumentName
         << ": " << std::endl;
      for (size_t i = index; i < args.size(); ++i) ss << args.at(i) << " ";
      throw MatchError(ss.str());
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
      throw MatchError(ss.str());
      index = oldIndex;
      return MATCH_ERROR;
    }
  }
  if (index >= args.size()) {
    index = oldIndex;
    std::stringstream ss;
    ss << "Argument error:" << std::endl;
    ss << "expected " << contextEnd
       << " at the end of context: " << argumentName;
    ss << " not end of arguments";
    throw MatchError(ss.str());
    return MATCH_ERROR;
  }
  ++index;
  return MATCH_SUCCESS;
}
