#ifndef UTILS_H
#define UTILS_H

#include <string>

std::wstring preprocessWord(const std::wstring& word);
std::wstring utf8ToWstring(const std::string& str);
std::string wstringToUtf8(const std::wstring& wstr);

#endif
