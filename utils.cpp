#include "utils.h"
#include <algorithm>
#include <cwctype>

std::wstring preprocessWord(const std::wstring& word) {
    std::wstring result = word;
    std::transform(result.begin(), result.end(), result.begin(), std::towlower);
    result.erase(std::remove_if(result.begin(), result.end(), std::iswpunct), result.end());
    return result;
}
