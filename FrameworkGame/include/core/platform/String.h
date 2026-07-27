#pragma once
#include <string>
#include <cstring>
#include <string_view>
#include <sstream>
#include <memory>

namespace sfmx
{

/************************************************************************/
/*
 * String related
 */
/************************************************************************/
/**
 * @brief Wide string stream used for primarily for constructing strings
 *        consisting of ASCII text.
 */
using StringStream = std::stringstream;

/**
 * @brief Basic string that uses geEngine memory allocators.
 */
template<typename T>
using BasicString = std::basic_string<T, std::char_traits<T>, std::allocator<T>>;

/**
 * @brief Basic string stream that uses geEngine memory allocators.
 */
template<typename T>
using BasicStringStream = std::basic_stringstream<T, std::char_traits<T>, std::allocator<T>>;

/**
 * @brief Wide string used primarily for handling Unicode text.
 */
using WString = std::wstring;

/**
 * @brief Narrow string used primarily for handling ASCII text.
 */
using String = std::string;//= BasicString<ANSICHAR>;

using StringView = std::string_view;

/**
 * @brief Wide string used UTF-16 encoded strings.
 */
using U16String = BasicString<char16_t>;

/**
 * @brief Wide string used UTF-32 encoded strings.
 */
using U32String = BasicString<char32_t>;



}