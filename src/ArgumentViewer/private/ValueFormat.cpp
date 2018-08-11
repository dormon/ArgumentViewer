#include <ArgumentViewer/private/CommonFunctions.h>
#include <ArgumentViewer/private/Globals.h>
#include <ArgumentViewer/private/ValueFormat.h>

ValueFormat::ValueFormat(string const &argument, string const &com)
    : Format(com), argumentName(argument)
{
}

string ValueFormat::getName() const { return argumentName; }

size_t ValueFormat::getDataLength() const { return getData().length(); }

void writeTypePreDecorator(stringstream &ss) { ss << typePreDecorator; }

void writeTypePostDecorator(stringstream &ss) { ss << typePostDecorator; }

void writeDefaultsSeparator(stringstream &ss) { ss << defaultsSeparator; }

void writeAlignedType(stringstream &ss, string const &type, size_t maxTypeSize)
{
  writeTypePreDecorator(ss);
  writeAlignedString(ss, type, maxTypeSize);
  writeTypePostDecorator(ss);
}

void ValueFormat::writeData(stringstream&ss,size_t maxDataSize)const{
  size_t const indentation = ss.str().length();
  auto const lines = splitString(getData(),"\n");
  size_t const firstLine = 0;
  size_t const lastLine = lines.size()-1;
  for(size_t i=0;i<lines.size();++i){
    if(i != firstLine)writeIndentation(ss,indentation);
    writeAlignedString(ss,lines[i],maxDataSize);
    if(i != lastLine )writeLineEnd(ss);
  }
  if(lines.empty())
    writeSpaces(ss,maxDataSize);
}

string ValueFormat::toStr(size_t indent,
                          size_t maxNameSize,
                          size_t maxDataSize,
                          size_t maxTypeSize) const
{
  stringstream ss;
  writeIndentation(ss, indent);
  writeAlignedString(ss, getName(), maxNameSize);
  writeDefaultsSeparator(ss);
  writeData(ss,maxDataSize);
  writeAlignedType(ss, getType(), maxTypeSize);
  writeComment(ss);
  writeLineEnd(ss);
  return ss.str();
}
