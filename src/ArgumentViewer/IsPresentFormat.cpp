#include <ArgumentViewer/IsPresentFormat.h>
#include <ArgumentViewer/CommonFunctions.h>
#include <sstream>

IsPresentFormat::IsPresentFormat(string const &name, string const &com)
    : Format(com), argumentName(name) {}

string IsPresentFormat::toStr(size_t indent,
                              size_t maxNameSize,
                              size_t maxDataSize,
                              size_t maxTypeSize) const {
  stringstream ss;
  for (size_t i = 0; i < indent; ++i) ss << " ";
  ss << alignString(argumentName, maxNameSize);
  ss << "   ";
  ss << alignString("", maxDataSize);
  ss << "  " << alignString("", maxTypeSize);
  ss << " ";
  if (comment != "") ss << " - " << comment << endl;
  return ss.str();
}

Format::MatchStatus IsPresentFormat::match(vector<string> const &args,
                                            size_t &              index) const {
  if (index >= args.size()) return MATCH_FAILURE;
  if (args.at(index) != argumentName) return MATCH_FAILURE;
  index++;
  return MATCH_SUCCESS;
}
