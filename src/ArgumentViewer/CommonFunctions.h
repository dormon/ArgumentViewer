#pragma once

#include <TxtUtils/TxtUtils.h>
#include <string>

using namespace std;

string parseEscapeSequence(string const &text);

string alignString(string const &text,
                        size_t             alignment  = 0,
                        char               filler     = ' ',
                        bool               fillerLeft = false);

template <typename TO, typename FROM>
bool isTypeOf(shared_ptr<FROM> const &value) {
  return dynamic_pointer_cast<TO>(value) != nullptr;
}

template <typename TYPE>
string typeName();

template <typename TYPE>
bool isValueConvertibleTo(string const &text);

template <>
string typeName<float>();
template <>
string typeName<double>();
template <>
string typeName<int32_t>();
template <>
string typeName<int64_t>();

template <>
string typeName<uint32_t>();

template <>
string typeName<uint64_t>();

template <>
string typeName<string>();

template <>
bool isValueConvertibleTo<float>(string const &text);

template <>
bool isValueConvertibleTo<double>(string const &text);

template <>
bool isValueConvertibleTo<int32_t>(string const &text);

template <>
bool isValueConvertibleTo<int64_t>(string const &text);

template <>
bool isValueConvertibleTo<uint32_t>(string const &text);
template <>
bool isValueConvertibleTo<uint64_t>(string const &text);
template <>
bool isValueConvertibleTo<string>(string const &text);

