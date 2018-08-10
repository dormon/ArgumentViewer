#include <ArgumentViewer/Exception.h>
#include <ArgumentViewer/private/ArgumentViewerImpl.h>
#include <ArgumentViewer/private/Globals.h>

#include <MealyMachine/MealyMachine.h>

using namespace argumentViewer;

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
    if (getArgument(argumentIndex) == contextEnd) {
      if (contextCounter == 0)
        return true;
      else
        contextCounter--;
    }
    if (getArgument(argumentIndex) == contextBegin)
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


