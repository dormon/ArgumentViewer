#include <ArgumentViewer/Exception.h>
#include <ArgumentViewer/private/Globals.h>
#include <ArgumentViewer/private/LineSplitter.h>
#include <ArgumentViewer/private/StringVectorFormat.h>

using namespace argumentViewer::ex;

StringVectorFormat::StringVectorFormat(string const &        argument,
                                       vector<string> const &defs,
                                       string const &        com)
    : ValueFormat(argument, com), defaults(defs) {}

string StringVectorFormat::getData() const {
  LineSplitter splitter;
  splitter.addString(contextBegin);
  bool first = true;
  for (auto const &x : defaults) {
    if (first)
      first = false;
    else
      splitter.addString(" ");
    splitter.addString(x);
  }
  splitter.addString(contextEnd);
  return splitter.get();
}

size_t StringVectorFormat::getDataLength() const {
  auto   data          = getData();
  size_t maxLineLength = 0;
  size_t lineStart     = 0;
  size_t lineEnd;
  while ((lineEnd = data.find("\n", lineStart)) != string::npos) {
    maxLineLength = max(maxLineLength, lineEnd - lineStart + 1);
    lineStart     = lineEnd + 1;
  }
  maxLineLength = max(maxLineLength, data.length() - lineStart + 1);
  return maxLineLength;
}

string StringVectorFormat::getType() const { return "string*"; }

Format::MatchStatus StringVectorFormat::match(vector<string> const &args,
                                              size_t &index) const {
  if (index >= args.size()) return MATCH_FAILURE;
  size_t oldIndex = index;
  if (args.at(index) != argumentName) return MATCH_FAILURE;
  ++index;
  if (args.at(index) != contextBegin) {
    index = oldIndex;
    stringstream ss;
    ss << "Argument error:" << endl;
    ss << "expected " << contextBegin << " after argument: " << argumentName;
    ss << " not argument: " << args.at(index);
    throw MatchError(ss.str());
    return MATCH_ERROR;
  }
  ++index;
  while (index < args.size() && args.at(index) != contextEnd) ++index;
  if (index >= args.size()) {
    index = oldIndex;
    stringstream ss;
    ss << "Argument error:" << endl;
    ss << "expected " << contextEnd
       << " at the end of context argument: " << argumentName;
    ss << " not end of arguments";
    throw MatchError(ss.str());
    return MATCH_ERROR;
  }
  ++index;
  return MATCH_SUCCESS;
}
