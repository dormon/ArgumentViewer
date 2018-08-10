#include <ArgumentViewer/ValueFormat.h>
#include <ArgumentViewer/CommonFunctions.h>

ValueFormat::ValueFormat(string const &argument, string const &com)
    : Format(com), argumentName(argument) {}

string ValueFormat::getName() const { return argumentName; }

size_t ValueFormat::getDataLength() const { return getData().length(); }

string ValueFormat::toStr(size_t indent,
                          size_t maxNameSize,
                          size_t maxDataSize,
                          size_t maxTypeSize) const {
  stringstream ss;
  for (size_t i = 0; i < indent; ++i) ss << " ";
  ss << alignString(getName(), maxNameSize);
  ss << " = ";
  size_t indentation = ss.str().length();
  auto   data        = getData();
  bool   firstLine   = true;
  size_t lineEnd     = 0;
  size_t lineStart   = 0;
  while ((lineEnd = data.find("\n", lineStart)) != string::npos) {
    if (firstLine)
      firstLine = false;
    else
      for (size_t i = 0; i < indentation; ++i) ss << " ";
    ss << data.substr(lineStart, lineEnd - lineStart + 1);
    lineStart = lineEnd + 1;
  }
  if (!firstLine)
    for (size_t i = 0; i < indentation; ++i) ss << " ";
  ss << alignString(data.substr(lineStart, data.length() - lineStart + 1),
                    maxDataSize);

  // ss<<ArgumentViewerImpl::alignString(getData(),maxDataSize);
  ss << " [" << alignString(getType(), maxTypeSize);
  ss << "]";
  if (comment != "") ss << " - " << comment;
  ss << endl;
  return ss.str();
}
