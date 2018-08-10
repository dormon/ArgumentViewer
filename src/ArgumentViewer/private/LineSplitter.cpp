#include <ArgumentViewer/private/Globals.h>
#include <ArgumentViewer/private/LineSplitter.h>

string LineSplitter::get() const { return ss.str(); }

void LineSplitter::addString(string const &text) {
  if (ss.str().length() - lineStart + 1 + text.length() >= maxDataLineLength) {
    ss << endl;
    lineStart = ss.str().length();
    if (text == " ") return;
  }
  ss << text;
}
