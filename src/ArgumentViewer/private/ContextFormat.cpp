#include <ArgumentViewer/Exception.h>
#include <ArgumentViewer/private/ContextFormat.h>
#include <set>

using namespace argumentViewer::ex;

ContextFormat::ContextFormat(string const &argument,
                             string const &com)
    : ArgumentListFormat(com), argumentName(argument) {}

string ContextFormat::toStr(size_t indent, size_t, size_t, size_t) const {
  stringstream ss;
  for (size_t i = 0; i < indent; ++i) ss << " ";
  ss << argumentName << " ";
  ss << contextBegin << " - " << comment << endl;
  ss << ArgumentListFormat::toStr(indent + 2);
  for (size_t i = 0; i < indent; ++i) ss << " ";
  ss << contextEnd << endl;
  return ss.str();
}

void throwIfIndexIsOutOfRange(vector<string> const &args,size_t &index,string const&argumentName){
  if (index < args.size()) return;
  stringstream ss;
  ss << "Argument error:" << endl;
  ss << "expected " << contextBegin
     << " after context argument: " << argumentName;
  ss << " not end of arguments";
  throw MatchError(ss.str());
}

void throwIfArgumentIsNotContextBegin(vector<string> const &args,size_t &index,string const&argumentName){
  if (args.at(index) == contextBegin) return;
  stringstream ss;
  ss << "Argument error:" << endl;
  ss << "expected " << contextBegin
     << " after context argument: " << argumentName;
  ss << " not: " << args.at(index);
  throw MatchError(ss.str());
}

void throwIfUnusedFormatsIsEmpty(vector<string> const &args,size_t &index,set<string>const&unusedFormats,string const&argumentName){
  if (!unusedFormats.empty()) return;
  stringstream ss;
  ss << "Argument error:" << endl;
  ss << "following arguments do not belong in context: " << argumentName
     << ": " << endl;
  for (size_t i = index; i < args.size(); ++i) ss << args.at(i) << " ";
  throw MatchError(ss.str());
}

void throwIfFromatForRemovalIsEmpty(string const&formatForRemoval,vector<string>const&args,size_t&index,string const&argumentName){
  if (formatForRemoval != "") return;
  stringstream ss;
  ss << "Argument error:" << endl;
  ss << "argument: " << args.at(index)
     << " does not belong in context: " << argumentName;
  throw MatchError(ss.str());
}

void throwIfArgumentsUbruptlyEnds(vector<string>const&args,size_t&index,string const&argumentName){
  if (index < args.size()) return;
  stringstream ss;
  ss << "Argument error:" << endl;
  ss << "expected " << contextEnd
     << " at the end of context: " << argumentName;
  ss << " not end of arguments";
  throw MatchError(ss.str());
}

void throwIfArgumentsDoNotContainContextEnd(vector<string>const&args,size_t&index,string const&argumentName){
  if (args.at(index) == contextEnd)return;
  stringstream ss;
  ss << "Argument error:" << endl;
  ss << "expected " << contextEnd
     << " at the end of context: " << argumentName;
  ss << " not argument: " << args.at(index);
  throw MatchError(ss.str());
}

Format::MatchStatus ContextFormat::match(vector<string> const &args,
                                         size_t &index) const {
  size_t oldIndex = index;
  if (index >= args.size()) return MATCH_FAILURE;
  if (args.at(index) != argumentName) return MATCH_FAILURE;
  ++index;
  throwIfIndexIsOutOfRange(args,index,argumentName);
  throwIfArgumentIsNotContextBegin(args,index,argumentName);
  ++index;
  set<string> unusedFormats;
  for (auto const &x : formats) unusedFormats.insert(x.first);
  while (index < args.size() &&
         args.at(index) != contextEnd) {
    throwIfUnusedFormatsIsEmpty(args,index,unusedFormats,argumentName);
    string formatForRemoval = "";
    for (auto const &f : unusedFormats) {
      auto status = formats.at(f)->match(args, index);
      if (status == MATCH_FAILURE) continue;
      formatForRemoval = f;
      break;
    }
    throwIfFromatForRemovalIsEmpty(formatForRemoval,args,index,argumentName);
    unusedFormats.erase(formatForRemoval);
  }
  throwIfArgumentsUbruptlyEnds(args,index,argumentName);
  throwIfArgumentsDoNotContainContextEnd(args,index,argumentName);
  ++index;
  return MATCH_SUCCESS;
}
