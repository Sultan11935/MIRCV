#include "utils.h"
#include <algorithm>
#include <cwctype>
#include <locale>
#include <sstream>
#include <iostream>
#include <unordered_set>
#include "libstemmer.h"
#include <codecvt>

// Conversion functions
std::string wstringToUtf8(const std::wstring &wstr) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.to_bytes(wstr);
}

std::wstring utf8ToWstring(const std::string &str) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(str);
}

// Define stopwords
const std::unordered_set<std::wstring> stopwords = {
    L"the", L"is", L"and", L"a", L"of", L"to", L"in", L"it", L"for", L"on", L"with", L"this", L"that"
};

// Preprocess a word
std::wstring preprocessWord(const std::wstring &word) {
    std::wstring result;
    result.reserve(word.size());

    for (wchar_t ch : word) {
    if (std::iswalnum(ch)) { // Check if the character is alphanumeric
        result += std::towlower(ch);
    } else if (std::iswpunct(ch)) { 
        continue; // Ignore punctuation
    } else if (ch == L'/' || ch == L'(' || ch == L')') { 
        return L""; // Skip terms with annotations or special characters
    }
}



    result.erase(result.begin(), std::find_if(result.begin(), result.end(), [](wchar_t c) {
        return !std::iswspace(c);
    }));
    result.erase(std::find_if(result.rbegin(), result.rend(), [](wchar_t c) {
        return !std::iswspace(c);
    }).base(), result.end());

    if (result.empty()) {
        return L"";
    }

    if (!std::all_of(result.begin(), result.end(), [](wchar_t c) { return std::iswprint(c); })) {
        //std::wcerr << L"Skipping malformed or non-printable term: " << word << std::endl;
        return L"";
    }

        #ifdef ENABLE_STOPWORDS
            //std::wcout << L"Before stopword removal: " << result << std::endl;
            if (stopwords.find(result) != stopwords.end()) {
                //std::wcout << L"Skipping stopword: " << result << std::endl;
                return L"";
            }
        #endif

    #ifdef ENABLE_STEMMING
        //std::wcout << L"Before stemming: " << result << std::endl;

        if (!result.empty()) {
            static sb_stemmer* stemmer = sb_stemmer_new("english", nullptr);
            if (!stemmer) {
                std::cerr << "Error: Could not initialize Snowball stemmer." << std::endl;
                return result;
            }

            std::string temp = wstringToUtf8(result);
            const sb_symbol* stemmed_symbol = sb_stemmer_stem(stemmer, reinterpret_cast<const sb_symbol*>(temp.c_str()), temp.size());
            if (stemmed_symbol) {
                std::string stemmed(reinterpret_cast<const char*>(stemmed_symbol), sb_stemmer_length(stemmer));
                result = utf8ToWstring(stemmed); // Convert back to wstring
            }
        }
    #endif


    return result;
}
