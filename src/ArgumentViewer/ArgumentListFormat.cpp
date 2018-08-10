#include <ArgumentViewer/ArgumentListFormat.h>
#include <ArgumentViewer/CommonFunctions.h>
#include <ArgumentViewer/Exception.h>
#include <ArgumentViewer/ContextFormat.h>
#include <set>

using namespace argumentViewer::ex;

ArgumentListFormat::ArgumentListFormat(string const &com) : Format(com) {}

string ArgumentListFormat::toStr(size_t indent, size_t, size_t, size_t) const {
  stringstream ss;
  size_t       largestNameLength = 0;
  size_t       largestDataLength = 0;
  size_t       largestTypeLength = 0;
  for (auto const &x : formats) {
    auto vf = dynamic_pointer_cast<ValueFormat>(x.second);
    if (!vf) continue;
    largestNameLength = max(largestNameLength, vf->getName().length());
    largestDataLength = max(largestDataLength, vf->getDataLength());
    largestTypeLength = max(largestTypeLength, vf->getType().length());
  }
  for (auto const &x : formats) {
    if (isTypeOf<ContextFormat>(x.second)) continue;
    ss << x.second->toStr(indent, largestNameLength, largestDataLength,
                          largestTypeLength);
  }
  for (auto const &x : formats) {
    if (!isTypeOf<ContextFormat>(x.second)) continue;
    ss << x.second->toStr(indent);
  }
  return ss.str();
}

Format::MatchStatus ArgumentListFormat::match(vector<string> const &args,
                                              size_t &index) const {
  size_t      oldIndex = index;
  set<string> unusedFormats;
  for (auto const &x : formats) unusedFormats.insert(x.first);
  while (index < args.size()) {
    if (unusedFormats.empty()) {
      index = oldIndex;
      stringstream ss;
      ss << "Argument error:" << endl;
      ss << "following arguments cannot be matched: " << endl;
      for (size_t i = index; i < args.size(); ++i) ss << args.at(i) << " ";
      throw MatchError(ss.str());
      return MATCH_ERROR;
    }
    string formatForRemoval = "";
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
      stringstream ss;
      ss << "Argument error:" << endl;
      ss << "argument: " << args.at(index) << " at index: " << index
         << " cannot be matched";
      throw MatchError(ss.str());
      index = oldIndex;
      return MATCH_ERROR;
    }
  }
  return MATCH_SUCCESS;
}
