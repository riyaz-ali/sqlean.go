// ---------------------------------
// Following is an amalgamated version of sqlean v0.21.6
// License @ https://github.com/nalgeon/sqlean/blob/main/LICENSE
// Find more details @ https://github.com/nalgeon/sqlean
// All copyrights belong to original author(s)
// ---------------------------------

#include "sqlean.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef SQLEAN_ENABLE_TEXT
// ---------------------------------
// src/text/bstring.c
// ---------------------------------
// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Byte string data structure.

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// bstring_new creates an empty string.
static ByteString bstring_new(void) {
    char* bytes = "\0";
    ByteString str = {.bytes = bytes, .length = 0, .owning = false};
    return str;
}

// bstring_from_cstring creates a new string that wraps an existing C string.
static ByteString bstring_from_cstring(const char* const cstring, size_t length) {
    ByteString str = {.bytes = cstring, .length = length, .owning = false};
    return str;
}

// bstring_clone creates a new string by copying an existing C string.
static ByteString bstring_clone(const char* const cstring, size_t length) {
    char* bytes = calloc(length + 1, sizeof(char));
    if (bytes == NULL) {
        ByteString str = {NULL, 0, true};
        return str;
    }
    memcpy(bytes, cstring, length);
    ByteString str = {bytes, length, true};
    return str;
}

// bstring_to_cstring converts the string to a zero-terminated C string.
static const char* bstring_to_cstring(ByteString str) {
    if (str.bytes == NULL) {
        return NULL;
    }
    return str.bytes;
}

// bstring_free destroys the string, freeing resources if necessary.
static void bstring_free(ByteString str) {
    if (str.owning && str.bytes != NULL) {
        free((void*)str.bytes);
    }
}

// bstring_at returns a character by its index in the string.
static char bstring_at(ByteString str, size_t idx) {
    if (str.length == 0) {
        return 0;
    }
    if (idx < 0 || idx >= str.length) {
        return 0;
    };
    return str.bytes[idx];
}

// bstring_slice returns a slice of the string,
// from the `start` index (inclusive) to the `end` index (non-inclusive).
// Negative `start` and `end` values count from the end of the string.
static ByteString bstring_slice(ByteString str, int start, int end) {
    if (str.length == 0) {
        return bstring_new();
    }

    // adjusted start index
    start = start < 0 ? str.length + start : start;
    // python-compatible: treat negative start index larger than the length of the string as zero
    start = start < 0 ? 0 : start;
    // adjusted start index should be less the the length of the string
    if (start >= (int)str.length) {
        return bstring_new();
    }

    // adjusted end index
    end = end < 0 ? str.length + end : end;
    // python-compatible: treat end index larger than the length of the string
    // as equal to the length
    end = end > (int)str.length ? (int)str.length : end;
    // adjusted end index should be >= 0
    if (end < 0) {
        return bstring_new();
    }

    // adjusted start index should be less than adjusted end index
    if (start >= end) {
        return bstring_new();
    }

    char* at = (char*)str.bytes + start;
    size_t length = end - start;
    ByteString slice = bstring_clone(at, length);
    return slice;
}

// bstring_substring returns a substring of `length` characters,
// starting from the `start` index.
static ByteString bstring_substring(ByteString str, size_t start, size_t length) {
    if (length > str.length - start) {
        length = str.length - start;
    }
    return bstring_slice(str, start, start + length);
}

// bstring_contains_after checks if the other string is a substring of the original string,
// starting at the `start` index.
static bool bstring_contains_after(ByteString str, ByteString other, size_t start) {
    if (start + other.length > str.length) {
        return false;
    }
    for (size_t idx = 0; idx < other.length; idx++) {
        if (str.bytes[start + idx] != other.bytes[idx]) {
            return false;
        }
    }
    return true;
}

// bstring_index_char returns the first index of the character in the string
// after the `start` index, inclusive.
static int bstring_index_char(ByteString str, char chr, size_t start) {
    for (size_t idx = start; idx < str.length; idx++) {
        if (str.bytes[idx] == chr) {
            return idx;
        }
    }
    return -1;
}

// bstring_last_index_char returns the last index of the character in the string
// before the `end` index, inclusive.
static int bstring_last_index_char(ByteString str, char chr, size_t end) {
    if (end >= str.length) {
        return -1;
    }
    for (int idx = end; idx >= 0; idx--) {
        if (str.bytes[idx] == chr) {
            return idx;
        }
    }
    return -1;
}

// bstring_index_after returns the index of the substring in the original string
// after the `start` index, inclusive.
static int bstring_index_after(ByteString str, ByteString other, size_t start) {
    if (other.length == 0) {
        return start;
    }
    if (str.length == 0 || other.length > str.length) {
        return -1;
    }

    size_t cur_idx = start;
    while (cur_idx < str.length) {
        int match_idx = bstring_index_char(str, other.bytes[0], cur_idx);
        if (match_idx == -1) {
            return match_idx;
        }
        if (bstring_contains_after(str, other, match_idx)) {
            return match_idx;
        }
        cur_idx = match_idx + 1;
    }
    return -1;
}

// bstring_index returns the first index of the substring in the original string.
static int bstring_index(ByteString str, ByteString other) {
    return bstring_index_after(str, other, 0);
}

// bstring_last_index returns the last index of the substring in the original string.
static int bstring_last_index(ByteString str, ByteString other) {
    if (other.length == 0) {
        return str.length - 1;
    }
    if (str.length == 0 || other.length > str.length) {
        return -1;
    }

    int cur_idx = str.length - 1;
    while (cur_idx >= 0) {
        int match_idx = bstring_last_index_char(str, other.bytes[0], cur_idx);
        if (match_idx == -1) {
            return match_idx;
        }
        if (bstring_contains_after(str, other, match_idx)) {
            return match_idx;
        }
        cur_idx = match_idx - 1;
    }

    return -1;
}

// bstring_contains checks if the string contains the substring.
static bool bstring_contains(ByteString str, ByteString other) {
    return bstring_index(str, other) != -1;
}

// bstring_equals checks if two strings are equal character by character.
static bool bstring_equals(ByteString str, ByteString other) {
    if (str.bytes == NULL && other.bytes == NULL) {
        return true;
    }
    if (str.bytes == NULL || other.bytes == NULL) {
        return false;
    }
    if (str.length != other.length) {
        return false;
    }
    return bstring_contains_after(str, other, 0);
}

// bstring_has_prefix checks if the string starts with the `other` substring.
static bool bstring_has_prefix(ByteString str, ByteString other) {
    return bstring_index(str, other) == 0;
}

// bstring_has_suffix checks if the string ends with the `other` substring.
static bool bstring_has_suffix(ByteString str, ByteString other) {
    if (other.length == 0) {
        return true;
    }
    int idx = bstring_last_index(str, other);
    return idx < 0 ? false : (size_t)idx == (str.length - other.length);
}

// bstring_count counts how many times the `other` substring is contained in the original string.
static size_t bstring_count(ByteString str, ByteString other) {
    if (str.length == 0 || other.length == 0 || other.length > str.length) {
        return 0;
    }

    size_t count = 0;
    size_t char_idx = 0;
    while (char_idx < str.length) {
        int match_idx = bstring_index_after(str, other, char_idx);
        if (match_idx == -1) {
            break;
        }
        count += 1;
        char_idx = match_idx + other.length;
    }

    return count;
}

// bstring_split_part splits the string by the separator and returns the nth part (0-based).
static ByteString bstring_split_part(ByteString str, ByteString sep, size_t part) {
    if (str.length == 0 || sep.length > str.length) {
        return bstring_new();
    }
    if (sep.length == 0) {
        if (part == 0) {
            return bstring_slice(str, 0, str.length);
        } else {
            return bstring_new();
        }
    }

    size_t found = 0;
    size_t prev_idx = 0;
    size_t char_idx = 0;
    while (char_idx < str.length) {
        int match_idx = bstring_index_after(str, sep, char_idx);
        if (match_idx == -1) {
            break;
        }
        if (found == part) {
            return bstring_slice(str, prev_idx, match_idx);
        }
        found += 1;
        prev_idx = match_idx + sep.length;
        char_idx = match_idx + sep.length;
    }

    if (found == part) {
        return bstring_slice(str, prev_idx, str.length);
    }

    return bstring_new();
}

// bstring_join joins strings using the separator and returns the resulting string.
static ByteString bstring_join(ByteString* strings, size_t count, ByteString sep) {
    // calculate total string length
    size_t total_length = 0;
    for (size_t idx = 0; idx < count; idx++) {
        ByteString str = strings[idx];
        total_length += str.length;
        // no separator after the last one
        if (idx != count - 1) {
            total_length += sep.length;
        }
    }

    // allocate memory for the bytes
    size_t total_size = total_length * sizeof(char);
    char* bytes = malloc(total_size + 1);
    if (bytes == NULL) {
        ByteString str = {NULL, 0, false};
        return str;
    }

    // copy bytes from each string with separator in between
    char* at = bytes;
    for (size_t idx = 0; idx < count; idx++) {
        ByteString str = strings[idx];
        memcpy(at, str.bytes, str.length);
        at += str.length;
        if (idx != count - 1 && sep.length != 0) {
            memcpy(at, sep.bytes, sep.length);
            at += sep.length;
        }
    }

    bytes[total_length] = '\0';
    ByteString str = {bytes, total_length, true};
    return str;
}

// bstring_concat concatenates strings and returns the resulting string.
static ByteString bstring_concat(ByteString* strings, size_t count) {
    ByteString sep = bstring_new();
    return bstring_join(strings, count, sep);
}

// bstring_repeat concatenates the string to itself a given number of times
// and returns the resulting string.
static ByteString bstring_repeat(ByteString str, size_t count) {
    // calculate total string length
    size_t total_length = str.length * count;

    // allocate memory for the bytes
    size_t total_size = total_length * sizeof(char);
    char* bytes = malloc(total_size + 1);
    if (bytes == NULL) {
        ByteString res = {NULL, 0, false};
        return res;
    }

    // copy bytes
    char* at = bytes;
    for (size_t idx = 0; idx < count; idx++) {
        memcpy(at, str.bytes, str.length);
        at += str.length;
    }

    bytes[total_size] = '\0';
    ByteString res = {bytes, total_length, true};
    return res;
}

// bstring_replace replaces the `old` substring with the `new` substring in the original string,
// but not more than `max_count` times.
static ByteString bstring_replace(ByteString str,
                                  ByteString old,
                                  ByteString new,
                                  size_t max_count) {
    // count matches of the old string in the source string
    size_t count = bstring_count(str, old);
    if (count == 0) {
        return bstring_slice(str, 0, str.length);
    }

    // limit the number of replacements
    if (max_count >= 0 && count > max_count) {
        count = max_count;
    }

    // k matches split string into (k+1) parts
    // allocate an array for them
    size_t parts_count = count + 1;
    ByteString* strings = malloc(parts_count * sizeof(ByteString));
    if (strings == NULL) {
        ByteString res = {NULL, 0, false};
        return res;
    }

    // split the source string where it matches the old string
    // and fill the strings array with these parts
    size_t part_idx = 0;
    size_t char_idx = 0;
    while (char_idx < str.length && part_idx < count) {
        int match_idx = bstring_index_after(str, old, char_idx);
        if (match_idx == -1) {
            break;
        }
        // slice from the prevoius match to the current match
        strings[part_idx] = bstring_slice(str, char_idx, match_idx);
        part_idx += 1;
        char_idx = match_idx + old.length;
    }
    // "tail" from the last match to the end of the source string
    strings[part_idx] = bstring_slice(str, char_idx, str.length);

    // join all the parts using new string as a separator
    ByteString res = bstring_join(strings, parts_count, new);
    // free string parts
    for (size_t idx = 0; idx < parts_count; idx++) {
        bstring_free(strings[idx]);
    }
    free(strings);
    return res;
}

// bstring_replace_all replaces all `old` substrings with the `new` substrings
// in the original string.
static ByteString bstring_replace_all(ByteString str, ByteString old, ByteString new) {
    return bstring_replace(str, old, new, -1);
}

// bstring_reverse returns the reversed string.
static ByteString bstring_reverse(ByteString str) {
    ByteString res = bstring_clone(str.bytes, str.length);
    char* bytes = (char*)res.bytes;
    for (size_t i = 0; i < str.length / 2; i++) {
        char r = bytes[i];
        bytes[i] = bytes[str.length - 1 - i];
        bytes[str.length - 1 - i] = r;
    }
    return res;
}

// bstring_trim_left trims whitespaces from the beginning of the string.
static ByteString bstring_trim_left(ByteString str) {
    if (str.length == 0) {
        return bstring_new();
    }
    size_t idx = 0;
    for (; idx < str.length; idx++) {
        if (!isspace(str.bytes[idx])) {
            break;
        }
    }
    return bstring_slice(str, idx, str.length);
}

// bstring_trim_right trims whitespaces from the end of the string.
static ByteString bstring_trim_right(ByteString str) {
    if (str.length == 0) {
        return bstring_new();
    }
    size_t idx = str.length - 1;
    for (; idx >= 0; idx--) {
        if (!isspace(str.bytes[idx])) {
            break;
        }
    }
    return bstring_slice(str, 0, idx + 1);
}

// bstring_trim trims whitespaces from the beginning and end of the string.
static ByteString bstring_trim(ByteString str) {
    if (str.length == 0) {
        return bstring_new();
    }
    size_t left = 0;
    for (; left < str.length; left++) {
        if (!isspace(str.bytes[left])) {
            break;
        }
    }
    size_t right = str.length - 1;
    for (; right >= 0; right--) {
        if (!isspace(str.bytes[right])) {
            break;
        }
    }
    return bstring_slice(str, left, right + 1);
}

// bstring_print prints the string to stdout.
static void bstring_print(ByteString str) {
    if (str.bytes == NULL) {
        printf("<null>\n");
        return;
    }
    printf("'%s' (len=%zu)\n", str.bytes, str.length);
}

struct bstring_ns bstring = {
    .new = bstring_new,
    .to_cstring = bstring_to_cstring,
    .from_cstring = bstring_from_cstring,
    .free = bstring_free,
    .at = bstring_at,
    .slice = bstring_slice,
    .substring = bstring_substring,
    .index = bstring_index,
    .last_index = bstring_last_index,
    .contains = bstring_contains,
    .equals = bstring_equals,
    .has_prefix = bstring_has_prefix,
    .has_suffix = bstring_has_suffix,
    .count = bstring_count,
    .split_part = bstring_split_part,
    .join = bstring_join,
    .concat = bstring_concat,
    .repeat = bstring_repeat,
    .replace = bstring_replace,
    .replace_all = bstring_replace_all,
    .reverse = bstring_reverse,
    .trim_left = bstring_trim_left,
    .trim_right = bstring_trim_right,
    .trim = bstring_trim,
    .print = bstring_print,
};

// ---------------------------------
// src/text/extension.c
// ---------------------------------
// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// SQLite extension for working with text.

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

SQLITE_EXTENSION_INIT3


#pragma region Substrings

// Extracts a substring starting at the `start` position (1-based).
// text_substring(str, start)
// [pg-compatible] substr(string, start)
static void text_substring2(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);

    const char* src = (char*)sqlite3_value_text(argv[0]);
    if (src == NULL) {
        sqlite3_result_null(context);
        return;
    }

    if (sqlite3_value_type(argv[1]) != SQLITE_INTEGER) {
        sqlite3_result_error(context, "start parameter should be integer", -1);
        return;
    }
    int start = sqlite3_value_int(argv[1]);

    // convert to 0-based index
    // postgres-compatible: treat negative index as zero
    start = start > 0 ? start - 1 : 0;

    RuneString s_src = rstring.from_cstring(src);
    RuneString s_res = rstring.slice(s_src, start, s_src.length);
    char* res = rstring.to_cstring(s_res);
    sqlite3_result_text(context, res, -1, free);
    rstring.free(s_src);
    rstring.free(s_res);
}

// Extracts a substring of `length` characters starting at the `start` position (1-based).
// text_substring(str, start, length)
// [pg-compatible] substr(string, start, count)
static void text_substring3(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 3);

    const char* src = (char*)sqlite3_value_text(argv[0]);
    if (src == NULL) {
        sqlite3_result_null(context);
        return;
    }

    if (sqlite3_value_type(argv[1]) != SQLITE_INTEGER) {
        sqlite3_result_error(context, "start parameter should be integer", -1);
        return;
    }
    int start = sqlite3_value_int(argv[1]);

    if (sqlite3_value_type(argv[2]) != SQLITE_INTEGER) {
        sqlite3_result_error(context, "length parameter should be integer", -1);
        return;
    }
    int length = sqlite3_value_int(argv[2]);
    if (length < 0) {
        sqlite3_result_error(context, "length parameter should >= 0", -1);
        return;
    }

    // convert to 0-based index
    start -= 1;
    // postgres-compatible: treat negative start as 0, but shorten the length accordingly
    if (start < 0) {
        length += start;
        start = 0;
    }

    // zero-length substring
    if (length <= 0) {
        sqlite3_result_text(context, "", -1, SQLITE_TRANSIENT);
        return;
    }

    RuneString s_src = rstring.from_cstring(src);

    // postgres-compatible: the substring cannot be longer the the original string
    if ((size_t)length > s_src.length) {
        length = s_src.length;
    }

    RuneString s_res = rstring.substring(s_src, start, length);
    char* res = rstring.to_cstring(s_res);
    sqlite3_result_text(context, res, -1, free);
    rstring.free(s_src);
    rstring.free(s_res);
}

// Extracts a substring starting at the `start` position (1-based).
// text_slice(str, start)
static void text_slice2(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);

    const char* src = (char*)sqlite3_value_text(argv[0]);
    if (src == NULL) {
        sqlite3_result_null(context);
        return;
    }

    if (sqlite3_value_type(argv[1]) != SQLITE_INTEGER) {
        sqlite3_result_error(context, "start parameter should be integer", -1);
        return;
    }
    int start = sqlite3_value_int(argv[1]);

    // convert to 0-based index
    start = start > 0 ? start - 1 : start;

    RuneString s_src = rstring.from_cstring(src);

    // python-compatible: treat negative index larger than the length of the string as zero
    // and return the original string
    if (start < -(int)s_src.length) {
        sqlite3_result_text(context, src, -1, SQLITE_TRANSIENT);
        rstring.free(s_src);
        return;
    }

    RuneString s_res = rstring.slice(s_src, start, s_src.length);
    char* res = rstring.to_cstring(s_res);
    sqlite3_result_text(context, res, -1, free);
    rstring.free(s_src);
    rstring.free(s_res);
}

// Extracts a substring from `start` position inclusive to `end` position non-inclusive (1-based).
// text_slice(str, start, end)
static void text_slice3(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 3);

    const char* src = (char*)sqlite3_value_text(argv[0]);
    if (src == NULL) {
        sqlite3_result_null(context);
        return;
    }

    if (sqlite3_value_type(argv[1]) != SQLITE_INTEGER) {
        sqlite3_result_error(context, "start parameter should be integer", -1);
        return;
    }
    int start = sqlite3_value_int(argv[1]);
    // convert to 0-based index
    start = start > 0 ? start - 1 : start;

    if (sqlite3_value_type(argv[2]) != SQLITE_INTEGER) {
        sqlite3_result_error(context, "end parameter should be integer", -1);
        return;
    }
    int end = sqlite3_value_int(argv[2]);
    // convert to 0-based index
    end = end > 0 ? end - 1 : end;

    RuneString s_src = rstring.from_cstring(src);
    RuneString s_res = rstring.slice(s_src, start, end);
    char* res = rstring.to_cstring(s_res);
    sqlite3_result_text(context, res, -1, free);
    rstring.free(s_src);
    rstring.free(s_res);
}

// Extracts a substring of `length` characters from the beginning of the string.
// For `length < 0`, extracts all but the last `|length|` characters.
// text_left(str, length)
// [pg-compatible] left(string, n)
static void text_left(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);

    const char* src = (char*)sqlite3_value_text(argv[0]);
    if (src == NULL) {
        sqlite3_result_null(context);
        return;
    }

    if (sqlite3_value_type(argv[1]) != SQLITE_INTEGER) {
        sqlite3_result_error(context, "length parameter should be integer", -1);
        return;
    }
    int length = sqlite3_value_int(argv[1]);

    RuneString s_src = rstring.from_cstring(src);
    if (length < 0) {
        length = s_src.length + length;
        length = length >= 0 ? length : 0;
    }
    RuneString s_res = rstring.substring(s_src, 0, length);
    char* res = rstring.to_cstring(s_res);
    sqlite3_result_text(context, res, -1, free);
    rstring.free(s_src);
    rstring.free(s_res);
}

// Extracts a substring of `length` characters from the end of the string.
// For `length < 0`, extracts all but the first `|length|` characters.
// text_right(str, length)
// [pg-compatible] right(string, n)
static void text_right(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);

    const char* src = (char*)sqlite3_value_text(argv[0]);
    if (src == NULL) {
        sqlite3_result_null(context);
        return;
    }

    if (sqlite3_value_type(argv[1]) != SQLITE_INTEGER) {
        sqlite3_result_error(context, "length parameter should be integer", -1);
        return;
    }
    int length = sqlite3_value_int(argv[1]);

    RuneString s_src = rstring.from_cstring(src);

    length = (length < 0) ? (int)s_src.length + length : length;
    int start = (int)s_src.length - length;
    start = start < 0 ? 0 : start;

    RuneString s_res = rstring.substring(s_src, start, length);
    char* res = rstring.to_cstring(s_res);
    sqlite3_result_text(context, res, -1, free);
    rstring.free(s_src);
    rstring.free(s_res);
}

#pragma endregion

#pragma region Search and match

// Returns the first index of the substring in the original string.
// text_index(str, other)
// [pg-compatible] strpos(string, substring)
static void text_index(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);

    const char* src = (char*)sqlite3_value_text(argv[0]);
    if (src == NULL) {
        sqlite3_result_null(context);
        return;
    }

    const char* other = (char*)sqlite3_value_text(argv[1]);
    if (other == NULL) {
        sqlite3_result_null(context);
        return;
    }

    RuneString s_src = rstring.from_cstring(src);
    RuneString s_other = rstring.from_cstring(other);
    int idx = rstring.index(s_src, s_other);
    sqlite3_result_int64(context, idx + 1);
    rstring.free(s_src);
    rstring.free(s_other);
}

// Returns the last index of the substring in the original string.
// text_last_index(str, other)
static void text_last_index(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);

    const char* src = (char*)sqlite3_value_text(argv[0]);
    if (src == NULL) {
        sqlite3_result_null(context);
        return;
    }

    const char* other = (char*)sqlite3_value_text(argv[1]);
    if (other == NULL) {
        sqlite3_result_null(context);
        return;
    }

    RuneString s_src = rstring.from_cstring(src);
    RuneString s_other = rstring.from_cstring(other);
    int idx = rstring.last_index(s_src, s_other);
    sqlite3_result_int64(context, idx + 1);
    rstring.free(s_src);
    rstring.free(s_other);
}

// Checks if the string contains the substring.
// text_contains(str, other)
static void text_contains(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);

    const char* src = (char*)sqlite3_value_text(argv[0]);
    if (src == NULL) {
        sqlite3_result_null(context);
        return;
    }

    const char* other = (char*)sqlite3_value_text(argv[1]);
    if (other == NULL) {
        sqlite3_result_null(context);
        return;
    }

    ByteString s_src = bstring.from_cstring(src, sqlite3_value_bytes(argv[0]));
    ByteString s_other = bstring.from_cstring(other, sqlite3_value_bytes(argv[1]));
    bool found = bstring.contains(s_src, s_other);
    sqlite3_result_int(context, found);
    bstring.free(s_src);
    bstring.free(s_other);
}

// Checks if the string starts with the substring.
// text_has_prefix(str, other)
// [pg-compatible] starts_with(string, prefix)
static void text_has_prefix(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);

    const char* src = (char*)sqlite3_value_text(argv[0]);
    if (src == NULL) {
        sqlite3_result_null(context);
        return;
    }

    const char* other = (char*)sqlite3_value_text(argv[1]);
    if (other == NULL) {
        sqlite3_result_null(context);
        return;
    }

    ByteString s_src = bstring.from_cstring(src, sqlite3_value_bytes(argv[0]));
    ByteString s_other = bstring.from_cstring(other, sqlite3_value_bytes(argv[1]));
    bool found = bstring.has_prefix(s_src, s_other);
    sqlite3_result_int(context, found);
    bstring.free(s_src);
    bstring.free(s_other);
}

// Checks if the string ends with the substring.
// text_has_suffix(str, other)
static void text_has_suffix(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);

    const char* src = (char*)sqlite3_value_text(argv[0]);
    if (src == NULL) {
        sqlite3_result_null(context);
        return;
    }

    const char* other = (char*)sqlite3_value_text(argv[1]);
    if (other == NULL) {
        sqlite3_result_null(context);
        return;
    }

    ByteString s_src = bstring.from_cstring(src, sqlite3_value_bytes(argv[0]));
    ByteString s_other = bstring.from_cstring(other, sqlite3_value_bytes(argv[1]));
    bool found = bstring.has_suffix(s_src, s_other);
    sqlite3_result_int(context, found);
    bstring.free(s_src);
    bstring.free(s_other);
}

// Counts how many times the substring is contained in the original string.
// text_count(str, other)
static void text_count(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);

    const char* src = (char*)sqlite3_value_text(argv[0]);
    if (src == NULL) {
        sqlite3_result_null(context);
        return;
    }

    const char* other = (char*)sqlite3_value_text(argv[1]);
    if (other == NULL) {
        sqlite3_result_null(context);
        return;
    }

    ByteString s_src = bstring.from_cstring(src, sqlite3_value_bytes(argv[0]));
    ByteString s_other = bstring.from_cstring(other, sqlite3_value_bytes(argv[1]));
    size_t count = bstring.count(s_src, s_other);
    sqlite3_result_int(context, count);
    bstring.free(s_src);
    bstring.free(s_other);
}

#pragma endregion

#pragma region Split and join

// Splits a string by a separator and returns the n-th part (counting from one).
// When n is negative, returns the |n|'th-from-last part.
// text_split(str, sep, n)
// [pg-compatible] split_part(string, delimiter, n)
static void text_split(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 3);

    const char* src = (char*)sqlite3_value_text(argv[0]);
    if (src == NULL) {
        sqlite3_result_null(context);
        return;
    }

    const char* sep = (const char*)sqlite3_value_text(argv[1]);
    if (sep == NULL) {
        sqlite3_result_null(context);
        return;
    }

    if (sqlite3_value_type(argv[2]) != SQLITE_INTEGER) {
        sqlite3_result_error(context, "part parameter should be integer", -1);
        return;
    }
    int part = sqlite3_value_int(argv[2]);
    // pg-compatible
    if (part == 0) {
        sqlite3_result_error(context, "part parameter should not be 0", -1);
        return;
    }
    // convert to 0-based index
    part = part > 0 ? part - 1 : part;

    ByteString s_src = bstring.from_cstring(src, strlen(src));
    ByteString s_sep = bstring.from_cstring(sep, strlen(sep));

    // count from the last part backwards
    if (part < 0) {
        int n_parts = bstring.count(s_src, s_sep) + 1;
        part = n_parts + part;
    }

    ByteString s_part = bstring.split_part(s_src, s_sep, part);
    sqlite3_result_text(context, s_part.bytes, -1, SQLITE_TRANSIENT);
    bstring.free(s_src);
    bstring.free(s_sep);
    bstring.free(s_part);
}

// Joins strings using the separator and returns the resulting string. Ignores nulls.
// text_join(sep, str, ...)
// [pg-compatible] concat_ws(sep, val1[, val2 [, ...]])
static void text_join(sqlite3_context* context, int argc, sqlite3_value** argv) {
    if (argc < 2) {
        sqlite3_result_error(context, "expected at least 2 parameters", -1);
        return;
    }

    // separator
    const char* sep = (char*)sqlite3_value_text(argv[0]);
    if (sep == NULL) {
        sqlite3_result_null(context);
        return;
    }
    ByteString s_sep = bstring.from_cstring(sep, sqlite3_value_bytes(argv[0]));

    // parts
    size_t n_parts = argc - 1;
    ByteString* s_parts = malloc(n_parts * sizeof(ByteString));
    if (s_parts == NULL) {
        sqlite3_result_null(context);
        return;
    }
    for (size_t i = 1, part_idx = 0; i < (size_t)argc; i++) {
        if (sqlite3_value_type(argv[i]) == SQLITE_NULL) {
            // ignore nulls
            n_parts--;
            continue;
        }
        const char* part = (char*)sqlite3_value_text(argv[i]);
        int part_len = sqlite3_value_bytes(argv[i]);
        s_parts[part_idx] = bstring.from_cstring(part, part_len);
        part_idx++;
    }

    // join parts with separator
    ByteString s_res = bstring.join(s_parts, n_parts, s_sep);
    const char* res = bstring.to_cstring(s_res);
    sqlite3_result_text(context, res, -1, SQLITE_TRANSIENT);
    bstring.free(s_sep);
    bstring.free(s_res);
    free(s_parts);
}

// Concatenates strings and returns the resulting string. Ignores nulls.
// text_concat(str, ...)
// [pg-compatible] concat(val1[, val2 [, ...]])
static void text_concat(sqlite3_context* context, int argc, sqlite3_value** argv) {
    if (argc < 1) {
        sqlite3_result_error(context, "expected at least 1 parameter", -1);
        return;
    }

    // parts
    size_t n_parts = argc;
    ByteString* s_parts = malloc(n_parts * sizeof(ByteString));
    if (s_parts == NULL) {
        sqlite3_result_null(context);
        return;
    }
    for (size_t i = 0, part_idx = 0; i < (size_t)argc; i++) {
        if (sqlite3_value_type(argv[i]) == SQLITE_NULL) {
            // ignore nulls
            n_parts--;
            continue;
        }
        const char* part = (char*)sqlite3_value_text(argv[i]);
        int part_len = sqlite3_value_bytes(argv[i]);
        s_parts[part_idx] = bstring.from_cstring(part, part_len);
        part_idx++;
    }

    // join parts
    ByteString s_res = bstring.concat(s_parts, n_parts);
    const char* res = bstring.to_cstring(s_res);
    sqlite3_result_text(context, res, -1, SQLITE_TRANSIENT);
    bstring.free(s_res);
    free(s_parts);
}

// Concatenates the string to itself a given number of times and returns the resulting string.
// text_repeat(str, count)
// [pg-compatible] repeat(string, number)
static void text_repeat(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);

    const char* src = (char*)sqlite3_value_text(argv[0]);
    if (src == NULL) {
        sqlite3_result_null(context);
        return;
    }

    if (sqlite3_value_type(argv[1]) != SQLITE_INTEGER) {
        sqlite3_result_error(context, "count parameter should be integer", -1);
        return;
    }
    int count = sqlite3_value_int(argv[1]);
    // pg-compatible: treat negative count as zero
    count = count >= 0 ? count : 0;

    ByteString s_src = bstring.from_cstring(src, sqlite3_value_bytes(argv[0]));
    ByteString s_res = bstring.repeat(s_src, count);
    const char* res = bstring.to_cstring(s_res);
    sqlite3_result_text(context, res, -1, SQLITE_TRANSIENT);
    bstring.free(s_src);
    bstring.free(s_res);
}

#pragma endregion

#pragma region Trim and pad

// Trims certain characters (spaces by default) from the beginning/end of the string.
// text_ltrim(str [,chars])
// text_rtrim(str [,chars])
// text_trim(str [,chars])
// [pg-compatible] ltrim(string [, characters])
// [pg-compatible] rtrim(string [, characters])
// [pg-compatible] btrim(string [, characters])
static void text_trim(sqlite3_context* context, int argc, sqlite3_value** argv) {
    if (argc != 1 && argc != 2) {
        sqlite3_result_error(context, "expected 1 or 2 parameters", -1);
        return;
    }

    const char* src = (char*)sqlite3_value_text(argv[0]);
    if (src == NULL) {
        sqlite3_result_null(context);
        return;
    }

    const char* chars = argc == 2 ? (char*)sqlite3_value_text(argv[1]) : " ";
    if (chars == NULL) {
        sqlite3_result_null(context);
        return;
    }

    RuneString (*trim_func)(RuneString, RuneString) = (void*)sqlite3_user_data(context);

    RuneString s_src = rstring.from_cstring(src);
    RuneString s_chars = rstring.from_cstring(chars);
    RuneString s_res = trim_func(s_src, s_chars);
    const char* res = rstring.to_cstring(s_res);
    sqlite3_result_text(context, res, -1, free);
    rstring.free(s_src);
    rstring.free(s_chars);
    rstring.free(s_res);
}

// Pads the string to the specified length by prepending/appending certain characters
// (spaces by default).
// text_lpad(str, length [,fill])
// text_rpad(str, length [,fill])
// [pg-compatible] lpad(string, length [, fill])
// [pg-compatible] rpad(string, length [, fill])
// (!) postgres does not support unicode strings in lpad/rpad, while this function does.
static void text_pad(sqlite3_context* context, int argc, sqlite3_value** argv) {
    if (argc != 2 && argc != 3) {
        sqlite3_result_error(context, "expected 2 or 3 parameters", -1);
        return;
    }

    const char* src = (char*)sqlite3_value_text(argv[0]);
    if (src == NULL) {
        sqlite3_result_null(context);
        return;
    }

    if (sqlite3_value_type(argv[1]) != SQLITE_INTEGER) {
        sqlite3_result_error(context, "length parameter should be integer", -1);
        return;
    }
    int length = sqlite3_value_int(argv[1]);
    // postgres-compatible: treat negative length as zero
    length = length < 0 ? 0 : length;

    const char* fill = argc == 3 ? (char*)sqlite3_value_text(argv[2]) : " ";
    if (fill == NULL) {
        sqlite3_result_null(context);
        return;
    }

    RuneString (*pad_func)(RuneString, size_t, RuneString) = (void*)sqlite3_user_data(context);

    RuneString s_src = rstring.from_cstring(src);
    RuneString s_fill = rstring.from_cstring(fill);
    RuneString s_res = pad_func(s_src, length, s_fill);
    const char* res = rstring.to_cstring(s_res);
    sqlite3_result_text(context, res, -1, free);
    rstring.free(s_src);
    rstring.free(s_fill);
    rstring.free(s_res);
}

#pragma endregion

#pragma region Other modifications

// Replaces all old substrings with new substrings in the original string.
// text_replace(str, old, new)
// [pg-compatible] replace(string, from, to)
static void text_replace_all(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 3);

    const char* src = (char*)sqlite3_value_text(argv[0]);
    if (src == NULL) {
        sqlite3_result_null(context);
        return;
    }

    const char* old = (char*)sqlite3_value_text(argv[1]);
    if (old == NULL) {
        sqlite3_result_null(context);
        return;
    }

    const char* new = (char*)sqlite3_value_text(argv[2]);
    if (new == NULL) {
        sqlite3_result_null(context);
        return;
    }

    ByteString s_src = bstring.from_cstring(src, sqlite3_value_bytes(argv[0]));
    ByteString s_old = bstring.from_cstring(old, sqlite3_value_bytes(argv[1]));
    ByteString s_new = bstring.from_cstring(new, sqlite3_value_bytes(argv[2]));
    ByteString s_res = bstring.replace_all(s_src, s_old, s_new);
    const char* res = bstring.to_cstring(s_res);
    sqlite3_result_text(context, res, -1, SQLITE_TRANSIENT);
    bstring.free(s_src);
    bstring.free(s_old);
    bstring.free(s_new);
    bstring.free(s_res);
}

// Replaces old substrings with new substrings in the original string,
// but not more than `count` times.
// text_replace(str, old, new, count)
static void text_replace(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 4);

    const char* src = (char*)sqlite3_value_text(argv[0]);
    if (src == NULL) {
        sqlite3_result_null(context);
        return;
    }

    const char* old = (char*)sqlite3_value_text(argv[1]);
    if (old == NULL) {
        sqlite3_result_null(context);
        return;
    }

    const char* new = (char*)sqlite3_value_text(argv[2]);
    if (new == NULL) {
        sqlite3_result_null(context);
        return;
    }

    if (sqlite3_value_type(argv[3]) != SQLITE_INTEGER) {
        sqlite3_result_error(context, "count parameter should be integer", -1);
        return;
    }
    int count = sqlite3_value_int(argv[3]);
    // treat negative count as zero
    count = count < 0 ? 0 : count;

    ByteString s_src = bstring.from_cstring(src, sqlite3_value_bytes(argv[0]));
    ByteString s_old = bstring.from_cstring(old, sqlite3_value_bytes(argv[1]));
    ByteString s_new = bstring.from_cstring(new, sqlite3_value_bytes(argv[2]));
    ByteString s_res = bstring.replace(s_src, s_old, s_new, count);
    const char* res = bstring.to_cstring(s_res);
    sqlite3_result_text(context, res, -1, SQLITE_TRANSIENT);
    bstring.free(s_src);
    bstring.free(s_old);
    bstring.free(s_new);
    bstring.free(s_res);
}

// Replaces each string character that matches a character in the `from` set
// with the corresponding character in the `to` set. If `from` is longer than `to`,
// occurrences of the extra characters in `from` are deleted.
// text_translate(str, from, to)
// [pg-compatible] translate(string, from, to)
// (!) postgres does not support unicode strings in translate, while this function does.
static void text_translate(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 3);

    const char* src = (char*)sqlite3_value_text(argv[0]);
    if (src == NULL) {
        sqlite3_result_null(context);
        return;
    }

    const char* from = (char*)sqlite3_value_text(argv[1]);
    if (from == NULL) {
        sqlite3_result_null(context);
        return;
    }

    const char* to = (char*)sqlite3_value_text(argv[2]);
    if (to == NULL) {
        sqlite3_result_null(context);
        return;
    }

    RuneString s_src = rstring.from_cstring(src);
    RuneString s_from = rstring.from_cstring(from);
    RuneString s_to = rstring.from_cstring(to);
    RuneString s_res = rstring.translate(s_src, s_from, s_to);
    char* res = rstring.to_cstring(s_res);
    sqlite3_result_text(context, res, -1, free);
    rstring.free(s_src);
    rstring.free(s_from);
    rstring.free(s_to);
    rstring.free(s_res);
}

// Reverses the order of the characters in the string.
// text_reverse(str)
// [pg-compatible] reverse(text)
// (!) postgres does not support unicode strings in reverse, while this function does.
static void text_reverse(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);

    const char* src = (char*)sqlite3_value_text(argv[0]);
    if (src == NULL) {
        sqlite3_result_null(context);
        return;
    }

    RuneString s_src = rstring.from_cstring(src);
    RuneString s_res = rstring.reverse(s_src);
    char* res = rstring.to_cstring(s_res);
    sqlite3_result_text(context, res, -1, free);
    rstring.free(s_src);
    rstring.free(s_res);
}

#pragma endregion

#pragma region Properties

// Returns the number of characters in the string.
// text_length(str)
// [pg-compatible] length(text)
// [pg-compatible] char_length(text)
// [pg-compatible] character_length(text)
static void text_length(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);

    const char* src = (char*)sqlite3_value_text(argv[0]);
    if (src == NULL) {
        sqlite3_result_null(context);
        return;
    }

    RuneString s_src = rstring.from_cstring(src);
    sqlite3_result_int64(context, s_src.length);
    rstring.free(s_src);
}

// Returns the number of bytes in the string.
// text_size(str)
// [pg-compatible] octet_length(text)
static void text_size(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);

    const char* src = (char*)sqlite3_value_text(argv[0]);
    if (src == NULL) {
        sqlite3_result_null(context);
        return;
    }

    sqlite3_result_int64(context, sqlite3_value_bytes(argv[0]));
}

// Returns the number of bits in the string.
// text_bitsize(str)
// [pg-compatible] bit_length(text)
static void text_bit_size(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);

    const char* src = (char*)sqlite3_value_text(argv[0]);
    if (src == NULL) {
        sqlite3_result_null(context);
        return;
    }

    int size = sqlite3_value_bytes(argv[0]);
    sqlite3_result_int64(context, 8 * size);
}

#pragma endregion

int text_init(sqlite3* db) {
    static const int flags = SQLITE_UTF8 | SQLITE_INNOCUOUS | SQLITE_DETERMINISTIC;

    // substrings
    sqlite3_create_function(db, "text_substring", 2, flags, 0, text_substring2, 0, 0);
    sqlite3_create_function(db, "text_substring", 3, flags, 0, text_substring3, 0, 0);
    sqlite3_create_function(db, "text_slice", 2, flags, 0, text_slice2, 0, 0);
    sqlite3_create_function(db, "text_slice", 3, flags, 0, text_slice3, 0, 0);
    sqlite3_create_function(db, "text_left", 2, flags, 0, text_left, 0, 0);
    sqlite3_create_function(db, "left", 2, flags, 0, text_left, 0, 0);
    sqlite3_create_function(db, "text_right", 2, flags, 0, text_right, 0, 0);
    sqlite3_create_function(db, "right", 2, flags, 0, text_right, 0, 0);

    // search and match
    sqlite3_create_function(db, "text_index", 2, flags, 0, text_index, 0, 0);
    sqlite3_create_function(db, "strpos", 2, flags, 0, text_index, 0, 0);
    sqlite3_create_function(db, "text_last_index", 2, flags, 0, text_last_index, 0, 0);
    sqlite3_create_function(db, "text_contains", 2, flags, 0, text_contains, 0, 0);
    sqlite3_create_function(db, "text_has_prefix", 2, flags, 0, text_has_prefix, 0, 0);
    sqlite3_create_function(db, "starts_with", 2, flags, 0, text_has_prefix, 0, 0);
    sqlite3_create_function(db, "text_has_suffix", 2, flags, 0, text_has_suffix, 0, 0);
    sqlite3_create_function(db, "text_count", 2, flags, 0, text_count, 0, 0);

    // split and join
    sqlite3_create_function(db, "text_split", 3, flags, 0, text_split, 0, 0);
    sqlite3_create_function(db, "split_part", 3, flags, 0, text_split, 0, 0);
    sqlite3_create_function(db, "text_join", -1, flags, 0, text_join, 0, 0);
    sqlite3_create_function(db, "concat_ws", -1, flags, 0, text_join, 0, 0);
    sqlite3_create_function(db, "text_concat", -1, flags, 0, text_concat, 0, 0);
    sqlite3_create_function(db, "concat", -1, flags, 0, text_concat, 0, 0);
    sqlite3_create_function(db, "text_repeat", 2, flags, 0, text_repeat, 0, 0);
    sqlite3_create_function(db, "repeat", 2, flags, 0, text_repeat, 0, 0);

    // trim and pad
    sqlite3_create_function(db, "text_ltrim", -1, flags, rstring.trim_left, text_trim, 0, 0);
    sqlite3_create_function(db, "ltrim", -1, flags, rstring.trim_left, text_trim, 0, 0);
    sqlite3_create_function(db, "text_rtrim", -1, flags, rstring.trim_right, text_trim, 0, 0);
    sqlite3_create_function(db, "rtrim", -1, flags, rstring.trim_right, text_trim, 0, 0);
    sqlite3_create_function(db, "text_trim", -1, flags, rstring.trim, text_trim, 0, 0);
    sqlite3_create_function(db, "btrim", -1, flags, rstring.trim, text_trim, 0, 0);
    sqlite3_create_function(db, "text_lpad", -1, flags, rstring.pad_left, text_pad, 0, 0);
    sqlite3_create_function(db, "lpad", -1, flags, rstring.pad_left, text_pad, 0, 0);
    sqlite3_create_function(db, "text_rpad", -1, flags, rstring.pad_right, text_pad, 0, 0);
    sqlite3_create_function(db, "rpad", -1, flags, rstring.pad_right, text_pad, 0, 0);

    // other modifications
    sqlite3_create_function(db, "text_replace", 3, flags, 0, text_replace_all, 0, 0);
    sqlite3_create_function(db, "text_replace", 4, flags, 0, text_replace, 0, 0);
    sqlite3_create_function(db, "text_translate", 3, flags, 0, text_translate, 0, 0);
    sqlite3_create_function(db, "translate", 3, flags, 0, text_translate, 0, 0);
    sqlite3_create_function(db, "text_reverse", 1, flags, 0, text_reverse, 0, 0);
    sqlite3_create_function(db, "reverse", 1, flags, 0, text_reverse, 0, 0);

    // properties
    sqlite3_create_function(db, "text_length", 1, flags, 0, text_length, 0, 0);
    sqlite3_create_function(db, "char_length", 1, flags, 0, text_length, 0, 0);
    sqlite3_create_function(db, "character_length", 1, flags, 0, text_length, 0, 0);
    sqlite3_create_function(db, "text_size", 1, flags, 0, text_size, 0, 0);
    sqlite3_create_function(db, "octet_length", 1, flags, 0, text_size, 0, 0);
    sqlite3_create_function(db, "text_bitsize", 1, flags, 0, text_bit_size, 0, 0);
    sqlite3_create_function(db, "bit_length", 1, flags, 0, text_bit_size, 0, 0);

    return SQLITE_OK;
}

// ---------------------------------
// src/text/rstring.c
// ---------------------------------
// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Rune (UTF-8) string data structure.

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// utf8_length returns the number of utf-8 characters in a string.
static size_t utf8_length(const char* str) {
    size_t length = 0;

    while (*str != '\0') {
        if (0xf0 == (0xf8 & *str)) {
            // 4-byte utf8 code point (began with 0b11110xxx)
            str += 4;
        } else if (0xe0 == (0xf0 & *str)) {
            // 3-byte utf8 code point (began with 0b1110xxxx)
            str += 3;
        } else if (0xc0 == (0xe0 & *str)) {
            // 2-byte utf8 code point (began with 0b110xxxxx)
            str += 2;
        } else {  // if (0x00 == (0x80 & *s)) {
            // 1-byte ascii (began with 0b0xxxxxxx)
            str += 1;
        }

        // no matter the bytes we marched s forward by, it was
        // only 1 utf8 codepoint
        length++;
    }

    return length;
}

// rstring_new creates an empty string.
static RuneString rstring_new(void) {
    RuneString str = {.runes = NULL, .length = 0, .size = 0, .owning = true};
    return str;
}

// rstring_from_runes creates a new string from an array of utf-8 characters.
// `owning` indicates whether the string owns the array and should free the memory when destroyed.
static RuneString rstring_from_runes(const int32_t* const runes, size_t length, bool owning) {
    RuneString str = {
        .runes = runes, .length = length, .size = length * sizeof(int32_t), .owning = owning};
    return str;
}

// rstring_from_cstring creates a new string from a zero-terminated C string.
static RuneString rstring_from_cstring(const char* const utf8str) {
    size_t length = utf8_length(utf8str);
    int32_t* runes = length > 0 ? runes_from_cstring(utf8str, length) : NULL;
    return rstring_from_runes(runes, length, true);
}

// rstring_to_cstring converts the string to a zero-terminated C string.
static char* rstring_to_cstring(RuneString str) {
    return runes_to_cstring(str.runes, str.length);
}

// rstring_free destroys the string, freeing resources if necessary.
static void rstring_free(RuneString str) {
    if (str.owning && str.runes != NULL) {
        free((void*)str.runes);
    }
}

// rstring_at returns a character by its index in the string.
static int32_t rstring_at(RuneString str, size_t idx) {
    if (str.length == 0) {
        return 0;
    }
    if (idx < 0 || idx >= str.length) {
        return 0;
    };
    return str.runes[idx];
}

// rstring_slice returns a slice of the string,
// from the `start` index (inclusive) to the `end` index (non-inclusive).
// Negative `start` and `end` values count from the end of the string.
static RuneString rstring_slice(RuneString str, int start, int end) {
    if (str.length == 0) {
        return rstring_new();
    }

    // adjusted start index
    start = start < 0 ? str.length + start : start;
    // python-compatible: treat negative start index larger than the length of the string as zero
    start = start < 0 ? 0 : start;
    // adjusted start index should be less the the length of the string
    if (start >= (int)str.length) {
        return rstring_new();
    }

    // adjusted end index
    end = end < 0 ? str.length + end : end;
    // python-compatible: treat end index larger than the length of the string
    // as equal to the length
    end = end > (int)str.length ? (int)str.length : end;
    // adjusted end index should be >= 0
    if (end < 0) {
        return rstring_new();
    }

    // adjusted start index should be less than adjusted end index
    if (start >= end) {
        return rstring_new();
    }

    int32_t* at = (int32_t*)str.runes + start;
    size_t length = end - start;
    RuneString slice = rstring_from_runes(at, length, false);
    return slice;
}

// rstring_substring returns a substring of `length` characters,
// starting from the `start` index.
static RuneString rstring_substring(RuneString str, size_t start, size_t length) {
    if (length > str.length - start) {
        length = str.length - start;
    }
    return rstring_slice(str, start, start + length);
}

// rstring_contains_after checks if the other string is a substring of the original string,
// starting at the `start` index.
static bool rstring_contains_after(RuneString str, RuneString other, size_t start) {
    if (start + other.length > str.length) {
        return false;
    }
    for (size_t idx = 0; idx < other.length; idx++) {
        if (str.runes[start + idx] != other.runes[idx]) {
            return false;
        }
    }
    return true;
}

// rstring_index_char returns the first index of the character in the string
// after the `start` index, inclusive.
static int rstring_index_char(RuneString str, int32_t rune, size_t start) {
    for (size_t idx = start; idx < str.length; idx++) {
        if (str.runes[idx] == rune) {
            return idx;
        }
    }
    return -1;
}

// rstring_index_char returns the last index of the character in the string
// before the `end` index, inclusive.
static int rstring_last_index_char(RuneString str, int32_t rune, size_t end) {
    if (end >= str.length) {
        return -1;
    }
    for (int idx = end; idx >= 0; idx--) {
        if (str.runes[idx] == rune) {
            return idx;
        }
    }
    return -1;
}

// rstring_index_after returns the index of the substring in the original string
// after the `start` index, inclusive.
static int rstring_index_after(RuneString str, RuneString other, size_t start) {
    if (other.length == 0) {
        return start;
    }
    if (str.length == 0 || other.length > str.length) {
        return -1;
    }

    size_t cur_idx = start;
    while (cur_idx < str.length) {
        int match_idx = rstring_index_char(str, other.runes[0], cur_idx);
        if (match_idx == -1) {
            return match_idx;
        }
        if (rstring_contains_after(str, other, match_idx)) {
            return match_idx;
        }
        cur_idx = match_idx + 1;
    }
    return -1;
}

// rstring_index returns the first index of the substring in the original string.
static int rstring_index(RuneString str, RuneString other) {
    return rstring_index_after(str, other, 0);
}

// rstring_last_index returns the last index of the substring in the original string.
static int rstring_last_index(RuneString str, RuneString other) {
    if (other.length == 0) {
        return str.length - 1;
    }
    if (str.length == 0 || other.length > str.length) {
        return -1;
    }

    int cur_idx = str.length - 1;
    while (cur_idx >= 0) {
        int match_idx = rstring_last_index_char(str, other.runes[0], cur_idx);
        if (match_idx == -1) {
            return match_idx;
        }
        if (rstring_contains_after(str, other, match_idx)) {
            return match_idx;
        }
        cur_idx = match_idx - 1;
    }

    return -1;
}

// rstring_translate replaces each string character that matches a character in the `from` set with
// the corresponding character in the `to` set. If `from` is longer than `to`, occurrences of the
// extra characters in `from` are deleted.
static RuneString rstring_translate(RuneString str, RuneString from, RuneString to) {
    if (str.length == 0) {
        return rstring_new();
    }

    // empty mapping, return the original string
    if (from.length == 0) {
        return rstring_from_runes(str.runes, str.length, false);
    }

    // resulting string can be no longer than the original one
    int32_t* runes = calloc(str.length, sizeof(int32_t));
    if (runes == NULL) {
        return rstring_new();
    }

    // but it may be shorter, so we should track its length separately
    size_t length = 0;
    // perform the translation
    for (size_t idx = 0; idx < str.length; idx++) {
        size_t k = 0;
        // map idx-th character in str `from` -> `to`
        for (; k < from.length && k < to.length; k++) {
            if (str.runes[idx] == from.runes[k]) {
                runes[length] = to.runes[k];
                length++;
                break;
            }
        }
        // if `from` is longer than `to`, ingore idx-th character found in `from`
        bool ignore = false;
        for (; k < from.length; k++) {
            if (str.runes[idx] == from.runes[k]) {
                ignore = true;
                break;
            }
        }
        // else copy idx-th character as is
        if (!ignore) {
            runes[length] = str.runes[idx];
            length++;
        }
    }

    return rstring_from_runes(runes, length, true);
}

// rstring_reverse returns the reversed string.
static RuneString rstring_reverse(RuneString str) {
    int32_t* runes = (int32_t*)str.runes;
    for (size_t i = 0; i < str.length / 2; i++) {
        int32_t r = runes[i];
        runes[i] = runes[str.length - 1 - i];
        runes[str.length - 1 - i] = r;
    }
    RuneString res = rstring_from_runes(runes, str.length, false);
    return res;
}

// rstring_trim_left trims certain characters from the beginning of the string.
static RuneString rstring_trim_left(RuneString str, RuneString chars) {
    if (str.length == 0) {
        return rstring_new();
    }
    size_t idx = 0;
    for (; idx < str.length; idx++) {
        if (rstring_index_char(chars, str.runes[idx], 0) == -1) {
            break;
        }
    }
    return rstring_slice(str, idx, str.length);
}

// rstring_trim_right trims certain characters from the end of the string.
static RuneString rstring_trim_right(RuneString str, RuneString chars) {
    if (str.length == 0) {
        return rstring_new();
    }
    int idx = str.length - 1;
    for (; idx >= 0; idx--) {
        if (rstring_index_char(chars, str.runes[idx], 0) == -1) {
            break;
        }
    }
    return rstring_slice(str, 0, idx + 1);
}

// rstring_trim trims certain characters from the beginning and end of the string.
static RuneString rstring_trim(RuneString str, RuneString chars) {
    if (str.length == 0) {
        return rstring_new();
    }
    size_t left = 0;
    for (; left < str.length; left++) {
        if (rstring_index_char(chars, str.runes[left], 0) == -1) {
            break;
        }
    }
    int right = str.length - 1;
    for (; right >= 0; right--) {
        if (rstring_index_char(chars, str.runes[right], 0) == -1) {
            break;
        }
    }
    return rstring_slice(str, left, right + 1);
}

// rstring_pad_left pads the string to the specified length by prepending `fill` characters.
// If the string is already longer than the specified length, it is truncated on the right.
RuneString rstring_pad_left(RuneString str, size_t length, RuneString fill) {
    if (str.length >= length) {
        // If the string is already longer than length, return a truncated version of the string
        return rstring_substring(str, 0, length);
    }

    if (fill.length == 0) {
        // If the fill string is empty, return the original string
        return rstring_from_runes(str.runes, str.length, false);
    }

    // Calculate the number of characters to pad
    size_t pad_langth = length - str.length;

    // Allocate memory for the padded string
    size_t new_size = (str.length + pad_langth) * sizeof(int32_t);
    int32_t* new_runes = malloc(new_size);
    if (new_runes == NULL) {
        return rstring_new();
    }

    // Copy the fill characters to the beginning of the new string
    for (size_t i = 0; i < pad_langth; i++) {
        new_runes[i] = fill.runes[i % fill.length];
    }

    // Copy the original string to the end of the new string
    memcpy(&new_runes[pad_langth], str.runes, str.size);

    // Return the new string
    RuneString new_str = rstring_from_runes(new_runes, length, true);
    return new_str;
}

// rstring_pad_right pads the string to the specified length by appending `fill` characters.
// If the string is already longer than the specified length, it is truncated on the right.
RuneString rstring_pad_right(RuneString str, size_t length, RuneString fill) {
    if (str.length >= length) {
        // If the string is already longer than length, return a truncated version of the string
        return rstring_substring(str, 0, length);
    }

    if (fill.length == 0) {
        // If the fill string is empty, return the original string
        return rstring_from_runes(str.runes, str.length, false);
    }

    // Calculate the number of characters to pad
    size_t pad_length = length - str.length;

    // Allocate memory for the padded string
    size_t new_size = (str.length + pad_length) * sizeof(int32_t);
    int32_t* new_runes = malloc(new_size);
    if (new_runes == NULL) {
        return rstring_new();
    }

    // Copy the original string to the beginning of the new string
    memcpy(new_runes, str.runes, str.size);

    // Copy the fill characters to the end of the new string
    for (size_t i = str.length; i < length; i++) {
        new_runes[i] = fill.runes[(i - str.length) % fill.length];
    }

    // Return the new string
    RuneString new_str = rstring_from_runes(new_runes, length, true);
    return new_str;
}

// rstring_print prints the string to stdout.
static void rstring_print(RuneString str) {
    if (str.length == 0) {
        printf("'' (len=0)\n");
        return;
    }
    printf("'");
    for (size_t i = 0; i < str.length; i++) {
        printf("%08x ", str.runes[i]);
    }
    printf("' (len=%zu)", str.length);
    printf("\n");
}

struct rstring_ns rstring = {
    .new = rstring_new,
    .from_cstring = rstring_from_cstring,
    .to_cstring = rstring_to_cstring,
    .free = rstring_free,
    .at = rstring_at,
    .index = rstring_index,
    .last_index = rstring_last_index,
    .slice = rstring_slice,
    .substring = rstring_substring,
    .translate = rstring_translate,
    .reverse = rstring_reverse,
    .trim_left = rstring_trim_left,
    .trim_right = rstring_trim_right,
    .trim = rstring_trim,
    .pad_left = rstring_pad_left,
    .pad_right = rstring_pad_right,
    .print = rstring_print,
};

// ---------------------------------
// src/text/runes.c
// ---------------------------------
// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// UTF-8 characters (runes) <-> C string conversions.

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// utf8_cat_rune prints the rune to the string.
static char* utf8_cat_rune(char* str, int32_t rune) {
    if (0 == ((int32_t)0xffffff80 & rune)) {
        // 1-byte/7-bit ascii
        // (0b0xxxxxxx)
        str[0] = (char)rune;
        str += 1;
    } else if (0 == ((int32_t)0xfffff800 & rune)) {
        // 2-byte/11-bit utf8 code point
        // (0b110xxxxx 0b10xxxxxx)
        str[0] = (char)(0xc0 | (char)((rune >> 6) & 0x1f));
        str[1] = (char)(0x80 | (char)(rune & 0x3f));
        str += 2;
    } else if (0 == ((int32_t)0xffff0000 & rune)) {
        // 3-byte/16-bit utf8 code point
        // (0b1110xxxx 0b10xxxxxx 0b10xxxxxx)
        str[0] = (char)(0xe0 | (char)((rune >> 12) & 0x0f));
        str[1] = (char)(0x80 | (char)((rune >> 6) & 0x3f));
        str[2] = (char)(0x80 | (char)(rune & 0x3f));
        str += 3;
    } else {  // if (0 == ((int)0xffe00000 & rune)) {
        // 4-byte/21-bit utf8 code point
        // (0b11110xxx 0b10xxxxxx 0b10xxxxxx 0b10xxxxxx)
        str[0] = (char)(0xf0 | (char)((rune >> 18) & 0x07));
        str[1] = (char)(0x80 | (char)((rune >> 12) & 0x3f));
        str[2] = (char)(0x80 | (char)((rune >> 6) & 0x3f));
        str[3] = (char)(0x80 | (char)(rune & 0x3f));
        str += 4;
    }
    return str;
}

// utf8iter iterates over the C string, producing runes.
typedef struct {
    const char* str;
    int32_t rune;
    size_t length;
    size_t index;
    bool eof;
} utf8iter;

// utf8iter_new creates a new iterator.
static utf8iter* utf8iter_new(const char* str, size_t length) {
    utf8iter* iter = malloc(sizeof(utf8iter));
    if (iter == NULL) {
        return NULL;
    }
    iter->str = str;
    iter->length = length;
    iter->index = 0;
    iter->eof = length == 0;
    return iter;
}

// utf8iter_next advances the iterator to the next rune and returns it.
static int32_t utf8iter_next(utf8iter* iter) {
    assert(iter != NULL);

    if (iter->eof) {
        return 0;
    }

    const char* str = iter->str;
    if (0xf0 == (0xf8 & str[0])) {
        // 4 byte utf8 codepoint
        iter->rune = ((0x07 & str[0]) << 18) | ((0x3f & str[1]) << 12) | ((0x3f & str[2]) << 6) |
                     (0x3f & str[3]);
        iter->str += 4;
    } else if (0xe0 == (0xf0 & str[0])) {
        // 3 byte utf8 codepoint
        iter->rune = ((0x0f & str[0]) << 12) | ((0x3f & str[1]) << 6) | (0x3f & str[2]);
        iter->str += 3;
    } else if (0xc0 == (0xe0 & str[0])) {
        // 2 byte utf8 codepoint
        iter->rune = ((0x1f & str[0]) << 6) | (0x3f & str[1]);
        iter->str += 2;
    } else {
        // 1 byte utf8 codepoint otherwise
        iter->rune = str[0];
        iter->str += 1;
    }

    iter->index += 1;

    if (iter->index == iter->length) {
        iter->eof = true;
    }

    return iter->rune;
}

// runes_from_cstring creates an array of runes from a C string.
int32_t* runes_from_cstring(const char* const str, size_t length) {
    assert(length > 0);
    int32_t* runes = calloc(length, sizeof(int32_t));
    if (runes == NULL) {
        return NULL;
    }
    utf8iter* iter = utf8iter_new(str, length);
    size_t idx = 0;
    while (!iter->eof) {
        int32_t rune = utf8iter_next(iter);
        runes[idx] = rune;
        idx += 1;
    }
    free(iter);
    return runes;
}

// runes_to_cstring creates a C string from an array of runes.
char* runes_to_cstring(const int32_t* runes, size_t length) {
    char* str;
    if (length == 0) {
        str = calloc(1, sizeof(char));
        return str;
    }

    size_t maxlen = length * sizeof(int32_t) + 1;
    str = malloc(maxlen);
    if (str == NULL) {
        return NULL;
    }

    char* at = str;
    for (size_t i = 0; i < length; i++) {
        at = utf8_cat_rune(at, runes[i]);
    }
    *at = '\0';
    at += 1;

    if ((size_t)(at - str) < maxlen) {
        // shrink to real size
        size_t size = at - str;
        str = realloc(str, size);
    }
    return str;
}

#endif // SQLEAN_ENABLE_TEXT
#ifdef SQLEAN_ENABLE_UNICODE
// ---------------------------------
// src/unicode/extension.c
// ---------------------------------
// Originally by Unknown Author, Public Domain
// https://github.com/Zensey/sqlite3_unicode

// Modified by Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Unicode support for SQLite.

/*
 * Implements case-insensitive string comparison for Unicode strings.
 * Provides the following Unicode features:
 *
 *   - upper(), lower() and casefold() functions to normalize case.
 *   - like() function and LIKE operator with case-independent matching.
 *   - unaccent() function to normalize strings by removing accents.
 *
 * Tries to override the default NOCASE case-insensitive collation sequence
 * to support UTF-8 characters (available in SQLite CLI and C API only).
 *
 * Compile the project with the SQLITE_ENABLE_UNICODE preprocessor definition
 * in order to enable the code below.
 */

/*
** Un|Comment to provide additional unicode support to SQLite3 or adjust size for unused features
*/
#define SQLITE3_UNICODE_FOLD   // ~ 10KB increase
#define SQLITE3_UNICODE_LOWER  // ~ 10KB increase
#define SQLITE3_UNICODE_UPPER  // ~ 10KB increase
// #define SQLITE3_UNICODE_TITLE  // ~ 10KB increase
#define SQLITE3_UNICODE_UNACC  // ~ 30KB increase

/*
** SQLITE3_UNICODE_COLLATE will register and use the custom nocase collation instead of the standard
** one, which supports case folding and unaccenting.
*/
#define SQLITE3_UNICODE_COLLATE  // requires SQLITE3_UNICODE_FOLD to be defined as well.

/*
** SQLITE3_UNICODE_UNACC_AUTOMATIC will automatically try to unaccent any characters that
** are over the 0x80 character in the LIKE comparison operation and in the NOCASE collation
*sequence.
*/
#define SQLITE3_UNICODE_UNACC_AUTOMATIC  // requires SQLITE3_UNICODE_UNACC to be defined as well.

/*************************************************************************************************
** DO NOT MODIFY BELOW THIS LINE
**************************************************************************************************/

/* Generated by builder. Do not modify. Start unicode_version_defines */
/*
File was generated by : sqlite3_unicode.in
File was generated on : Fri Jun  5 01:10:23 2009
Using unicode data db : UnicodeData.txt
Using unicode fold db : CaseFolding.txt
*/
#define SQLITE3_UNICODE_VERSION_MAJOR 5
#define SQLITE3_UNICODE_VERSION_MINOR 1
#define SQLITE3_UNICODE_VERSION_MICRO 0
#define SQLITE3_UNICODE_VERSION_BUILD 12

#define __SQLITE3_UNICODE_VERSION_STRING(a, b, c, d) #a "." #b "." #c "." #d
#define _SQLITE3_UNICODE_VERSION_STRING(a, b, c, d) __SQLITE3_UNICODE_VERSION_STRING(a, b, c, d)
#define SQLITE3_UNICODE_VERSION_STRING                                                            \
    _SQLITE3_UNICODE_VERSION_STRING(SQLITE3_UNICODE_VERSION_MAJOR, SQLITE3_UNICODE_VERSION_MINOR, \
                                    SQLITE3_UNICODE_VERSION_MICRO, SQLITE3_UNICODE_VERSION_BUILD)

/* Generated by builder. Do not modify. End unicode_version_defines */

#include <assert.h>
#include <string.h>

SQLITE_EXTENSION_INIT3

#ifndef _SQLITE3_UNICODE_H
#define _SQLITE3_UNICODE_H

/*
** Add the ability to override 'extern'
*/
/*
** <sqlite3_unicode>
** The define of SQLITE_EXPORT is necessary to add the ability of exporting
** functions for both Microsoft Windows and Linux systems without the need
** of a .def file containing the names of the functions being exported.
*/
#ifndef SQLITE_EXPORT
#if ((defined(_WIN32) || defined(WIN32) || defined(__CYGWIN__) || defined(__MINGW32__) || \
      defined(__BORLANDC__)) &&                                                           \
     (!defined(SQLITE_CORE)))
#define SQLITE_EXPORT __declspec(dllexport)
#else
#define SQLITE_EXPORT SQLITE_EXTERN
#endif
#endif

#ifndef SQLITE_PRIVATE
#define SQLITE_PRIVATE static
#endif
#ifndef SQLITE_API
#define SQLITE_API
#endif

/*
** Integers of known sizes.  These typedefs might change for architectures
** where the sizes very.  Preprocessor macros are available so that the
** types can be conveniently redefined at compile-type.  Like this:
**
**         cc '-DUINTPTR_TYPE=long long int' ...
*/
#ifndef UINT32_TYPE
#ifdef HAVE_UINT32_T
#define UINT32_TYPE uint32_t
#else
#define UINT32_TYPE unsigned int
#endif
#endif
#ifndef UINT16_TYPE
#ifdef HAVE_UINT16_T
#define UINT16_TYPE uint16_t
#else
#define UINT16_TYPE unsigned short int
#endif
#endif
#ifndef INT16_TYPE
#ifdef HAVE_INT16_T
#define INT16_TYPE int16_t
#else
#define INT16_TYPE short int
#endif
#endif
#ifndef UINT8_TYPE
#ifdef HAVE_UINT8_T
#define UINT8_TYPE uint8_t
#else
#define UINT8_TYPE unsigned char
#endif
#endif
#ifndef INT8_TYPE
#ifdef HAVE_INT8_T
#define INT8_TYPE int8_t
#else
#define INT8_TYPE signed char
#endif
#endif
#ifndef LONGDOUBLE_TYPE
#define LONGDOUBLE_TYPE long double
#endif
typedef sqlite_int64 i64;  /* 8-byte signed integer */
typedef sqlite_uint64 u64; /* 8-byte unsigned integer */
typedef UINT32_TYPE u32;   /* 4-byte unsigned integer */
typedef UINT16_TYPE u16;   /* 2-byte unsigned integer */
typedef INT16_TYPE i16;    /* 2-byte signed integer */
typedef UINT8_TYPE u8;     /* 1-byte unsigned integer */
typedef INT8_TYPE i8;      /* 1-byte signed integer */

/*
** <sqlite3_unicode>
** These functions are intended for case conversion of single characters
** and return a single character containing the case converted character
** based on the unicode mapping tables.
*/
SQLITE_EXPORT u16 sqlite3_unicode_fold(u16 c);
SQLITE_EXPORT u16 sqlite3_unicode_lower(u16 c);
SQLITE_EXPORT u16 sqlite3_unicode_upper(u16 c);
SQLITE_EXPORT u16 sqlite3_unicode_title(u16 c);

/*
** <sqlite3_unicode>
** This function is intended for decomposing of single characters
** and return a pointer of characters (u16 **)p containing the decomposed
** character or string of characters. (int *)l will contain the length
** of characters contained in (u16 **)p based on the unicode mapping tables.
*/
SQLITE_EXPORT u16 sqlite3_unicode_unacc(u16 c, u16** p, int* l);

/*
** Another built-in collating sequence: NOCASE.
**
** This collating sequence is intended to be used for "case independant
** comparison". SQLite's knowledge of upper and lower case equivalents
** extends only to the 26 characters used in the English language.
**
** At the moment there is only a UTF-8 implementation.
*/
/*
** <sqlite3_unicode>
** The built-in collating sequence: NOCASE is extended to accomodate the
** unicode case folding mapping tables to normalize characters to their
** fold equivalents and test them for equality.
**
** Both UTF-8 and UTF-16 implementations are supported.
**
** (void *)encoding takes the following values
**   * SQLITE_UTF8  for UTF-8  encoded string comparison
**   * SQLITE_UFT16 for UTF-16 encoded string comparison
*/
SQLITE_EXPORT int sqlite3_unicode_collate(void* encoding,
                                          int nKey1,
                                          const void* pKey1,
                                          int nKey2,
                                          const void* pKey2);

/*
** <sqlite3_unicode>
** The following function needs to be called at application startup to load the extension.
*/
SQLITE_EXPORT int sqlite3_unicode_load();

/*
** <sqlite3_unicode>
** The following function needs to be called before application exit to unload the extension.
*/
SQLITE_EXPORT void sqlite3_unicode_free();

#ifdef __cplusplus
}
#endif

#endif /* _SQLITE3_UNICODE_H */
/*************************************************************************************************
 *************************************************************************************************
 *************************************************************************************************/

#ifdef SQLITE3_UNICODE_FOLD
/* Generated by builder. Do not modify. Start unicode_fold_defines */
#define UNICODE_FOLD_BLOCK_SHIFT 5
#define UNICODE_FOLD_BLOCK_MASK ((1 << UNICODE_FOLD_BLOCK_SHIFT) - 1)
#define UNICODE_FOLD_BLOCK_SIZE (1 << UNICODE_FOLD_BLOCK_SHIFT)
#define UNICODE_FOLD_BLOCK_COUNT 69
#define UNICODE_FOLD_INDEXES_SIZE (0x10000 >> UNICODE_FOLD_BLOCK_SHIFT)
/* Generated by builder. Do not modify. End unicode_fold_defines */

/* Generated by builder. Do not modify. Start unicode_fold_tables */

static unsigned short unicode_fold_indexes[UNICODE_FOLD_INDEXES_SIZE] = {
    0,  0,  1,  0,  0,  2,  3,  0,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 0,  0,  0,  0,  0,
    0,  0,  15, 16, 17, 18, 19, 20, 21, 22, 0,  23, 24, 25, 26, 27, 28, 29, 30, 0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  31, 32, 0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 0,  0,  0,  0,  0,  0,  0,  0,
    0,  49, 0,  50, 51, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  52, 53, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  54, 55, 0,  56, 57, 58, 59, 60,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  61, 62, 63, 0,  0,  0,  0,  64, 65, 66, 67, 0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  68, 0,  0,  0,  0,  0,  0};

static unsigned char unicode_fold_positions[UNICODE_FOLD_BLOCK_COUNT][UNICODE_FOLD_BLOCK_SIZE + 1] =
    {
        /* 0 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 1 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 2 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 3 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 4 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 5 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 6 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 7 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 8 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 9 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 10 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 11 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 12 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 13 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 14 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 15 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 16 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 17 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 18 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 19 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 20 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 21 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 22 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 23 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 24 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 25 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 26 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 27 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 28 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 29 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 30 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 31 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 32 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 33 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 34 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 35 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 36 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 37 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 38 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 39 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 40 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 41 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 42 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 43 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 44 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 45 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 46 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 47 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 48 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 49 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 50 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 51 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 52 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 53 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 54 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 55 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 56 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 57 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 58 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 59 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 60 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 61 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 62 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 63 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 64 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 65 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 66 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 67 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 68 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32}};

static unsigned short unicode_fold_data0[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_fold_data1[] = {
    0xFFFF, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006A,
    0x006B, 0x006C, 0x006D, 0x006E, 0x006F, 0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075,
    0x0076, 0x0077, 0x0078, 0x0079, 0x007A, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_fold_data2[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x03BC,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_fold_data3[] = {
    0x00E0, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x00E7, 0x00E8, 0x00E9, 0x00EA,
    0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF, 0x00F0, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x00F5,
    0x00F6, 0xFFFF, 0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x00FD, 0x00FE, 0xFFFF};
static unsigned short unicode_fold_data4[] = {
    0x0101, 0xFFFF, 0x0103, 0xFFFF, 0x0105, 0xFFFF, 0x0107, 0xFFFF, 0x0109, 0xFFFF, 0x010B,
    0xFFFF, 0x010D, 0xFFFF, 0x010F, 0xFFFF, 0x0111, 0xFFFF, 0x0113, 0xFFFF, 0x0115, 0xFFFF,
    0x0117, 0xFFFF, 0x0119, 0xFFFF, 0x011B, 0xFFFF, 0x011D, 0xFFFF, 0x011F, 0xFFFF};
static unsigned short unicode_fold_data5[] = {
    0x0121, 0xFFFF, 0x0123, 0xFFFF, 0x0125, 0xFFFF, 0x0127, 0xFFFF, 0x0129, 0xFFFF, 0x012B,
    0xFFFF, 0x012D, 0xFFFF, 0x012F, 0xFFFF, 0xFFFF, 0xFFFF, 0x0133, 0xFFFF, 0x0135, 0xFFFF,
    0x0137, 0xFFFF, 0xFFFF, 0x013A, 0xFFFF, 0x013C, 0xFFFF, 0x013E, 0xFFFF, 0x0140};
static unsigned short unicode_fold_data6[] = {
    0xFFFF, 0x0142, 0xFFFF, 0x0144, 0xFFFF, 0x0146, 0xFFFF, 0x0148, 0xFFFF, 0xFFFF, 0x014B,
    0xFFFF, 0x014D, 0xFFFF, 0x014F, 0xFFFF, 0x0151, 0xFFFF, 0x0153, 0xFFFF, 0x0155, 0xFFFF,
    0x0157, 0xFFFF, 0x0159, 0xFFFF, 0x015B, 0xFFFF, 0x015D, 0xFFFF, 0x015F, 0xFFFF};
static unsigned short unicode_fold_data7[] = {
    0x0161, 0xFFFF, 0x0163, 0xFFFF, 0x0165, 0xFFFF, 0x0167, 0xFFFF, 0x0169, 0xFFFF, 0x016B,
    0xFFFF, 0x016D, 0xFFFF, 0x016F, 0xFFFF, 0x0171, 0xFFFF, 0x0173, 0xFFFF, 0x0175, 0xFFFF,
    0x0177, 0xFFFF, 0x00FF, 0x017A, 0xFFFF, 0x017C, 0xFFFF, 0x017E, 0xFFFF, 0x0073};
static unsigned short unicode_fold_data8[] = {
    0xFFFF, 0x0253, 0x0183, 0xFFFF, 0x0185, 0xFFFF, 0x0254, 0x0188, 0xFFFF, 0x0256, 0x0257,
    0x018C, 0xFFFF, 0xFFFF, 0x01DD, 0x0259, 0x025B, 0x0192, 0xFFFF, 0x0260, 0x0263, 0xFFFF,
    0x0269, 0x0268, 0x0199, 0xFFFF, 0xFFFF, 0xFFFF, 0x026F, 0x0272, 0xFFFF, 0x0275};
static unsigned short unicode_fold_data9[] = {
    0x01A1, 0xFFFF, 0x01A3, 0xFFFF, 0x01A5, 0xFFFF, 0x0280, 0x01A8, 0xFFFF, 0x0283, 0xFFFF,
    0xFFFF, 0x01AD, 0xFFFF, 0x0288, 0x01B0, 0xFFFF, 0x028A, 0x028B, 0x01B4, 0xFFFF, 0x01B6,
    0xFFFF, 0x0292, 0x01B9, 0xFFFF, 0xFFFF, 0xFFFF, 0x01BD, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_fold_data10[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x01C6, 0x01C6, 0xFFFF, 0x01C9, 0x01C9, 0xFFFF, 0x01CC,
    0x01CC, 0xFFFF, 0x01CE, 0xFFFF, 0x01D0, 0xFFFF, 0x01D2, 0xFFFF, 0x01D4, 0xFFFF, 0x01D6,
    0xFFFF, 0x01D8, 0xFFFF, 0x01DA, 0xFFFF, 0x01DC, 0xFFFF, 0xFFFF, 0x01DF, 0xFFFF};
static unsigned short unicode_fold_data11[] = {
    0x01E1, 0xFFFF, 0x01E3, 0xFFFF, 0x01E5, 0xFFFF, 0x01E7, 0xFFFF, 0x01E9, 0xFFFF, 0x01EB,
    0xFFFF, 0x01ED, 0xFFFF, 0x01EF, 0xFFFF, 0xFFFF, 0x01F3, 0x01F3, 0xFFFF, 0x01F5, 0xFFFF,
    0x0195, 0x01BF, 0x01F9, 0xFFFF, 0x01FB, 0xFFFF, 0x01FD, 0xFFFF, 0x01FF, 0xFFFF};
static unsigned short unicode_fold_data12[] = {
    0x0201, 0xFFFF, 0x0203, 0xFFFF, 0x0205, 0xFFFF, 0x0207, 0xFFFF, 0x0209, 0xFFFF, 0x020B,
    0xFFFF, 0x020D, 0xFFFF, 0x020F, 0xFFFF, 0x0211, 0xFFFF, 0x0213, 0xFFFF, 0x0215, 0xFFFF,
    0x0217, 0xFFFF, 0x0219, 0xFFFF, 0x021B, 0xFFFF, 0x021D, 0xFFFF, 0x021F, 0xFFFF};
static unsigned short unicode_fold_data13[] = {
    0x019E, 0xFFFF, 0x0223, 0xFFFF, 0x0225, 0xFFFF, 0x0227, 0xFFFF, 0x0229, 0xFFFF, 0x022B,
    0xFFFF, 0x022D, 0xFFFF, 0x022F, 0xFFFF, 0x0231, 0xFFFF, 0x0233, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x2C65, 0x023C, 0xFFFF, 0x019A, 0x2C66, 0xFFFF};
static unsigned short unicode_fold_data14[] = {
    0xFFFF, 0x0242, 0xFFFF, 0x0180, 0x0289, 0x028C, 0x0247, 0xFFFF, 0x0249, 0xFFFF, 0x024B,
    0xFFFF, 0x024D, 0xFFFF, 0x024F, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_fold_data15[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x03B9, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_fold_data16[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0371, 0xFFFF, 0x0373, 0xFFFF, 0xFFFF, 0xFFFF,
    0x0377, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_fold_data17[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x03AC, 0xFFFF, 0x03AD, 0x03AE, 0x03AF,
    0xFFFF, 0x03CC, 0xFFFF, 0x03CD, 0x03CE, 0xFFFF, 0x03B1, 0x03B2, 0x03B3, 0x03B4, 0x03B5,
    0x03B6, 0x03B7, 0x03B8, 0x03B9, 0x03BA, 0x03BB, 0x03BC, 0x03BD, 0x03BE, 0x03BF};
static unsigned short unicode_fold_data18[] = {
    0x03C0, 0x03C1, 0xFFFF, 0x03C3, 0x03C4, 0x03C5, 0x03C6, 0x03C7, 0x03C8, 0x03C9, 0x03CA,
    0x03CB, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_fold_data19[] = {
    0xFFFF, 0xFFFF, 0x03C3, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x03D7, 0x03B2, 0x03B8, 0xFFFF, 0xFFFF, 0xFFFF, 0x03C6,
    0x03C0, 0xFFFF, 0x03D9, 0xFFFF, 0x03DB, 0xFFFF, 0x03DD, 0xFFFF, 0x03DF, 0xFFFF};
static unsigned short unicode_fold_data20[] = {
    0x03E1, 0xFFFF, 0x03E3, 0xFFFF, 0x03E5, 0xFFFF, 0x03E7, 0xFFFF, 0x03E9, 0xFFFF, 0x03EB,
    0xFFFF, 0x03ED, 0xFFFF, 0x03EF, 0xFFFF, 0x03BA, 0x03C1, 0xFFFF, 0xFFFF, 0x03B8, 0x03B5,
    0xFFFF, 0x03F8, 0xFFFF, 0x03F2, 0x03FB, 0xFFFF, 0xFFFF, 0x037B, 0x037C, 0x037D};
static unsigned short unicode_fold_data21[] = {
    0x0450, 0x0451, 0x0452, 0x0453, 0x0454, 0x0455, 0x0456, 0x0457, 0x0458, 0x0459, 0x045A,
    0x045B, 0x045C, 0x045D, 0x045E, 0x045F, 0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435,
    0x0436, 0x0437, 0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F};
static unsigned short unicode_fold_data22[] = {
    0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447, 0x0448, 0x0449, 0x044A,
    0x044B, 0x044C, 0x044D, 0x044E, 0x044F, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_fold_data23[] = {
    0x0461, 0xFFFF, 0x0463, 0xFFFF, 0x0465, 0xFFFF, 0x0467, 0xFFFF, 0x0469, 0xFFFF, 0x046B,
    0xFFFF, 0x046D, 0xFFFF, 0x046F, 0xFFFF, 0x0471, 0xFFFF, 0x0473, 0xFFFF, 0x0475, 0xFFFF,
    0x0477, 0xFFFF, 0x0479, 0xFFFF, 0x047B, 0xFFFF, 0x047D, 0xFFFF, 0x047F, 0xFFFF};
static unsigned short unicode_fold_data24[] = {
    0x0481, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x048B,
    0xFFFF, 0x048D, 0xFFFF, 0x048F, 0xFFFF, 0x0491, 0xFFFF, 0x0493, 0xFFFF, 0x0495, 0xFFFF,
    0x0497, 0xFFFF, 0x0499, 0xFFFF, 0x049B, 0xFFFF, 0x049D, 0xFFFF, 0x049F, 0xFFFF};
static unsigned short unicode_fold_data25[] = {
    0x04A1, 0xFFFF, 0x04A3, 0xFFFF, 0x04A5, 0xFFFF, 0x04A7, 0xFFFF, 0x04A9, 0xFFFF, 0x04AB,
    0xFFFF, 0x04AD, 0xFFFF, 0x04AF, 0xFFFF, 0x04B1, 0xFFFF, 0x04B3, 0xFFFF, 0x04B5, 0xFFFF,
    0x04B7, 0xFFFF, 0x04B9, 0xFFFF, 0x04BB, 0xFFFF, 0x04BD, 0xFFFF, 0x04BF, 0xFFFF};
static unsigned short unicode_fold_data26[] = {
    0x04CF, 0x04C2, 0xFFFF, 0x04C4, 0xFFFF, 0x04C6, 0xFFFF, 0x04C8, 0xFFFF, 0x04CA, 0xFFFF,
    0x04CC, 0xFFFF, 0x04CE, 0xFFFF, 0xFFFF, 0x04D1, 0xFFFF, 0x04D3, 0xFFFF, 0x04D5, 0xFFFF,
    0x04D7, 0xFFFF, 0x04D9, 0xFFFF, 0x04DB, 0xFFFF, 0x04DD, 0xFFFF, 0x04DF, 0xFFFF};
static unsigned short unicode_fold_data27[] = {
    0x04E1, 0xFFFF, 0x04E3, 0xFFFF, 0x04E5, 0xFFFF, 0x04E7, 0xFFFF, 0x04E9, 0xFFFF, 0x04EB,
    0xFFFF, 0x04ED, 0xFFFF, 0x04EF, 0xFFFF, 0x04F1, 0xFFFF, 0x04F3, 0xFFFF, 0x04F5, 0xFFFF,
    0x04F7, 0xFFFF, 0x04F9, 0xFFFF, 0x04FB, 0xFFFF, 0x04FD, 0xFFFF, 0x04FF, 0xFFFF};
static unsigned short unicode_fold_data28[] = {
    0x0501, 0xFFFF, 0x0503, 0xFFFF, 0x0505, 0xFFFF, 0x0507, 0xFFFF, 0x0509, 0xFFFF, 0x050B,
    0xFFFF, 0x050D, 0xFFFF, 0x050F, 0xFFFF, 0x0511, 0xFFFF, 0x0513, 0xFFFF, 0x0515, 0xFFFF,
    0x0517, 0xFFFF, 0x0519, 0xFFFF, 0x051B, 0xFFFF, 0x051D, 0xFFFF, 0x051F, 0xFFFF};
static unsigned short unicode_fold_data29[] = {
    0x0521, 0xFFFF, 0x0523, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0561, 0x0562, 0x0563, 0x0564, 0x0565,
    0x0566, 0x0567, 0x0568, 0x0569, 0x056A, 0x056B, 0x056C, 0x056D, 0x056E, 0x056F};
static unsigned short unicode_fold_data30[] = {
    0x0570, 0x0571, 0x0572, 0x0573, 0x0574, 0x0575, 0x0576, 0x0577, 0x0578, 0x0579, 0x057A,
    0x057B, 0x057C, 0x057D, 0x057E, 0x057F, 0x0580, 0x0581, 0x0582, 0x0583, 0x0584, 0x0585,
    0x0586, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_fold_data31[] = {
    0x2D00, 0x2D01, 0x2D02, 0x2D03, 0x2D04, 0x2D05, 0x2D06, 0x2D07, 0x2D08, 0x2D09, 0x2D0A,
    0x2D0B, 0x2D0C, 0x2D0D, 0x2D0E, 0x2D0F, 0x2D10, 0x2D11, 0x2D12, 0x2D13, 0x2D14, 0x2D15,
    0x2D16, 0x2D17, 0x2D18, 0x2D19, 0x2D1A, 0x2D1B, 0x2D1C, 0x2D1D, 0x2D1E, 0x2D1F};
static unsigned short unicode_fold_data32[] = {
    0x2D20, 0x2D21, 0x2D22, 0x2D23, 0x2D24, 0x2D25, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_fold_data33[] = {
    0x1E01, 0xFFFF, 0x1E03, 0xFFFF, 0x1E05, 0xFFFF, 0x1E07, 0xFFFF, 0x1E09, 0xFFFF, 0x1E0B,
    0xFFFF, 0x1E0D, 0xFFFF, 0x1E0F, 0xFFFF, 0x1E11, 0xFFFF, 0x1E13, 0xFFFF, 0x1E15, 0xFFFF,
    0x1E17, 0xFFFF, 0x1E19, 0xFFFF, 0x1E1B, 0xFFFF, 0x1E1D, 0xFFFF, 0x1E1F, 0xFFFF};
static unsigned short unicode_fold_data34[] = {
    0x1E21, 0xFFFF, 0x1E23, 0xFFFF, 0x1E25, 0xFFFF, 0x1E27, 0xFFFF, 0x1E29, 0xFFFF, 0x1E2B,
    0xFFFF, 0x1E2D, 0xFFFF, 0x1E2F, 0xFFFF, 0x1E31, 0xFFFF, 0x1E33, 0xFFFF, 0x1E35, 0xFFFF,
    0x1E37, 0xFFFF, 0x1E39, 0xFFFF, 0x1E3B, 0xFFFF, 0x1E3D, 0xFFFF, 0x1E3F, 0xFFFF};
static unsigned short unicode_fold_data35[] = {
    0x1E41, 0xFFFF, 0x1E43, 0xFFFF, 0x1E45, 0xFFFF, 0x1E47, 0xFFFF, 0x1E49, 0xFFFF, 0x1E4B,
    0xFFFF, 0x1E4D, 0xFFFF, 0x1E4F, 0xFFFF, 0x1E51, 0xFFFF, 0x1E53, 0xFFFF, 0x1E55, 0xFFFF,
    0x1E57, 0xFFFF, 0x1E59, 0xFFFF, 0x1E5B, 0xFFFF, 0x1E5D, 0xFFFF, 0x1E5F, 0xFFFF};
static unsigned short unicode_fold_data36[] = {
    0x1E61, 0xFFFF, 0x1E63, 0xFFFF, 0x1E65, 0xFFFF, 0x1E67, 0xFFFF, 0x1E69, 0xFFFF, 0x1E6B,
    0xFFFF, 0x1E6D, 0xFFFF, 0x1E6F, 0xFFFF, 0x1E71, 0xFFFF, 0x1E73, 0xFFFF, 0x1E75, 0xFFFF,
    0x1E77, 0xFFFF, 0x1E79, 0xFFFF, 0x1E7B, 0xFFFF, 0x1E7D, 0xFFFF, 0x1E7F, 0xFFFF};
static unsigned short unicode_fold_data37[] = {
    0x1E81, 0xFFFF, 0x1E83, 0xFFFF, 0x1E85, 0xFFFF, 0x1E87, 0xFFFF, 0x1E89, 0xFFFF, 0x1E8B,
    0xFFFF, 0x1E8D, 0xFFFF, 0x1E8F, 0xFFFF, 0x1E91, 0xFFFF, 0x1E93, 0xFFFF, 0x1E95, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1E61, 0xFFFF, 0xFFFF, 0x00DF, 0xFFFF};
static unsigned short unicode_fold_data38[] = {
    0x1EA1, 0xFFFF, 0x1EA3, 0xFFFF, 0x1EA5, 0xFFFF, 0x1EA7, 0xFFFF, 0x1EA9, 0xFFFF, 0x1EAB,
    0xFFFF, 0x1EAD, 0xFFFF, 0x1EAF, 0xFFFF, 0x1EB1, 0xFFFF, 0x1EB3, 0xFFFF, 0x1EB5, 0xFFFF,
    0x1EB7, 0xFFFF, 0x1EB9, 0xFFFF, 0x1EBB, 0xFFFF, 0x1EBD, 0xFFFF, 0x1EBF, 0xFFFF};
static unsigned short unicode_fold_data39[] = {
    0x1EC1, 0xFFFF, 0x1EC3, 0xFFFF, 0x1EC5, 0xFFFF, 0x1EC7, 0xFFFF, 0x1EC9, 0xFFFF, 0x1ECB,
    0xFFFF, 0x1ECD, 0xFFFF, 0x1ECF, 0xFFFF, 0x1ED1, 0xFFFF, 0x1ED3, 0xFFFF, 0x1ED5, 0xFFFF,
    0x1ED7, 0xFFFF, 0x1ED9, 0xFFFF, 0x1EDB, 0xFFFF, 0x1EDD, 0xFFFF, 0x1EDF, 0xFFFF};
static unsigned short unicode_fold_data40[] = {
    0x1EE1, 0xFFFF, 0x1EE3, 0xFFFF, 0x1EE5, 0xFFFF, 0x1EE7, 0xFFFF, 0x1EE9, 0xFFFF, 0x1EEB,
    0xFFFF, 0x1EED, 0xFFFF, 0x1EEF, 0xFFFF, 0x1EF1, 0xFFFF, 0x1EF3, 0xFFFF, 0x1EF5, 0xFFFF,
    0x1EF7, 0xFFFF, 0x1EF9, 0xFFFF, 0x1EFB, 0xFFFF, 0x1EFD, 0xFFFF, 0x1EFF, 0xFFFF};
static unsigned short unicode_fold_data41[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1F00, 0x1F01, 0x1F02,
    0x1F03, 0x1F04, 0x1F05, 0x1F06, 0x1F07, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0x1F10, 0x1F11, 0x1F12, 0x1F13, 0x1F14, 0x1F15, 0xFFFF, 0xFFFF};
static unsigned short unicode_fold_data42[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1F20, 0x1F21, 0x1F22,
    0x1F23, 0x1F24, 0x1F25, 0x1F26, 0x1F27, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0x1F30, 0x1F31, 0x1F32, 0x1F33, 0x1F34, 0x1F35, 0x1F36, 0x1F37};
static unsigned short unicode_fold_data43[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1F40, 0x1F41, 0x1F42,
    0x1F43, 0x1F44, 0x1F45, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0x1F51, 0xFFFF, 0x1F53, 0xFFFF, 0x1F55, 0xFFFF, 0x1F57};
static unsigned short unicode_fold_data44[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1F60, 0x1F61, 0x1F62,
    0x1F63, 0x1F64, 0x1F65, 0x1F66, 0x1F67, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_fold_data45[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1F80, 0x1F81, 0x1F82,
    0x1F83, 0x1F84, 0x1F85, 0x1F86, 0x1F87, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0x1F90, 0x1F91, 0x1F92, 0x1F93, 0x1F94, 0x1F95, 0x1F96, 0x1F97};
static unsigned short unicode_fold_data46[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1FA0, 0x1FA1, 0x1FA2,
    0x1FA3, 0x1FA4, 0x1FA5, 0x1FA6, 0x1FA7, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0x1FB0, 0x1FB1, 0x1F70, 0x1F71, 0x1FB3, 0xFFFF, 0x03B9, 0xFFFF};
static unsigned short unicode_fold_data47[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1F72, 0x1F73, 0x1F74,
    0x1F75, 0x1FC3, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0x1FD0, 0x1FD1, 0x1F76, 0x1F77, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_fold_data48[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1FE0, 0x1FE1, 0x1F7A,
    0x1F7B, 0x1FE5, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0x1F78, 0x1F79, 0x1F7C, 0x1F7D, 0x1FF3, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_fold_data49[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x03C9, 0xFFFF, 0xFFFF, 0xFFFF, 0x006B,
    0x00E5, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x214E, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_fold_data50[] = {
    0x2170, 0x2171, 0x2172, 0x2173, 0x2174, 0x2175, 0x2176, 0x2177, 0x2178, 0x2179, 0x217A,
    0x217B, 0x217C, 0x217D, 0x217E, 0x217F, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_fold_data51[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0x2184, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_fold_data52[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0x24D0, 0x24D1, 0x24D2, 0x24D3, 0x24D4, 0x24D5, 0x24D6, 0x24D7, 0x24D8, 0x24D9};
static unsigned short unicode_fold_data53[] = {
    0x24DA, 0x24DB, 0x24DC, 0x24DD, 0x24DE, 0x24DF, 0x24E0, 0x24E1, 0x24E2, 0x24E3, 0x24E4,
    0x24E5, 0x24E6, 0x24E7, 0x24E8, 0x24E9, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_fold_data54[] = {
    0x2C30, 0x2C31, 0x2C32, 0x2C33, 0x2C34, 0x2C35, 0x2C36, 0x2C37, 0x2C38, 0x2C39, 0x2C3A,
    0x2C3B, 0x2C3C, 0x2C3D, 0x2C3E, 0x2C3F, 0x2C40, 0x2C41, 0x2C42, 0x2C43, 0x2C44, 0x2C45,
    0x2C46, 0x2C47, 0x2C48, 0x2C49, 0x2C4A, 0x2C4B, 0x2C4C, 0x2C4D, 0x2C4E, 0x2C4F};
static unsigned short unicode_fold_data55[] = {
    0x2C50, 0x2C51, 0x2C52, 0x2C53, 0x2C54, 0x2C55, 0x2C56, 0x2C57, 0x2C58, 0x2C59, 0x2C5A,
    0x2C5B, 0x2C5C, 0x2C5D, 0x2C5E, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_fold_data56[] = {
    0x2C61, 0xFFFF, 0x026B, 0x1D7D, 0x027D, 0xFFFF, 0xFFFF, 0x2C68, 0xFFFF, 0x2C6A, 0xFFFF,
    0x2C6C, 0xFFFF, 0x0251, 0x0271, 0x0250, 0xFFFF, 0xFFFF, 0x2C73, 0xFFFF, 0xFFFF, 0x2C76,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_fold_data57[] = {
    0x2C81, 0xFFFF, 0x2C83, 0xFFFF, 0x2C85, 0xFFFF, 0x2C87, 0xFFFF, 0x2C89, 0xFFFF, 0x2C8B,
    0xFFFF, 0x2C8D, 0xFFFF, 0x2C8F, 0xFFFF, 0x2C91, 0xFFFF, 0x2C93, 0xFFFF, 0x2C95, 0xFFFF,
    0x2C97, 0xFFFF, 0x2C99, 0xFFFF, 0x2C9B, 0xFFFF, 0x2C9D, 0xFFFF, 0x2C9F, 0xFFFF};
static unsigned short unicode_fold_data58[] = {
    0x2CA1, 0xFFFF, 0x2CA3, 0xFFFF, 0x2CA5, 0xFFFF, 0x2CA7, 0xFFFF, 0x2CA9, 0xFFFF, 0x2CAB,
    0xFFFF, 0x2CAD, 0xFFFF, 0x2CAF, 0xFFFF, 0x2CB1, 0xFFFF, 0x2CB3, 0xFFFF, 0x2CB5, 0xFFFF,
    0x2CB7, 0xFFFF, 0x2CB9, 0xFFFF, 0x2CBB, 0xFFFF, 0x2CBD, 0xFFFF, 0x2CBF, 0xFFFF};
static unsigned short unicode_fold_data59[] = {
    0x2CC1, 0xFFFF, 0x2CC3, 0xFFFF, 0x2CC5, 0xFFFF, 0x2CC7, 0xFFFF, 0x2CC9, 0xFFFF, 0x2CCB,
    0xFFFF, 0x2CCD, 0xFFFF, 0x2CCF, 0xFFFF, 0x2CD1, 0xFFFF, 0x2CD3, 0xFFFF, 0x2CD5, 0xFFFF,
    0x2CD7, 0xFFFF, 0x2CD9, 0xFFFF, 0x2CDB, 0xFFFF, 0x2CDD, 0xFFFF, 0x2CDF, 0xFFFF};
static unsigned short unicode_fold_data60[] = {
    0x2CE1, 0xFFFF, 0x2CE3, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_fold_data61[] = {
    0xA641, 0xFFFF, 0xA643, 0xFFFF, 0xA645, 0xFFFF, 0xA647, 0xFFFF, 0xA649, 0xFFFF, 0xA64B,
    0xFFFF, 0xA64D, 0xFFFF, 0xA64F, 0xFFFF, 0xA651, 0xFFFF, 0xA653, 0xFFFF, 0xA655, 0xFFFF,
    0xA657, 0xFFFF, 0xA659, 0xFFFF, 0xA65B, 0xFFFF, 0xA65D, 0xFFFF, 0xA65F, 0xFFFF};
static unsigned short unicode_fold_data62[] = {
    0xFFFF, 0xFFFF, 0xA663, 0xFFFF, 0xA665, 0xFFFF, 0xA667, 0xFFFF, 0xA669, 0xFFFF, 0xA66B,
    0xFFFF, 0xA66D, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_fold_data63[] = {
    0xA681, 0xFFFF, 0xA683, 0xFFFF, 0xA685, 0xFFFF, 0xA687, 0xFFFF, 0xA689, 0xFFFF, 0xA68B,
    0xFFFF, 0xA68D, 0xFFFF, 0xA68F, 0xFFFF, 0xA691, 0xFFFF, 0xA693, 0xFFFF, 0xA695, 0xFFFF,
    0xA697, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_fold_data64[] = {
    0xFFFF, 0xFFFF, 0xA723, 0xFFFF, 0xA725, 0xFFFF, 0xA727, 0xFFFF, 0xA729, 0xFFFF, 0xA72B,
    0xFFFF, 0xA72D, 0xFFFF, 0xA72F, 0xFFFF, 0xFFFF, 0xFFFF, 0xA733, 0xFFFF, 0xA735, 0xFFFF,
    0xA737, 0xFFFF, 0xA739, 0xFFFF, 0xA73B, 0xFFFF, 0xA73D, 0xFFFF, 0xA73F, 0xFFFF};
static unsigned short unicode_fold_data65[] = {
    0xA741, 0xFFFF, 0xA743, 0xFFFF, 0xA745, 0xFFFF, 0xA747, 0xFFFF, 0xA749, 0xFFFF, 0xA74B,
    0xFFFF, 0xA74D, 0xFFFF, 0xA74F, 0xFFFF, 0xA751, 0xFFFF, 0xA753, 0xFFFF, 0xA755, 0xFFFF,
    0xA757, 0xFFFF, 0xA759, 0xFFFF, 0xA75B, 0xFFFF, 0xA75D, 0xFFFF, 0xA75F, 0xFFFF};
static unsigned short unicode_fold_data66[] = {
    0xA761, 0xFFFF, 0xA763, 0xFFFF, 0xA765, 0xFFFF, 0xA767, 0xFFFF, 0xA769, 0xFFFF, 0xA76B,
    0xFFFF, 0xA76D, 0xFFFF, 0xA76F, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xA77A, 0xFFFF, 0xA77C, 0xFFFF, 0x1D79, 0xA77F, 0xFFFF};
static unsigned short unicode_fold_data67[] = {
    0xA781, 0xFFFF, 0xA783, 0xFFFF, 0xA785, 0xFFFF, 0xA787, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xA78C, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_fold_data68[] = {
    0xFFFF, 0xFF41, 0xFF42, 0xFF43, 0xFF44, 0xFF45, 0xFF46, 0xFF47, 0xFF48, 0xFF49, 0xFF4A,
    0xFF4B, 0xFF4C, 0xFF4D, 0xFF4E, 0xFF4F, 0xFF50, 0xFF51, 0xFF52, 0xFF53, 0xFF54, 0xFF55,
    0xFF56, 0xFF57, 0xFF58, 0xFF59, 0xFF5A, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};

static unsigned short* unicode_fold_data_table[UNICODE_FOLD_BLOCK_COUNT] = {
    unicode_fold_data0,  unicode_fold_data1,  unicode_fold_data2,  unicode_fold_data3,
    unicode_fold_data4,  unicode_fold_data5,  unicode_fold_data6,  unicode_fold_data7,
    unicode_fold_data8,  unicode_fold_data9,  unicode_fold_data10, unicode_fold_data11,
    unicode_fold_data12, unicode_fold_data13, unicode_fold_data14, unicode_fold_data15,
    unicode_fold_data16, unicode_fold_data17, unicode_fold_data18, unicode_fold_data19,
    unicode_fold_data20, unicode_fold_data21, unicode_fold_data22, unicode_fold_data23,
    unicode_fold_data24, unicode_fold_data25, unicode_fold_data26, unicode_fold_data27,
    unicode_fold_data28, unicode_fold_data29, unicode_fold_data30, unicode_fold_data31,
    unicode_fold_data32, unicode_fold_data33, unicode_fold_data34, unicode_fold_data35,
    unicode_fold_data36, unicode_fold_data37, unicode_fold_data38, unicode_fold_data39,
    unicode_fold_data40, unicode_fold_data41, unicode_fold_data42, unicode_fold_data43,
    unicode_fold_data44, unicode_fold_data45, unicode_fold_data46, unicode_fold_data47,
    unicode_fold_data48, unicode_fold_data49, unicode_fold_data50, unicode_fold_data51,
    unicode_fold_data52, unicode_fold_data53, unicode_fold_data54, unicode_fold_data55,
    unicode_fold_data56, unicode_fold_data57, unicode_fold_data58, unicode_fold_data59,
    unicode_fold_data60, unicode_fold_data61, unicode_fold_data62, unicode_fold_data63,
    unicode_fold_data64, unicode_fold_data65, unicode_fold_data66, unicode_fold_data67,
    unicode_fold_data68};
/* Generated by builder. Do not modify. End unicode_fold_tables */

SQLITE_EXPORT u16 sqlite3_unicode_fold(u16 c) {
    u16 index = unicode_fold_indexes[(c) >> UNICODE_FOLD_BLOCK_SHIFT];
    u8 position = (c)&UNICODE_FOLD_BLOCK_MASK;
    u16(p) = (unicode_fold_data_table[index][unicode_fold_positions[index][position]]);
    int l = unicode_fold_positions[index][position + 1] - unicode_fold_positions[index][position];

    return ((l == 1) && ((p) == 0xFFFF)) ? c : p;
}
#endif

#ifdef SQLITE3_UNICODE_LOWER
/* Generated by builder. Do not modify. Start unicode_lower_defines */
#define UNICODE_LOWER_BLOCK_SHIFT 5
#define UNICODE_LOWER_BLOCK_MASK ((1 << UNICODE_LOWER_BLOCK_SHIFT) - 1)
#define UNICODE_LOWER_BLOCK_SIZE (1 << UNICODE_LOWER_BLOCK_SHIFT)
#define UNICODE_LOWER_BLOCK_COUNT 67
#define UNICODE_LOWER_INDEXES_SIZE (0x10000 >> UNICODE_LOWER_BLOCK_SHIFT)
/* Generated by builder. Do not modify. End unicode_lower_defines */

/* Generated by builder. Do not modify. Start unicode_lower_tables */

static unsigned short unicode_lower_indexes[UNICODE_LOWER_INDEXES_SIZE] = {
    0,  0,  1,  0,  0,  0,  2,  0,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 0,  0,  0,  0,  0,
    0,  0,  0,  14, 15, 16, 17, 18, 19, 20, 0,  21, 22, 23, 24, 25, 26, 27, 28, 0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  29, 30, 0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 0,  0,  0,  0,  0,  0,  0,  0,
    0,  47, 0,  48, 49, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  50, 51, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  52, 53, 0,  54, 55, 56, 57, 58,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  59, 60, 61, 0,  0,  0,  0,  62, 63, 64, 65, 0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  66, 0,  0,  0,  0,  0,  0};

static unsigned char
    unicode_lower_positions[UNICODE_LOWER_BLOCK_COUNT][UNICODE_LOWER_BLOCK_SIZE + 1] = {
        /* 0 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 1 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 2 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 3 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 4 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 5 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 6 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 7 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 8 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 9 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 10 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 11 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 12 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 13 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 14 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 15 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 16 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 17 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 18 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 19 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 20 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 21 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 22 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 23 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 24 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 25 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 26 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 27 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 28 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 29 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 30 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 31 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 32 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 33 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 34 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 35 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 36 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 37 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 38 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 39 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 40 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 41 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 42 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 43 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 44 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 45 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 46 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 47 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 48 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 49 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 50 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 51 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 52 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 53 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 54 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 55 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 56 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 57 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 58 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 59 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 60 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 61 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 62 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 63 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 64 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 65 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 66 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32}};

static unsigned short unicode_lower_data0[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_lower_data1[] = {
    0xFFFF, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006A,
    0x006B, 0x006C, 0x006D, 0x006E, 0x006F, 0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075,
    0x0076, 0x0077, 0x0078, 0x0079, 0x007A, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_lower_data2[] = {
    0x00E0, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x00E7, 0x00E8, 0x00E9, 0x00EA,
    0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF, 0x00F0, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x00F5,
    0x00F6, 0xFFFF, 0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x00FD, 0x00FE, 0xFFFF};
static unsigned short unicode_lower_data3[] = {
    0x0101, 0xFFFF, 0x0103, 0xFFFF, 0x0105, 0xFFFF, 0x0107, 0xFFFF, 0x0109, 0xFFFF, 0x010B,
    0xFFFF, 0x010D, 0xFFFF, 0x010F, 0xFFFF, 0x0111, 0xFFFF, 0x0113, 0xFFFF, 0x0115, 0xFFFF,
    0x0117, 0xFFFF, 0x0119, 0xFFFF, 0x011B, 0xFFFF, 0x011D, 0xFFFF, 0x011F, 0xFFFF};
static unsigned short unicode_lower_data4[] = {
    0x0121, 0xFFFF, 0x0123, 0xFFFF, 0x0125, 0xFFFF, 0x0127, 0xFFFF, 0x0129, 0xFFFF, 0x012B,
    0xFFFF, 0x012D, 0xFFFF, 0x012F, 0xFFFF, 0x0069, 0xFFFF, 0x0133, 0xFFFF, 0x0135, 0xFFFF,
    0x0137, 0xFFFF, 0xFFFF, 0x013A, 0xFFFF, 0x013C, 0xFFFF, 0x013E, 0xFFFF, 0x0140};
static unsigned short unicode_lower_data5[] = {
    0xFFFF, 0x0142, 0xFFFF, 0x0144, 0xFFFF, 0x0146, 0xFFFF, 0x0148, 0xFFFF, 0xFFFF, 0x014B,
    0xFFFF, 0x014D, 0xFFFF, 0x014F, 0xFFFF, 0x0151, 0xFFFF, 0x0153, 0xFFFF, 0x0155, 0xFFFF,
    0x0157, 0xFFFF, 0x0159, 0xFFFF, 0x015B, 0xFFFF, 0x015D, 0xFFFF, 0x015F, 0xFFFF};
static unsigned short unicode_lower_data6[] = {
    0x0161, 0xFFFF, 0x0163, 0xFFFF, 0x0165, 0xFFFF, 0x0167, 0xFFFF, 0x0169, 0xFFFF, 0x016B,
    0xFFFF, 0x016D, 0xFFFF, 0x016F, 0xFFFF, 0x0171, 0xFFFF, 0x0173, 0xFFFF, 0x0175, 0xFFFF,
    0x0177, 0xFFFF, 0x00FF, 0x017A, 0xFFFF, 0x017C, 0xFFFF, 0x017E, 0xFFFF, 0xFFFF};
static unsigned short unicode_lower_data7[] = {
    0xFFFF, 0x0253, 0x0183, 0xFFFF, 0x0185, 0xFFFF, 0x0254, 0x0188, 0xFFFF, 0x0256, 0x0257,
    0x018C, 0xFFFF, 0xFFFF, 0x01DD, 0x0259, 0x025B, 0x0192, 0xFFFF, 0x0260, 0x0263, 0xFFFF,
    0x0269, 0x0268, 0x0199, 0xFFFF, 0xFFFF, 0xFFFF, 0x026F, 0x0272, 0xFFFF, 0x0275};
static unsigned short unicode_lower_data8[] = {
    0x01A1, 0xFFFF, 0x01A3, 0xFFFF, 0x01A5, 0xFFFF, 0x0280, 0x01A8, 0xFFFF, 0x0283, 0xFFFF,
    0xFFFF, 0x01AD, 0xFFFF, 0x0288, 0x01B0, 0xFFFF, 0x028A, 0x028B, 0x01B4, 0xFFFF, 0x01B6,
    0xFFFF, 0x0292, 0x01B9, 0xFFFF, 0xFFFF, 0xFFFF, 0x01BD, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_lower_data9[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x01C6, 0x01C6, 0xFFFF, 0x01C9, 0x01C9, 0xFFFF, 0x01CC,
    0x01CC, 0xFFFF, 0x01CE, 0xFFFF, 0x01D0, 0xFFFF, 0x01D2, 0xFFFF, 0x01D4, 0xFFFF, 0x01D6,
    0xFFFF, 0x01D8, 0xFFFF, 0x01DA, 0xFFFF, 0x01DC, 0xFFFF, 0xFFFF, 0x01DF, 0xFFFF};
static unsigned short unicode_lower_data10[] = {
    0x01E1, 0xFFFF, 0x01E3, 0xFFFF, 0x01E5, 0xFFFF, 0x01E7, 0xFFFF, 0x01E9, 0xFFFF, 0x01EB,
    0xFFFF, 0x01ED, 0xFFFF, 0x01EF, 0xFFFF, 0xFFFF, 0x01F3, 0x01F3, 0xFFFF, 0x01F5, 0xFFFF,
    0x0195, 0x01BF, 0x01F9, 0xFFFF, 0x01FB, 0xFFFF, 0x01FD, 0xFFFF, 0x01FF, 0xFFFF};
static unsigned short unicode_lower_data11[] = {
    0x0201, 0xFFFF, 0x0203, 0xFFFF, 0x0205, 0xFFFF, 0x0207, 0xFFFF, 0x0209, 0xFFFF, 0x020B,
    0xFFFF, 0x020D, 0xFFFF, 0x020F, 0xFFFF, 0x0211, 0xFFFF, 0x0213, 0xFFFF, 0x0215, 0xFFFF,
    0x0217, 0xFFFF, 0x0219, 0xFFFF, 0x021B, 0xFFFF, 0x021D, 0xFFFF, 0x021F, 0xFFFF};
static unsigned short unicode_lower_data12[] = {
    0x019E, 0xFFFF, 0x0223, 0xFFFF, 0x0225, 0xFFFF, 0x0227, 0xFFFF, 0x0229, 0xFFFF, 0x022B,
    0xFFFF, 0x022D, 0xFFFF, 0x022F, 0xFFFF, 0x0231, 0xFFFF, 0x0233, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x2C65, 0x023C, 0xFFFF, 0x019A, 0x2C66, 0xFFFF};
static unsigned short unicode_lower_data13[] = {
    0xFFFF, 0x0242, 0xFFFF, 0x0180, 0x0289, 0x028C, 0x0247, 0xFFFF, 0x0249, 0xFFFF, 0x024B,
    0xFFFF, 0x024D, 0xFFFF, 0x024F, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_lower_data14[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0371, 0xFFFF, 0x0373, 0xFFFF, 0xFFFF, 0xFFFF,
    0x0377, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_lower_data15[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x03AC, 0xFFFF, 0x03AD, 0x03AE, 0x03AF,
    0xFFFF, 0x03CC, 0xFFFF, 0x03CD, 0x03CE, 0xFFFF, 0x03B1, 0x03B2, 0x03B3, 0x03B4, 0x03B5,
    0x03B6, 0x03B7, 0x03B8, 0x03B9, 0x03BA, 0x03BB, 0x03BC, 0x03BD, 0x03BE, 0x03BF};
static unsigned short unicode_lower_data16[] = {
    0x03C0, 0x03C1, 0xFFFF, 0x03C3, 0x03C4, 0x03C5, 0x03C6, 0x03C7, 0x03C8, 0x03C9, 0x03CA,
    0x03CB, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_lower_data17[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x03D7, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0x03D9, 0xFFFF, 0x03DB, 0xFFFF, 0x03DD, 0xFFFF, 0x03DF, 0xFFFF};
static unsigned short unicode_lower_data18[] = {
    0x03E1, 0xFFFF, 0x03E3, 0xFFFF, 0x03E5, 0xFFFF, 0x03E7, 0xFFFF, 0x03E9, 0xFFFF, 0x03EB,
    0xFFFF, 0x03ED, 0xFFFF, 0x03EF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x03B8, 0xFFFF,
    0xFFFF, 0x03F8, 0xFFFF, 0x03F2, 0x03FB, 0xFFFF, 0xFFFF, 0x037B, 0x037C, 0x037D};
static unsigned short unicode_lower_data19[] = {
    0x0450, 0x0451, 0x0452, 0x0453, 0x0454, 0x0455, 0x0456, 0x0457, 0x0458, 0x0459, 0x045A,
    0x045B, 0x045C, 0x045D, 0x045E, 0x045F, 0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435,
    0x0436, 0x0437, 0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F};
static unsigned short unicode_lower_data20[] = {
    0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447, 0x0448, 0x0449, 0x044A,
    0x044B, 0x044C, 0x044D, 0x044E, 0x044F, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_lower_data21[] = {
    0x0461, 0xFFFF, 0x0463, 0xFFFF, 0x0465, 0xFFFF, 0x0467, 0xFFFF, 0x0469, 0xFFFF, 0x046B,
    0xFFFF, 0x046D, 0xFFFF, 0x046F, 0xFFFF, 0x0471, 0xFFFF, 0x0473, 0xFFFF, 0x0475, 0xFFFF,
    0x0477, 0xFFFF, 0x0479, 0xFFFF, 0x047B, 0xFFFF, 0x047D, 0xFFFF, 0x047F, 0xFFFF};
static unsigned short unicode_lower_data22[] = {
    0x0481, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x048B,
    0xFFFF, 0x048D, 0xFFFF, 0x048F, 0xFFFF, 0x0491, 0xFFFF, 0x0493, 0xFFFF, 0x0495, 0xFFFF,
    0x0497, 0xFFFF, 0x0499, 0xFFFF, 0x049B, 0xFFFF, 0x049D, 0xFFFF, 0x049F, 0xFFFF};
static unsigned short unicode_lower_data23[] = {
    0x04A1, 0xFFFF, 0x04A3, 0xFFFF, 0x04A5, 0xFFFF, 0x04A7, 0xFFFF, 0x04A9, 0xFFFF, 0x04AB,
    0xFFFF, 0x04AD, 0xFFFF, 0x04AF, 0xFFFF, 0x04B1, 0xFFFF, 0x04B3, 0xFFFF, 0x04B5, 0xFFFF,
    0x04B7, 0xFFFF, 0x04B9, 0xFFFF, 0x04BB, 0xFFFF, 0x04BD, 0xFFFF, 0x04BF, 0xFFFF};
static unsigned short unicode_lower_data24[] = {
    0x04CF, 0x04C2, 0xFFFF, 0x04C4, 0xFFFF, 0x04C6, 0xFFFF, 0x04C8, 0xFFFF, 0x04CA, 0xFFFF,
    0x04CC, 0xFFFF, 0x04CE, 0xFFFF, 0xFFFF, 0x04D1, 0xFFFF, 0x04D3, 0xFFFF, 0x04D5, 0xFFFF,
    0x04D7, 0xFFFF, 0x04D9, 0xFFFF, 0x04DB, 0xFFFF, 0x04DD, 0xFFFF, 0x04DF, 0xFFFF};
static unsigned short unicode_lower_data25[] = {
    0x04E1, 0xFFFF, 0x04E3, 0xFFFF, 0x04E5, 0xFFFF, 0x04E7, 0xFFFF, 0x04E9, 0xFFFF, 0x04EB,
    0xFFFF, 0x04ED, 0xFFFF, 0x04EF, 0xFFFF, 0x04F1, 0xFFFF, 0x04F3, 0xFFFF, 0x04F5, 0xFFFF,
    0x04F7, 0xFFFF, 0x04F9, 0xFFFF, 0x04FB, 0xFFFF, 0x04FD, 0xFFFF, 0x04FF, 0xFFFF};
static unsigned short unicode_lower_data26[] = {
    0x0501, 0xFFFF, 0x0503, 0xFFFF, 0x0505, 0xFFFF, 0x0507, 0xFFFF, 0x0509, 0xFFFF, 0x050B,
    0xFFFF, 0x050D, 0xFFFF, 0x050F, 0xFFFF, 0x0511, 0xFFFF, 0x0513, 0xFFFF, 0x0515, 0xFFFF,
    0x0517, 0xFFFF, 0x0519, 0xFFFF, 0x051B, 0xFFFF, 0x051D, 0xFFFF, 0x051F, 0xFFFF};
static unsigned short unicode_lower_data27[] = {
    0x0521, 0xFFFF, 0x0523, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0561, 0x0562, 0x0563, 0x0564, 0x0565,
    0x0566, 0x0567, 0x0568, 0x0569, 0x056A, 0x056B, 0x056C, 0x056D, 0x056E, 0x056F};
static unsigned short unicode_lower_data28[] = {
    0x0570, 0x0571, 0x0572, 0x0573, 0x0574, 0x0575, 0x0576, 0x0577, 0x0578, 0x0579, 0x057A,
    0x057B, 0x057C, 0x057D, 0x057E, 0x057F, 0x0580, 0x0581, 0x0582, 0x0583, 0x0584, 0x0585,
    0x0586, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_lower_data29[] = {
    0x2D00, 0x2D01, 0x2D02, 0x2D03, 0x2D04, 0x2D05, 0x2D06, 0x2D07, 0x2D08, 0x2D09, 0x2D0A,
    0x2D0B, 0x2D0C, 0x2D0D, 0x2D0E, 0x2D0F, 0x2D10, 0x2D11, 0x2D12, 0x2D13, 0x2D14, 0x2D15,
    0x2D16, 0x2D17, 0x2D18, 0x2D19, 0x2D1A, 0x2D1B, 0x2D1C, 0x2D1D, 0x2D1E, 0x2D1F};
static unsigned short unicode_lower_data30[] = {
    0x2D20, 0x2D21, 0x2D22, 0x2D23, 0x2D24, 0x2D25, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_lower_data31[] = {
    0x1E01, 0xFFFF, 0x1E03, 0xFFFF, 0x1E05, 0xFFFF, 0x1E07, 0xFFFF, 0x1E09, 0xFFFF, 0x1E0B,
    0xFFFF, 0x1E0D, 0xFFFF, 0x1E0F, 0xFFFF, 0x1E11, 0xFFFF, 0x1E13, 0xFFFF, 0x1E15, 0xFFFF,
    0x1E17, 0xFFFF, 0x1E19, 0xFFFF, 0x1E1B, 0xFFFF, 0x1E1D, 0xFFFF, 0x1E1F, 0xFFFF};
static unsigned short unicode_lower_data32[] = {
    0x1E21, 0xFFFF, 0x1E23, 0xFFFF, 0x1E25, 0xFFFF, 0x1E27, 0xFFFF, 0x1E29, 0xFFFF, 0x1E2B,
    0xFFFF, 0x1E2D, 0xFFFF, 0x1E2F, 0xFFFF, 0x1E31, 0xFFFF, 0x1E33, 0xFFFF, 0x1E35, 0xFFFF,
    0x1E37, 0xFFFF, 0x1E39, 0xFFFF, 0x1E3B, 0xFFFF, 0x1E3D, 0xFFFF, 0x1E3F, 0xFFFF};
static unsigned short unicode_lower_data33[] = {
    0x1E41, 0xFFFF, 0x1E43, 0xFFFF, 0x1E45, 0xFFFF, 0x1E47, 0xFFFF, 0x1E49, 0xFFFF, 0x1E4B,
    0xFFFF, 0x1E4D, 0xFFFF, 0x1E4F, 0xFFFF, 0x1E51, 0xFFFF, 0x1E53, 0xFFFF, 0x1E55, 0xFFFF,
    0x1E57, 0xFFFF, 0x1E59, 0xFFFF, 0x1E5B, 0xFFFF, 0x1E5D, 0xFFFF, 0x1E5F, 0xFFFF};
static unsigned short unicode_lower_data34[] = {
    0x1E61, 0xFFFF, 0x1E63, 0xFFFF, 0x1E65, 0xFFFF, 0x1E67, 0xFFFF, 0x1E69, 0xFFFF, 0x1E6B,
    0xFFFF, 0x1E6D, 0xFFFF, 0x1E6F, 0xFFFF, 0x1E71, 0xFFFF, 0x1E73, 0xFFFF, 0x1E75, 0xFFFF,
    0x1E77, 0xFFFF, 0x1E79, 0xFFFF, 0x1E7B, 0xFFFF, 0x1E7D, 0xFFFF, 0x1E7F, 0xFFFF};
static unsigned short unicode_lower_data35[] = {
    0x1E81, 0xFFFF, 0x1E83, 0xFFFF, 0x1E85, 0xFFFF, 0x1E87, 0xFFFF, 0x1E89, 0xFFFF, 0x1E8B,
    0xFFFF, 0x1E8D, 0xFFFF, 0x1E8F, 0xFFFF, 0x1E91, 0xFFFF, 0x1E93, 0xFFFF, 0x1E95, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x00DF, 0xFFFF};
static unsigned short unicode_lower_data36[] = {
    0x1EA1, 0xFFFF, 0x1EA3, 0xFFFF, 0x1EA5, 0xFFFF, 0x1EA7, 0xFFFF, 0x1EA9, 0xFFFF, 0x1EAB,
    0xFFFF, 0x1EAD, 0xFFFF, 0x1EAF, 0xFFFF, 0x1EB1, 0xFFFF, 0x1EB3, 0xFFFF, 0x1EB5, 0xFFFF,
    0x1EB7, 0xFFFF, 0x1EB9, 0xFFFF, 0x1EBB, 0xFFFF, 0x1EBD, 0xFFFF, 0x1EBF, 0xFFFF};
static unsigned short unicode_lower_data37[] = {
    0x1EC1, 0xFFFF, 0x1EC3, 0xFFFF, 0x1EC5, 0xFFFF, 0x1EC7, 0xFFFF, 0x1EC9, 0xFFFF, 0x1ECB,
    0xFFFF, 0x1ECD, 0xFFFF, 0x1ECF, 0xFFFF, 0x1ED1, 0xFFFF, 0x1ED3, 0xFFFF, 0x1ED5, 0xFFFF,
    0x1ED7, 0xFFFF, 0x1ED9, 0xFFFF, 0x1EDB, 0xFFFF, 0x1EDD, 0xFFFF, 0x1EDF, 0xFFFF};
static unsigned short unicode_lower_data38[] = {
    0x1EE1, 0xFFFF, 0x1EE3, 0xFFFF, 0x1EE5, 0xFFFF, 0x1EE7, 0xFFFF, 0x1EE9, 0xFFFF, 0x1EEB,
    0xFFFF, 0x1EED, 0xFFFF, 0x1EEF, 0xFFFF, 0x1EF1, 0xFFFF, 0x1EF3, 0xFFFF, 0x1EF5, 0xFFFF,
    0x1EF7, 0xFFFF, 0x1EF9, 0xFFFF, 0x1EFB, 0xFFFF, 0x1EFD, 0xFFFF, 0x1EFF, 0xFFFF};
static unsigned short unicode_lower_data39[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1F00, 0x1F01, 0x1F02,
    0x1F03, 0x1F04, 0x1F05, 0x1F06, 0x1F07, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0x1F10, 0x1F11, 0x1F12, 0x1F13, 0x1F14, 0x1F15, 0xFFFF, 0xFFFF};
static unsigned short unicode_lower_data40[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1F20, 0x1F21, 0x1F22,
    0x1F23, 0x1F24, 0x1F25, 0x1F26, 0x1F27, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0x1F30, 0x1F31, 0x1F32, 0x1F33, 0x1F34, 0x1F35, 0x1F36, 0x1F37};
static unsigned short unicode_lower_data41[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1F40, 0x1F41, 0x1F42,
    0x1F43, 0x1F44, 0x1F45, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0x1F51, 0xFFFF, 0x1F53, 0xFFFF, 0x1F55, 0xFFFF, 0x1F57};
static unsigned short unicode_lower_data42[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1F60, 0x1F61, 0x1F62,
    0x1F63, 0x1F64, 0x1F65, 0x1F66, 0x1F67, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_lower_data43[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1F80, 0x1F81, 0x1F82,
    0x1F83, 0x1F84, 0x1F85, 0x1F86, 0x1F87, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0x1F90, 0x1F91, 0x1F92, 0x1F93, 0x1F94, 0x1F95, 0x1F96, 0x1F97};
static unsigned short unicode_lower_data44[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1FA0, 0x1FA1, 0x1FA2,
    0x1FA3, 0x1FA4, 0x1FA5, 0x1FA6, 0x1FA7, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0x1FB0, 0x1FB1, 0x1F70, 0x1F71, 0x1FB3, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_lower_data45[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1F72, 0x1F73, 0x1F74,
    0x1F75, 0x1FC3, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0x1FD0, 0x1FD1, 0x1F76, 0x1F77, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_lower_data46[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1FE0, 0x1FE1, 0x1F7A,
    0x1F7B, 0x1FE5, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0x1F78, 0x1F79, 0x1F7C, 0x1F7D, 0x1FF3, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_lower_data47[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x03C9, 0xFFFF, 0xFFFF, 0xFFFF, 0x006B,
    0x00E5, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x214E, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_lower_data48[] = {
    0x2170, 0x2171, 0x2172, 0x2173, 0x2174, 0x2175, 0x2176, 0x2177, 0x2178, 0x2179, 0x217A,
    0x217B, 0x217C, 0x217D, 0x217E, 0x217F, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_lower_data49[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0x2184, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_lower_data50[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0x24D0, 0x24D1, 0x24D2, 0x24D3, 0x24D4, 0x24D5, 0x24D6, 0x24D7, 0x24D8, 0x24D9};
static unsigned short unicode_lower_data51[] = {
    0x24DA, 0x24DB, 0x24DC, 0x24DD, 0x24DE, 0x24DF, 0x24E0, 0x24E1, 0x24E2, 0x24E3, 0x24E4,
    0x24E5, 0x24E6, 0x24E7, 0x24E8, 0x24E9, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_lower_data52[] = {
    0x2C30, 0x2C31, 0x2C32, 0x2C33, 0x2C34, 0x2C35, 0x2C36, 0x2C37, 0x2C38, 0x2C39, 0x2C3A,
    0x2C3B, 0x2C3C, 0x2C3D, 0x2C3E, 0x2C3F, 0x2C40, 0x2C41, 0x2C42, 0x2C43, 0x2C44, 0x2C45,
    0x2C46, 0x2C47, 0x2C48, 0x2C49, 0x2C4A, 0x2C4B, 0x2C4C, 0x2C4D, 0x2C4E, 0x2C4F};
static unsigned short unicode_lower_data53[] = {
    0x2C50, 0x2C51, 0x2C52, 0x2C53, 0x2C54, 0x2C55, 0x2C56, 0x2C57, 0x2C58, 0x2C59, 0x2C5A,
    0x2C5B, 0x2C5C, 0x2C5D, 0x2C5E, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_lower_data54[] = {
    0x2C61, 0xFFFF, 0x026B, 0x1D7D, 0x027D, 0xFFFF, 0xFFFF, 0x2C68, 0xFFFF, 0x2C6A, 0xFFFF,
    0x2C6C, 0xFFFF, 0x0251, 0x0271, 0x0250, 0xFFFF, 0xFFFF, 0x2C73, 0xFFFF, 0xFFFF, 0x2C76,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_lower_data55[] = {
    0x2C81, 0xFFFF, 0x2C83, 0xFFFF, 0x2C85, 0xFFFF, 0x2C87, 0xFFFF, 0x2C89, 0xFFFF, 0x2C8B,
    0xFFFF, 0x2C8D, 0xFFFF, 0x2C8F, 0xFFFF, 0x2C91, 0xFFFF, 0x2C93, 0xFFFF, 0x2C95, 0xFFFF,
    0x2C97, 0xFFFF, 0x2C99, 0xFFFF, 0x2C9B, 0xFFFF, 0x2C9D, 0xFFFF, 0x2C9F, 0xFFFF};
static unsigned short unicode_lower_data56[] = {
    0x2CA1, 0xFFFF, 0x2CA3, 0xFFFF, 0x2CA5, 0xFFFF, 0x2CA7, 0xFFFF, 0x2CA9, 0xFFFF, 0x2CAB,
    0xFFFF, 0x2CAD, 0xFFFF, 0x2CAF, 0xFFFF, 0x2CB1, 0xFFFF, 0x2CB3, 0xFFFF, 0x2CB5, 0xFFFF,
    0x2CB7, 0xFFFF, 0x2CB9, 0xFFFF, 0x2CBB, 0xFFFF, 0x2CBD, 0xFFFF, 0x2CBF, 0xFFFF};
static unsigned short unicode_lower_data57[] = {
    0x2CC1, 0xFFFF, 0x2CC3, 0xFFFF, 0x2CC5, 0xFFFF, 0x2CC7, 0xFFFF, 0x2CC9, 0xFFFF, 0x2CCB,
    0xFFFF, 0x2CCD, 0xFFFF, 0x2CCF, 0xFFFF, 0x2CD1, 0xFFFF, 0x2CD3, 0xFFFF, 0x2CD5, 0xFFFF,
    0x2CD7, 0xFFFF, 0x2CD9, 0xFFFF, 0x2CDB, 0xFFFF, 0x2CDD, 0xFFFF, 0x2CDF, 0xFFFF};
static unsigned short unicode_lower_data58[] = {
    0x2CE1, 0xFFFF, 0x2CE3, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_lower_data59[] = {
    0xA641, 0xFFFF, 0xA643, 0xFFFF, 0xA645, 0xFFFF, 0xA647, 0xFFFF, 0xA649, 0xFFFF, 0xA64B,
    0xFFFF, 0xA64D, 0xFFFF, 0xA64F, 0xFFFF, 0xA651, 0xFFFF, 0xA653, 0xFFFF, 0xA655, 0xFFFF,
    0xA657, 0xFFFF, 0xA659, 0xFFFF, 0xA65B, 0xFFFF, 0xA65D, 0xFFFF, 0xA65F, 0xFFFF};
static unsigned short unicode_lower_data60[] = {
    0xFFFF, 0xFFFF, 0xA663, 0xFFFF, 0xA665, 0xFFFF, 0xA667, 0xFFFF, 0xA669, 0xFFFF, 0xA66B,
    0xFFFF, 0xA66D, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_lower_data61[] = {
    0xA681, 0xFFFF, 0xA683, 0xFFFF, 0xA685, 0xFFFF, 0xA687, 0xFFFF, 0xA689, 0xFFFF, 0xA68B,
    0xFFFF, 0xA68D, 0xFFFF, 0xA68F, 0xFFFF, 0xA691, 0xFFFF, 0xA693, 0xFFFF, 0xA695, 0xFFFF,
    0xA697, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_lower_data62[] = {
    0xFFFF, 0xFFFF, 0xA723, 0xFFFF, 0xA725, 0xFFFF, 0xA727, 0xFFFF, 0xA729, 0xFFFF, 0xA72B,
    0xFFFF, 0xA72D, 0xFFFF, 0xA72F, 0xFFFF, 0xFFFF, 0xFFFF, 0xA733, 0xFFFF, 0xA735, 0xFFFF,
    0xA737, 0xFFFF, 0xA739, 0xFFFF, 0xA73B, 0xFFFF, 0xA73D, 0xFFFF, 0xA73F, 0xFFFF};
static unsigned short unicode_lower_data63[] = {
    0xA741, 0xFFFF, 0xA743, 0xFFFF, 0xA745, 0xFFFF, 0xA747, 0xFFFF, 0xA749, 0xFFFF, 0xA74B,
    0xFFFF, 0xA74D, 0xFFFF, 0xA74F, 0xFFFF, 0xA751, 0xFFFF, 0xA753, 0xFFFF, 0xA755, 0xFFFF,
    0xA757, 0xFFFF, 0xA759, 0xFFFF, 0xA75B, 0xFFFF, 0xA75D, 0xFFFF, 0xA75F, 0xFFFF};
static unsigned short unicode_lower_data64[] = {
    0xA761, 0xFFFF, 0xA763, 0xFFFF, 0xA765, 0xFFFF, 0xA767, 0xFFFF, 0xA769, 0xFFFF, 0xA76B,
    0xFFFF, 0xA76D, 0xFFFF, 0xA76F, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xA77A, 0xFFFF, 0xA77C, 0xFFFF, 0x1D79, 0xA77F, 0xFFFF};
static unsigned short unicode_lower_data65[] = {
    0xA781, 0xFFFF, 0xA783, 0xFFFF, 0xA785, 0xFFFF, 0xA787, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xA78C, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_lower_data66[] = {
    0xFFFF, 0xFF41, 0xFF42, 0xFF43, 0xFF44, 0xFF45, 0xFF46, 0xFF47, 0xFF48, 0xFF49, 0xFF4A,
    0xFF4B, 0xFF4C, 0xFF4D, 0xFF4E, 0xFF4F, 0xFF50, 0xFF51, 0xFF52, 0xFF53, 0xFF54, 0xFF55,
    0xFF56, 0xFF57, 0xFF58, 0xFF59, 0xFF5A, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};

static unsigned short* unicode_lower_data_table[UNICODE_LOWER_BLOCK_COUNT] = {
    unicode_lower_data0,  unicode_lower_data1,  unicode_lower_data2,  unicode_lower_data3,
    unicode_lower_data4,  unicode_lower_data5,  unicode_lower_data6,  unicode_lower_data7,
    unicode_lower_data8,  unicode_lower_data9,  unicode_lower_data10, unicode_lower_data11,
    unicode_lower_data12, unicode_lower_data13, unicode_lower_data14, unicode_lower_data15,
    unicode_lower_data16, unicode_lower_data17, unicode_lower_data18, unicode_lower_data19,
    unicode_lower_data20, unicode_lower_data21, unicode_lower_data22, unicode_lower_data23,
    unicode_lower_data24, unicode_lower_data25, unicode_lower_data26, unicode_lower_data27,
    unicode_lower_data28, unicode_lower_data29, unicode_lower_data30, unicode_lower_data31,
    unicode_lower_data32, unicode_lower_data33, unicode_lower_data34, unicode_lower_data35,
    unicode_lower_data36, unicode_lower_data37, unicode_lower_data38, unicode_lower_data39,
    unicode_lower_data40, unicode_lower_data41, unicode_lower_data42, unicode_lower_data43,
    unicode_lower_data44, unicode_lower_data45, unicode_lower_data46, unicode_lower_data47,
    unicode_lower_data48, unicode_lower_data49, unicode_lower_data50, unicode_lower_data51,
    unicode_lower_data52, unicode_lower_data53, unicode_lower_data54, unicode_lower_data55,
    unicode_lower_data56, unicode_lower_data57, unicode_lower_data58, unicode_lower_data59,
    unicode_lower_data60, unicode_lower_data61, unicode_lower_data62, unicode_lower_data63,
    unicode_lower_data64, unicode_lower_data65, unicode_lower_data66};
/* Generated by builder. Do not modify. End unicode_lower_tables */

SQLITE_EXPORT u16 sqlite3_unicode_lower(u16 c) {
    u16 index = unicode_lower_indexes[(c) >> UNICODE_LOWER_BLOCK_SHIFT];
    u8 position = (c)&UNICODE_LOWER_BLOCK_MASK;
    u16(p) = (unicode_lower_data_table[index][unicode_lower_positions[index][position]]);
    int l = unicode_lower_positions[index][position + 1] - unicode_lower_positions[index][position];

    return ((l == 1) && ((p) == 0xFFFF)) ? c : p;
}
#endif

#ifdef SQLITE3_UNICODE_UPPER
/* Generated by builder. Do not modify. Start unicode_upper_defines */
#define UNICODE_UPPER_BLOCK_SHIFT 6
#define UNICODE_UPPER_BLOCK_MASK ((1 << UNICODE_UPPER_BLOCK_SHIFT) - 1)
#define UNICODE_UPPER_BLOCK_SIZE (1 << UNICODE_UPPER_BLOCK_SHIFT)
#define UNICODE_UPPER_BLOCK_COUNT 44
#define UNICODE_UPPER_INDEXES_SIZE (0x10000 >> UNICODE_UPPER_BLOCK_SHIFT)
/* Generated by builder. Do not modify. End unicode_upper_defines */

/* Generated by builder. Do not modify. Start unicode_upper_tables */

static unsigned short unicode_upper_indexes[UNICODE_UPPER_INDEXES_SIZE] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,  10, 0, 0,  11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 21, 0,  0, 22, 23, 24, 25, 26, 27, 28, 29, 0,  0,  0,  0, 0, 30, 31,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 32, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  33, 34, 35, 36, 37, 0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  38, 39, 0,  40, 41, 42, 0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  43, 0, 0};

static unsigned char unicode_upper_positions[UNICODE_UPPER_BLOCK_COUNT][UNICODE_UPPER_BLOCK_SIZE +
                                                                        1] = {
    /* 0 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
             22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
             44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 1 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
             22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
             44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 2 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
             22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
             44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 3 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
             22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
             44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 4 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
             22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
             44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 5 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
             22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
             44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 6 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
             22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
             44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 7 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
             22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
             44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 8 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
             22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
             44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 9 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
             22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
             44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 10 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 11 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 12 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 13 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 14 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 15 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 16 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 17 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 18 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 19 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 20 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 21 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 22 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 23 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 24 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 25 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 26 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 27 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 28 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 29 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 30 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 31 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 32 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 33 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 34 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 35 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 36 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 37 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 38 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 39 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 40 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 41 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 42 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 43 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64}};

static unsigned short unicode_upper_data0[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_upper_data1[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004A, 0x004B,
    0x004C, 0x004D, 0x004E, 0x004F, 0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056,
    0x0057, 0x0058, 0x0059, 0x005A, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_upper_data2[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x039C, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_upper_data3[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x00C0,
    0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x00C7, 0x00C8, 0x00C9, 0x00CA, 0x00CB,
    0x00CC, 0x00CD, 0x00CE, 0x00CF, 0x00D0, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D6,
    0xFFFF, 0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x00DD, 0x00DE, 0x0178};
static unsigned short unicode_upper_data4[] = {
    0xFFFF, 0x0100, 0xFFFF, 0x0102, 0xFFFF, 0x0104, 0xFFFF, 0x0106, 0xFFFF, 0x0108, 0xFFFF,
    0x010A, 0xFFFF, 0x010C, 0xFFFF, 0x010E, 0xFFFF, 0x0110, 0xFFFF, 0x0112, 0xFFFF, 0x0114,
    0xFFFF, 0x0116, 0xFFFF, 0x0118, 0xFFFF, 0x011A, 0xFFFF, 0x011C, 0xFFFF, 0x011E, 0xFFFF,
    0x0120, 0xFFFF, 0x0122, 0xFFFF, 0x0124, 0xFFFF, 0x0126, 0xFFFF, 0x0128, 0xFFFF, 0x012A,
    0xFFFF, 0x012C, 0xFFFF, 0x012E, 0xFFFF, 0x0049, 0xFFFF, 0x0132, 0xFFFF, 0x0134, 0xFFFF,
    0x0136, 0xFFFF, 0xFFFF, 0x0139, 0xFFFF, 0x013B, 0xFFFF, 0x013D, 0xFFFF};
static unsigned short unicode_upper_data5[] = {
    0x013F, 0xFFFF, 0x0141, 0xFFFF, 0x0143, 0xFFFF, 0x0145, 0xFFFF, 0x0147, 0xFFFF, 0xFFFF,
    0x014A, 0xFFFF, 0x014C, 0xFFFF, 0x014E, 0xFFFF, 0x0150, 0xFFFF, 0x0152, 0xFFFF, 0x0154,
    0xFFFF, 0x0156, 0xFFFF, 0x0158, 0xFFFF, 0x015A, 0xFFFF, 0x015C, 0xFFFF, 0x015E, 0xFFFF,
    0x0160, 0xFFFF, 0x0162, 0xFFFF, 0x0164, 0xFFFF, 0x0166, 0xFFFF, 0x0168, 0xFFFF, 0x016A,
    0xFFFF, 0x016C, 0xFFFF, 0x016E, 0xFFFF, 0x0170, 0xFFFF, 0x0172, 0xFFFF, 0x0174, 0xFFFF,
    0x0176, 0xFFFF, 0xFFFF, 0x0179, 0xFFFF, 0x017B, 0xFFFF, 0x017D, 0x0053};
static unsigned short unicode_upper_data6[] = {
    0x0243, 0xFFFF, 0xFFFF, 0x0182, 0xFFFF, 0x0184, 0xFFFF, 0xFFFF, 0x0187, 0xFFFF, 0xFFFF,
    0xFFFF, 0x018B, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0191, 0xFFFF, 0xFFFF, 0x01F6,
    0xFFFF, 0xFFFF, 0xFFFF, 0x0198, 0x023D, 0xFFFF, 0xFFFF, 0xFFFF, 0x0220, 0xFFFF, 0xFFFF,
    0x01A0, 0xFFFF, 0x01A2, 0xFFFF, 0x01A4, 0xFFFF, 0xFFFF, 0x01A7, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0x01AC, 0xFFFF, 0xFFFF, 0x01AF, 0xFFFF, 0xFFFF, 0xFFFF, 0x01B3, 0xFFFF, 0x01B5,
    0xFFFF, 0xFFFF, 0x01B8, 0xFFFF, 0xFFFF, 0xFFFF, 0x01BC, 0xFFFF, 0x01F7};
static unsigned short unicode_upper_data7[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x01C4, 0x01C4, 0xFFFF, 0x01C7, 0x01C7, 0xFFFF,
    0x01CA, 0x01CA, 0xFFFF, 0x01CD, 0xFFFF, 0x01CF, 0xFFFF, 0x01D1, 0xFFFF, 0x01D3, 0xFFFF,
    0x01D5, 0xFFFF, 0x01D7, 0xFFFF, 0x01D9, 0xFFFF, 0x01DB, 0x018E, 0xFFFF, 0x01DE, 0xFFFF,
    0x01E0, 0xFFFF, 0x01E2, 0xFFFF, 0x01E4, 0xFFFF, 0x01E6, 0xFFFF, 0x01E8, 0xFFFF, 0x01EA,
    0xFFFF, 0x01EC, 0xFFFF, 0x01EE, 0xFFFF, 0xFFFF, 0x01F1, 0x01F1, 0xFFFF, 0x01F4, 0xFFFF,
    0xFFFF, 0xFFFF, 0x01F8, 0xFFFF, 0x01FA, 0xFFFF, 0x01FC, 0xFFFF, 0x01FE};
static unsigned short unicode_upper_data8[] = {
    0xFFFF, 0x0200, 0xFFFF, 0x0202, 0xFFFF, 0x0204, 0xFFFF, 0x0206, 0xFFFF, 0x0208, 0xFFFF,
    0x020A, 0xFFFF, 0x020C, 0xFFFF, 0x020E, 0xFFFF, 0x0210, 0xFFFF, 0x0212, 0xFFFF, 0x0214,
    0xFFFF, 0x0216, 0xFFFF, 0x0218, 0xFFFF, 0x021A, 0xFFFF, 0x021C, 0xFFFF, 0x021E, 0xFFFF,
    0xFFFF, 0xFFFF, 0x0222, 0xFFFF, 0x0224, 0xFFFF, 0x0226, 0xFFFF, 0x0228, 0xFFFF, 0x022A,
    0xFFFF, 0x022C, 0xFFFF, 0x022E, 0xFFFF, 0x0230, 0xFFFF, 0x0232, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x023B, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_upper_data9[] = {
    0xFFFF, 0xFFFF, 0x0241, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0246, 0xFFFF, 0x0248, 0xFFFF,
    0x024A, 0xFFFF, 0x024C, 0xFFFF, 0x024E, 0x2C6F, 0x2C6D, 0xFFFF, 0x0181, 0x0186, 0xFFFF,
    0x0189, 0x018A, 0xFFFF, 0x018F, 0xFFFF, 0x0190, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0193,
    0xFFFF, 0xFFFF, 0x0194, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0197, 0x0196, 0xFFFF, 0x2C62,
    0xFFFF, 0xFFFF, 0xFFFF, 0x019C, 0xFFFF, 0x2C6E, 0x019D, 0xFFFF, 0xFFFF, 0x019F, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x2C64, 0xFFFF, 0xFFFF};
static unsigned short unicode_upper_data10[] = {
    0x01A6, 0xFFFF, 0xFFFF, 0x01A9, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x01AE, 0x0244, 0x01B1,
    0x01B2, 0x0245, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x01B7, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_upper_data11[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0399, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0370, 0xFFFF, 0x0372, 0xFFFF, 0xFFFF, 0xFFFF,
    0x0376, 0xFFFF, 0xFFFF, 0xFFFF, 0x03FD, 0x03FE, 0x03FF, 0xFFFF, 0xFFFF};
static unsigned short unicode_upper_data12[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0x0386, 0x0388, 0x0389, 0x038A, 0xFFFF, 0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396,
    0x0397, 0x0398, 0x0399, 0x039A, 0x039B, 0x039C, 0x039D, 0x039E, 0x039F};
static unsigned short unicode_upper_data13[] = {
    0x03A0, 0x03A1, 0x03A3, 0x03A3, 0x03A4, 0x03A5, 0x03A6, 0x03A7, 0x03A8, 0x03A9, 0x03AA,
    0x03AB, 0x038C, 0x038E, 0x038F, 0xFFFF, 0x0392, 0x0398, 0xFFFF, 0xFFFF, 0xFFFF, 0x03A6,
    0x03A0, 0x03CF, 0xFFFF, 0x03D8, 0xFFFF, 0x03DA, 0xFFFF, 0x03DC, 0xFFFF, 0x03DE, 0xFFFF,
    0x03E0, 0xFFFF, 0x03E2, 0xFFFF, 0x03E4, 0xFFFF, 0x03E6, 0xFFFF, 0x03E8, 0xFFFF, 0x03EA,
    0xFFFF, 0x03EC, 0xFFFF, 0x03EE, 0x039A, 0x03A1, 0x03F9, 0xFFFF, 0xFFFF, 0x0395, 0xFFFF,
    0xFFFF, 0x03F7, 0xFFFF, 0xFFFF, 0x03FA, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_upper_data14[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416,
    0x0417, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F};
static unsigned short unicode_upper_data15[] = {
    0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427, 0x0428, 0x0429, 0x042A,
    0x042B, 0x042C, 0x042D, 0x042E, 0x042F, 0x0400, 0x0401, 0x0402, 0x0403, 0x0404, 0x0405,
    0x0406, 0x0407, 0x0408, 0x0409, 0x040A, 0x040B, 0x040C, 0x040D, 0x040E, 0x040F, 0xFFFF,
    0x0460, 0xFFFF, 0x0462, 0xFFFF, 0x0464, 0xFFFF, 0x0466, 0xFFFF, 0x0468, 0xFFFF, 0x046A,
    0xFFFF, 0x046C, 0xFFFF, 0x046E, 0xFFFF, 0x0470, 0xFFFF, 0x0472, 0xFFFF, 0x0474, 0xFFFF,
    0x0476, 0xFFFF, 0x0478, 0xFFFF, 0x047A, 0xFFFF, 0x047C, 0xFFFF, 0x047E};
static unsigned short unicode_upper_data16[] = {
    0xFFFF, 0x0480, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0x048A, 0xFFFF, 0x048C, 0xFFFF, 0x048E, 0xFFFF, 0x0490, 0xFFFF, 0x0492, 0xFFFF, 0x0494,
    0xFFFF, 0x0496, 0xFFFF, 0x0498, 0xFFFF, 0x049A, 0xFFFF, 0x049C, 0xFFFF, 0x049E, 0xFFFF,
    0x04A0, 0xFFFF, 0x04A2, 0xFFFF, 0x04A4, 0xFFFF, 0x04A6, 0xFFFF, 0x04A8, 0xFFFF, 0x04AA,
    0xFFFF, 0x04AC, 0xFFFF, 0x04AE, 0xFFFF, 0x04B0, 0xFFFF, 0x04B2, 0xFFFF, 0x04B4, 0xFFFF,
    0x04B6, 0xFFFF, 0x04B8, 0xFFFF, 0x04BA, 0xFFFF, 0x04BC, 0xFFFF, 0x04BE};
static unsigned short unicode_upper_data17[] = {
    0xFFFF, 0xFFFF, 0x04C1, 0xFFFF, 0x04C3, 0xFFFF, 0x04C5, 0xFFFF, 0x04C7, 0xFFFF, 0x04C9,
    0xFFFF, 0x04CB, 0xFFFF, 0x04CD, 0x04C0, 0xFFFF, 0x04D0, 0xFFFF, 0x04D2, 0xFFFF, 0x04D4,
    0xFFFF, 0x04D6, 0xFFFF, 0x04D8, 0xFFFF, 0x04DA, 0xFFFF, 0x04DC, 0xFFFF, 0x04DE, 0xFFFF,
    0x04E0, 0xFFFF, 0x04E2, 0xFFFF, 0x04E4, 0xFFFF, 0x04E6, 0xFFFF, 0x04E8, 0xFFFF, 0x04EA,
    0xFFFF, 0x04EC, 0xFFFF, 0x04EE, 0xFFFF, 0x04F0, 0xFFFF, 0x04F2, 0xFFFF, 0x04F4, 0xFFFF,
    0x04F6, 0xFFFF, 0x04F8, 0xFFFF, 0x04FA, 0xFFFF, 0x04FC, 0xFFFF, 0x04FE};
static unsigned short unicode_upper_data18[] = {
    0xFFFF, 0x0500, 0xFFFF, 0x0502, 0xFFFF, 0x0504, 0xFFFF, 0x0506, 0xFFFF, 0x0508, 0xFFFF,
    0x050A, 0xFFFF, 0x050C, 0xFFFF, 0x050E, 0xFFFF, 0x0510, 0xFFFF, 0x0512, 0xFFFF, 0x0514,
    0xFFFF, 0x0516, 0xFFFF, 0x0518, 0xFFFF, 0x051A, 0xFFFF, 0x051C, 0xFFFF, 0x051E, 0xFFFF,
    0x0520, 0xFFFF, 0x0522, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_upper_data19[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0x0531, 0x0532, 0x0533, 0x0534, 0x0535, 0x0536, 0x0537, 0x0538, 0x0539, 0x053A, 0x053B,
    0x053C, 0x053D, 0x053E, 0x053F, 0x0540, 0x0541, 0x0542, 0x0543, 0x0544, 0x0545, 0x0546,
    0x0547, 0x0548, 0x0549, 0x054A, 0x054B, 0x054C, 0x054D, 0x054E, 0x054F};
static unsigned short unicode_upper_data20[] = {
    0x0550, 0x0551, 0x0552, 0x0553, 0x0554, 0x0555, 0x0556, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_upper_data21[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xA77D, 0xFFFF, 0xFFFF, 0xFFFF, 0x2C63, 0xFFFF, 0xFFFF};
static unsigned short unicode_upper_data22[] = {
    0xFFFF, 0x1E00, 0xFFFF, 0x1E02, 0xFFFF, 0x1E04, 0xFFFF, 0x1E06, 0xFFFF, 0x1E08, 0xFFFF,
    0x1E0A, 0xFFFF, 0x1E0C, 0xFFFF, 0x1E0E, 0xFFFF, 0x1E10, 0xFFFF, 0x1E12, 0xFFFF, 0x1E14,
    0xFFFF, 0x1E16, 0xFFFF, 0x1E18, 0xFFFF, 0x1E1A, 0xFFFF, 0x1E1C, 0xFFFF, 0x1E1E, 0xFFFF,
    0x1E20, 0xFFFF, 0x1E22, 0xFFFF, 0x1E24, 0xFFFF, 0x1E26, 0xFFFF, 0x1E28, 0xFFFF, 0x1E2A,
    0xFFFF, 0x1E2C, 0xFFFF, 0x1E2E, 0xFFFF, 0x1E30, 0xFFFF, 0x1E32, 0xFFFF, 0x1E34, 0xFFFF,
    0x1E36, 0xFFFF, 0x1E38, 0xFFFF, 0x1E3A, 0xFFFF, 0x1E3C, 0xFFFF, 0x1E3E};
static unsigned short unicode_upper_data23[] = {
    0xFFFF, 0x1E40, 0xFFFF, 0x1E42, 0xFFFF, 0x1E44, 0xFFFF, 0x1E46, 0xFFFF, 0x1E48, 0xFFFF,
    0x1E4A, 0xFFFF, 0x1E4C, 0xFFFF, 0x1E4E, 0xFFFF, 0x1E50, 0xFFFF, 0x1E52, 0xFFFF, 0x1E54,
    0xFFFF, 0x1E56, 0xFFFF, 0x1E58, 0xFFFF, 0x1E5A, 0xFFFF, 0x1E5C, 0xFFFF, 0x1E5E, 0xFFFF,
    0x1E60, 0xFFFF, 0x1E62, 0xFFFF, 0x1E64, 0xFFFF, 0x1E66, 0xFFFF, 0x1E68, 0xFFFF, 0x1E6A,
    0xFFFF, 0x1E6C, 0xFFFF, 0x1E6E, 0xFFFF, 0x1E70, 0xFFFF, 0x1E72, 0xFFFF, 0x1E74, 0xFFFF,
    0x1E76, 0xFFFF, 0x1E78, 0xFFFF, 0x1E7A, 0xFFFF, 0x1E7C, 0xFFFF, 0x1E7E};
static unsigned short unicode_upper_data24[] = {
    0xFFFF, 0x1E80, 0xFFFF, 0x1E82, 0xFFFF, 0x1E84, 0xFFFF, 0x1E86, 0xFFFF, 0x1E88, 0xFFFF,
    0x1E8A, 0xFFFF, 0x1E8C, 0xFFFF, 0x1E8E, 0xFFFF, 0x1E90, 0xFFFF, 0x1E92, 0xFFFF, 0x1E94,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1E60, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0x1EA0, 0xFFFF, 0x1EA2, 0xFFFF, 0x1EA4, 0xFFFF, 0x1EA6, 0xFFFF, 0x1EA8, 0xFFFF, 0x1EAA,
    0xFFFF, 0x1EAC, 0xFFFF, 0x1EAE, 0xFFFF, 0x1EB0, 0xFFFF, 0x1EB2, 0xFFFF, 0x1EB4, 0xFFFF,
    0x1EB6, 0xFFFF, 0x1EB8, 0xFFFF, 0x1EBA, 0xFFFF, 0x1EBC, 0xFFFF, 0x1EBE};
static unsigned short unicode_upper_data25[] = {
    0xFFFF, 0x1EC0, 0xFFFF, 0x1EC2, 0xFFFF, 0x1EC4, 0xFFFF, 0x1EC6, 0xFFFF, 0x1EC8, 0xFFFF,
    0x1ECA, 0xFFFF, 0x1ECC, 0xFFFF, 0x1ECE, 0xFFFF, 0x1ED0, 0xFFFF, 0x1ED2, 0xFFFF, 0x1ED4,
    0xFFFF, 0x1ED6, 0xFFFF, 0x1ED8, 0xFFFF, 0x1EDA, 0xFFFF, 0x1EDC, 0xFFFF, 0x1EDE, 0xFFFF,
    0x1EE0, 0xFFFF, 0x1EE2, 0xFFFF, 0x1EE4, 0xFFFF, 0x1EE6, 0xFFFF, 0x1EE8, 0xFFFF, 0x1EEA,
    0xFFFF, 0x1EEC, 0xFFFF, 0x1EEE, 0xFFFF, 0x1EF0, 0xFFFF, 0x1EF2, 0xFFFF, 0x1EF4, 0xFFFF,
    0x1EF6, 0xFFFF, 0x1EF8, 0xFFFF, 0x1EFA, 0xFFFF, 0x1EFC, 0xFFFF, 0x1EFE};
static unsigned short unicode_upper_data26[] = {
    0x1F08, 0x1F09, 0x1F0A, 0x1F0B, 0x1F0C, 0x1F0D, 0x1F0E, 0x1F0F, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1F18, 0x1F19, 0x1F1A, 0x1F1B, 0x1F1C, 0x1F1D,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1F28,
    0x1F29, 0x1F2A, 0x1F2B, 0x1F2C, 0x1F2D, 0x1F2E, 0x1F2F, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1F38, 0x1F39, 0x1F3A, 0x1F3B, 0x1F3C, 0x1F3D, 0x1F3E,
    0x1F3F, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_upper_data27[] = {
    0x1F48, 0x1F49, 0x1F4A, 0x1F4B, 0x1F4C, 0x1F4D, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1F59, 0xFFFF, 0x1F5B, 0xFFFF, 0x1F5D,
    0xFFFF, 0x1F5F, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1F68,
    0x1F69, 0x1F6A, 0x1F6B, 0x1F6C, 0x1F6D, 0x1F6E, 0x1F6F, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1FBA, 0x1FBB, 0x1FC8, 0x1FC9, 0x1FCA, 0x1FCB, 0x1FDA,
    0x1FDB, 0x1FF8, 0x1FF9, 0x1FEA, 0x1FEB, 0x1FFA, 0x1FFB, 0xFFFF, 0xFFFF};
static unsigned short unicode_upper_data28[] = {
    0x1F88, 0x1F89, 0x1F8A, 0x1F8B, 0x1F8C, 0x1F8D, 0x1F8E, 0x1F8F, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1F98, 0x1F99, 0x1F9A, 0x1F9B, 0x1F9C, 0x1F9D,
    0x1F9E, 0x1F9F, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1FA8,
    0x1FA9, 0x1FAA, 0x1FAB, 0x1FAC, 0x1FAD, 0x1FAE, 0x1FAF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1FB8, 0x1FB9, 0xFFFF, 0x1FBC, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0399, 0xFFFF};
static unsigned short unicode_upper_data29[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0x1FCC, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1FD8, 0x1FD9, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1FE8,
    0x1FE9, 0xFFFF, 0xFFFF, 0xFFFF, 0x1FEC, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1FFC, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_upper_data30[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0x2132, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x2160, 0x2161, 0x2162, 0x2163, 0x2164, 0x2165, 0x2166,
    0x2167, 0x2168, 0x2169, 0x216A, 0x216B, 0x216C, 0x216D, 0x216E, 0x216F};
static unsigned short unicode_upper_data31[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x2183, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_upper_data32[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x24B6, 0x24B7, 0x24B8, 0x24B9, 0x24BA, 0x24BB,
    0x24BC, 0x24BD, 0x24BE, 0x24BF, 0x24C0, 0x24C1, 0x24C2, 0x24C3, 0x24C4, 0x24C5, 0x24C6,
    0x24C7, 0x24C8, 0x24C9, 0x24CA, 0x24CB, 0x24CC, 0x24CD, 0x24CE, 0x24CF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_upper_data33[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x2C00, 0x2C01, 0x2C02, 0x2C03, 0x2C04, 0x2C05, 0x2C06,
    0x2C07, 0x2C08, 0x2C09, 0x2C0A, 0x2C0B, 0x2C0C, 0x2C0D, 0x2C0E, 0x2C0F};
static unsigned short unicode_upper_data34[] = {
    0x2C10, 0x2C11, 0x2C12, 0x2C13, 0x2C14, 0x2C15, 0x2C16, 0x2C17, 0x2C18, 0x2C19, 0x2C1A,
    0x2C1B, 0x2C1C, 0x2C1D, 0x2C1E, 0x2C1F, 0x2C20, 0x2C21, 0x2C22, 0x2C23, 0x2C24, 0x2C25,
    0x2C26, 0x2C27, 0x2C28, 0x2C29, 0x2C2A, 0x2C2B, 0x2C2C, 0x2C2D, 0x2C2E, 0xFFFF, 0xFFFF,
    0x2C60, 0xFFFF, 0xFFFF, 0xFFFF, 0x023A, 0x023E, 0xFFFF, 0x2C67, 0xFFFF, 0x2C69, 0xFFFF,
    0x2C6B, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x2C72, 0xFFFF, 0xFFFF, 0x2C75,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_upper_data35[] = {
    0xFFFF, 0x2C80, 0xFFFF, 0x2C82, 0xFFFF, 0x2C84, 0xFFFF, 0x2C86, 0xFFFF, 0x2C88, 0xFFFF,
    0x2C8A, 0xFFFF, 0x2C8C, 0xFFFF, 0x2C8E, 0xFFFF, 0x2C90, 0xFFFF, 0x2C92, 0xFFFF, 0x2C94,
    0xFFFF, 0x2C96, 0xFFFF, 0x2C98, 0xFFFF, 0x2C9A, 0xFFFF, 0x2C9C, 0xFFFF, 0x2C9E, 0xFFFF,
    0x2CA0, 0xFFFF, 0x2CA2, 0xFFFF, 0x2CA4, 0xFFFF, 0x2CA6, 0xFFFF, 0x2CA8, 0xFFFF, 0x2CAA,
    0xFFFF, 0x2CAC, 0xFFFF, 0x2CAE, 0xFFFF, 0x2CB0, 0xFFFF, 0x2CB2, 0xFFFF, 0x2CB4, 0xFFFF,
    0x2CB6, 0xFFFF, 0x2CB8, 0xFFFF, 0x2CBA, 0xFFFF, 0x2CBC, 0xFFFF, 0x2CBE};
static unsigned short unicode_upper_data36[] = {
    0xFFFF, 0x2CC0, 0xFFFF, 0x2CC2, 0xFFFF, 0x2CC4, 0xFFFF, 0x2CC6, 0xFFFF, 0x2CC8, 0xFFFF,
    0x2CCA, 0xFFFF, 0x2CCC, 0xFFFF, 0x2CCE, 0xFFFF, 0x2CD0, 0xFFFF, 0x2CD2, 0xFFFF, 0x2CD4,
    0xFFFF, 0x2CD6, 0xFFFF, 0x2CD8, 0xFFFF, 0x2CDA, 0xFFFF, 0x2CDC, 0xFFFF, 0x2CDE, 0xFFFF,
    0x2CE0, 0xFFFF, 0x2CE2, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_upper_data37[] = {
    0x10A0, 0x10A1, 0x10A2, 0x10A3, 0x10A4, 0x10A5, 0x10A6, 0x10A7, 0x10A8, 0x10A9, 0x10AA,
    0x10AB, 0x10AC, 0x10AD, 0x10AE, 0x10AF, 0x10B0, 0x10B1, 0x10B2, 0x10B3, 0x10B4, 0x10B5,
    0x10B6, 0x10B7, 0x10B8, 0x10B9, 0x10BA, 0x10BB, 0x10BC, 0x10BD, 0x10BE, 0x10BF, 0x10C0,
    0x10C1, 0x10C2, 0x10C3, 0x10C4, 0x10C5, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_upper_data38[] = {
    0xFFFF, 0xA640, 0xFFFF, 0xA642, 0xFFFF, 0xA644, 0xFFFF, 0xA646, 0xFFFF, 0xA648, 0xFFFF,
    0xA64A, 0xFFFF, 0xA64C, 0xFFFF, 0xA64E, 0xFFFF, 0xA650, 0xFFFF, 0xA652, 0xFFFF, 0xA654,
    0xFFFF, 0xA656, 0xFFFF, 0xA658, 0xFFFF, 0xA65A, 0xFFFF, 0xA65C, 0xFFFF, 0xA65E, 0xFFFF,
    0xFFFF, 0xFFFF, 0xA662, 0xFFFF, 0xA664, 0xFFFF, 0xA666, 0xFFFF, 0xA668, 0xFFFF, 0xA66A,
    0xFFFF, 0xA66C, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_upper_data39[] = {
    0xFFFF, 0xA680, 0xFFFF, 0xA682, 0xFFFF, 0xA684, 0xFFFF, 0xA686, 0xFFFF, 0xA688, 0xFFFF,
    0xA68A, 0xFFFF, 0xA68C, 0xFFFF, 0xA68E, 0xFFFF, 0xA690, 0xFFFF, 0xA692, 0xFFFF, 0xA694,
    0xFFFF, 0xA696, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_upper_data40[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xA722, 0xFFFF, 0xA724, 0xFFFF, 0xA726, 0xFFFF, 0xA728, 0xFFFF, 0xA72A,
    0xFFFF, 0xA72C, 0xFFFF, 0xA72E, 0xFFFF, 0xFFFF, 0xFFFF, 0xA732, 0xFFFF, 0xA734, 0xFFFF,
    0xA736, 0xFFFF, 0xA738, 0xFFFF, 0xA73A, 0xFFFF, 0xA73C, 0xFFFF, 0xA73E};
static unsigned short unicode_upper_data41[] = {
    0xFFFF, 0xA740, 0xFFFF, 0xA742, 0xFFFF, 0xA744, 0xFFFF, 0xA746, 0xFFFF, 0xA748, 0xFFFF,
    0xA74A, 0xFFFF, 0xA74C, 0xFFFF, 0xA74E, 0xFFFF, 0xA750, 0xFFFF, 0xA752, 0xFFFF, 0xA754,
    0xFFFF, 0xA756, 0xFFFF, 0xA758, 0xFFFF, 0xA75A, 0xFFFF, 0xA75C, 0xFFFF, 0xA75E, 0xFFFF,
    0xA760, 0xFFFF, 0xA762, 0xFFFF, 0xA764, 0xFFFF, 0xA766, 0xFFFF, 0xA768, 0xFFFF, 0xA76A,
    0xFFFF, 0xA76C, 0xFFFF, 0xA76E, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xA779, 0xFFFF, 0xA77B, 0xFFFF, 0xFFFF, 0xA77E};
static unsigned short unicode_upper_data42[] = {
    0xFFFF, 0xA780, 0xFFFF, 0xA782, 0xFFFF, 0xA784, 0xFFFF, 0xA786, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xA78B, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_upper_data43[] = {
    0xFFFF, 0xFF21, 0xFF22, 0xFF23, 0xFF24, 0xFF25, 0xFF26, 0xFF27, 0xFF28, 0xFF29, 0xFF2A,
    0xFF2B, 0xFF2C, 0xFF2D, 0xFF2E, 0xFF2F, 0xFF30, 0xFF31, 0xFF32, 0xFF33, 0xFF34, 0xFF35,
    0xFF36, 0xFF37, 0xFF38, 0xFF39, 0xFF3A, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};

static unsigned short* unicode_upper_data_table[UNICODE_UPPER_BLOCK_COUNT] = {
    unicode_upper_data0,  unicode_upper_data1,  unicode_upper_data2,  unicode_upper_data3,
    unicode_upper_data4,  unicode_upper_data5,  unicode_upper_data6,  unicode_upper_data7,
    unicode_upper_data8,  unicode_upper_data9,  unicode_upper_data10, unicode_upper_data11,
    unicode_upper_data12, unicode_upper_data13, unicode_upper_data14, unicode_upper_data15,
    unicode_upper_data16, unicode_upper_data17, unicode_upper_data18, unicode_upper_data19,
    unicode_upper_data20, unicode_upper_data21, unicode_upper_data22, unicode_upper_data23,
    unicode_upper_data24, unicode_upper_data25, unicode_upper_data26, unicode_upper_data27,
    unicode_upper_data28, unicode_upper_data29, unicode_upper_data30, unicode_upper_data31,
    unicode_upper_data32, unicode_upper_data33, unicode_upper_data34, unicode_upper_data35,
    unicode_upper_data36, unicode_upper_data37, unicode_upper_data38, unicode_upper_data39,
    unicode_upper_data40, unicode_upper_data41, unicode_upper_data42, unicode_upper_data43};
/* Generated by builder. Do not modify. End unicode_upper_tables */

SQLITE_EXPORT u16 sqlite3_unicode_upper(u16 c) {
    u16 index = unicode_upper_indexes[(c) >> UNICODE_UPPER_BLOCK_SHIFT];
    u8 position = (c)&UNICODE_UPPER_BLOCK_MASK;
    u16(p) = (unicode_upper_data_table[index][unicode_upper_positions[index][position]]);
    int l = unicode_upper_positions[index][position + 1] - unicode_upper_positions[index][position];

    return ((l == 1) && ((p) == 0xFFFF)) ? c : p;
}
#endif

#ifdef SQLITE3_UNICODE_TITLE
/* Generated by builder. Do not modify. Start unicode_title_defines */
#define UNICODE_TITLE_BLOCK_SHIFT 6
#define UNICODE_TITLE_BLOCK_MASK ((1 << UNICODE_TITLE_BLOCK_SHIFT) - 1)
#define UNICODE_TITLE_BLOCK_SIZE (1 << UNICODE_TITLE_BLOCK_SHIFT)
#define UNICODE_TITLE_BLOCK_COUNT 44
#define UNICODE_TITLE_INDEXES_SIZE (0x10000 >> UNICODE_TITLE_BLOCK_SHIFT)
/* Generated by builder. Do not modify. End unicode_title_defines */

/* Generated by builder. Do not modify. Start unicode_title_tables */

static unsigned short unicode_title_indexes[UNICODE_TITLE_INDEXES_SIZE] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,  10, 0, 0,  11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 21, 0,  0, 22, 23, 24, 25, 26, 27, 28, 29, 0,  0,  0,  0, 0, 30, 31,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 32, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  33, 34, 35, 36, 37, 0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  38, 39, 0,  40, 41, 42, 0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  43, 0, 0};

static unsigned char unicode_title_positions[UNICODE_TITLE_BLOCK_COUNT][UNICODE_TITLE_BLOCK_SIZE +
                                                                        1] = {
    /* 0 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
             22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
             44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 1 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
             22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
             44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 2 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
             22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
             44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 3 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
             22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
             44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 4 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
             22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
             44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 5 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
             22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
             44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 6 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
             22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
             44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 7 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
             22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
             44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 8 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
             22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
             44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 9 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
             22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
             44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 10 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 11 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 12 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 13 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 14 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 15 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 16 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 17 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 18 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 19 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 20 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 21 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 22 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 23 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 24 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 25 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 26 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 27 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 28 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 29 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 30 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 31 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 32 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 33 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 34 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 35 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 36 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 37 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 38 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 39 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 40 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 41 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 42 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    /* 43 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
              17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
              34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
              51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64}};

static unsigned short unicode_title_data0[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_title_data1[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004A, 0x004B,
    0x004C, 0x004D, 0x004E, 0x004F, 0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056,
    0x0057, 0x0058, 0x0059, 0x005A, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_title_data2[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x039C, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_title_data3[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x00C0,
    0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x00C7, 0x00C8, 0x00C9, 0x00CA, 0x00CB,
    0x00CC, 0x00CD, 0x00CE, 0x00CF, 0x00D0, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D6,
    0xFFFF, 0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x00DD, 0x00DE, 0x0178};
static unsigned short unicode_title_data4[] = {
    0xFFFF, 0x0100, 0xFFFF, 0x0102, 0xFFFF, 0x0104, 0xFFFF, 0x0106, 0xFFFF, 0x0108, 0xFFFF,
    0x010A, 0xFFFF, 0x010C, 0xFFFF, 0x010E, 0xFFFF, 0x0110, 0xFFFF, 0x0112, 0xFFFF, 0x0114,
    0xFFFF, 0x0116, 0xFFFF, 0x0118, 0xFFFF, 0x011A, 0xFFFF, 0x011C, 0xFFFF, 0x011E, 0xFFFF,
    0x0120, 0xFFFF, 0x0122, 0xFFFF, 0x0124, 0xFFFF, 0x0126, 0xFFFF, 0x0128, 0xFFFF, 0x012A,
    0xFFFF, 0x012C, 0xFFFF, 0x012E, 0xFFFF, 0x0049, 0xFFFF, 0x0132, 0xFFFF, 0x0134, 0xFFFF,
    0x0136, 0xFFFF, 0xFFFF, 0x0139, 0xFFFF, 0x013B, 0xFFFF, 0x013D, 0xFFFF};
static unsigned short unicode_title_data5[] = {
    0x013F, 0xFFFF, 0x0141, 0xFFFF, 0x0143, 0xFFFF, 0x0145, 0xFFFF, 0x0147, 0xFFFF, 0xFFFF,
    0x014A, 0xFFFF, 0x014C, 0xFFFF, 0x014E, 0xFFFF, 0x0150, 0xFFFF, 0x0152, 0xFFFF, 0x0154,
    0xFFFF, 0x0156, 0xFFFF, 0x0158, 0xFFFF, 0x015A, 0xFFFF, 0x015C, 0xFFFF, 0x015E, 0xFFFF,
    0x0160, 0xFFFF, 0x0162, 0xFFFF, 0x0164, 0xFFFF, 0x0166, 0xFFFF, 0x0168, 0xFFFF, 0x016A,
    0xFFFF, 0x016C, 0xFFFF, 0x016E, 0xFFFF, 0x0170, 0xFFFF, 0x0172, 0xFFFF, 0x0174, 0xFFFF,
    0x0176, 0xFFFF, 0xFFFF, 0x0179, 0xFFFF, 0x017B, 0xFFFF, 0x017D, 0x0053};
static unsigned short unicode_title_data6[] = {
    0x0243, 0xFFFF, 0xFFFF, 0x0182, 0xFFFF, 0x0184, 0xFFFF, 0xFFFF, 0x0187, 0xFFFF, 0xFFFF,
    0xFFFF, 0x018B, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0191, 0xFFFF, 0xFFFF, 0x01F6,
    0xFFFF, 0xFFFF, 0xFFFF, 0x0198, 0x023D, 0xFFFF, 0xFFFF, 0xFFFF, 0x0220, 0xFFFF, 0xFFFF,
    0x01A0, 0xFFFF, 0x01A2, 0xFFFF, 0x01A4, 0xFFFF, 0xFFFF, 0x01A7, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0x01AC, 0xFFFF, 0xFFFF, 0x01AF, 0xFFFF, 0xFFFF, 0xFFFF, 0x01B3, 0xFFFF, 0x01B5,
    0xFFFF, 0xFFFF, 0x01B8, 0xFFFF, 0xFFFF, 0xFFFF, 0x01BC, 0xFFFF, 0x01F7};
static unsigned short unicode_title_data7[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x01C5, 0x01C5, 0x01C5, 0x01C8, 0x01C8, 0x01C8, 0x01CB,
    0x01CB, 0x01CB, 0xFFFF, 0x01CD, 0xFFFF, 0x01CF, 0xFFFF, 0x01D1, 0xFFFF, 0x01D3, 0xFFFF,
    0x01D5, 0xFFFF, 0x01D7, 0xFFFF, 0x01D9, 0xFFFF, 0x01DB, 0x018E, 0xFFFF, 0x01DE, 0xFFFF,
    0x01E0, 0xFFFF, 0x01E2, 0xFFFF, 0x01E4, 0xFFFF, 0x01E6, 0xFFFF, 0x01E8, 0xFFFF, 0x01EA,
    0xFFFF, 0x01EC, 0xFFFF, 0x01EE, 0xFFFF, 0x01F2, 0x01F2, 0x01F2, 0xFFFF, 0x01F4, 0xFFFF,
    0xFFFF, 0xFFFF, 0x01F8, 0xFFFF, 0x01FA, 0xFFFF, 0x01FC, 0xFFFF, 0x01FE};
static unsigned short unicode_title_data8[] = {
    0xFFFF, 0x0200, 0xFFFF, 0x0202, 0xFFFF, 0x0204, 0xFFFF, 0x0206, 0xFFFF, 0x0208, 0xFFFF,
    0x020A, 0xFFFF, 0x020C, 0xFFFF, 0x020E, 0xFFFF, 0x0210, 0xFFFF, 0x0212, 0xFFFF, 0x0214,
    0xFFFF, 0x0216, 0xFFFF, 0x0218, 0xFFFF, 0x021A, 0xFFFF, 0x021C, 0xFFFF, 0x021E, 0xFFFF,
    0xFFFF, 0xFFFF, 0x0222, 0xFFFF, 0x0224, 0xFFFF, 0x0226, 0xFFFF, 0x0228, 0xFFFF, 0x022A,
    0xFFFF, 0x022C, 0xFFFF, 0x022E, 0xFFFF, 0x0230, 0xFFFF, 0x0232, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x023B, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_title_data9[] = {
    0xFFFF, 0xFFFF, 0x0241, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0246, 0xFFFF, 0x0248, 0xFFFF,
    0x024A, 0xFFFF, 0x024C, 0xFFFF, 0x024E, 0x2C6F, 0x2C6D, 0xFFFF, 0x0181, 0x0186, 0xFFFF,
    0x0189, 0x018A, 0xFFFF, 0x018F, 0xFFFF, 0x0190, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0193,
    0xFFFF, 0xFFFF, 0x0194, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0197, 0x0196, 0xFFFF, 0x2C62,
    0xFFFF, 0xFFFF, 0xFFFF, 0x019C, 0xFFFF, 0x2C6E, 0x019D, 0xFFFF, 0xFFFF, 0x019F, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x2C64, 0xFFFF, 0xFFFF};
static unsigned short unicode_title_data10[] = {
    0x01A6, 0xFFFF, 0xFFFF, 0x01A9, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x01AE, 0x0244, 0x01B1,
    0x01B2, 0x0245, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x01B7, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_title_data11[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0399, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0370, 0xFFFF, 0x0372, 0xFFFF, 0xFFFF, 0xFFFF,
    0x0376, 0xFFFF, 0xFFFF, 0xFFFF, 0x03FD, 0x03FE, 0x03FF, 0xFFFF, 0xFFFF};
static unsigned short unicode_title_data12[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0x0386, 0x0388, 0x0389, 0x038A, 0xFFFF, 0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396,
    0x0397, 0x0398, 0x0399, 0x039A, 0x039B, 0x039C, 0x039D, 0x039E, 0x039F};
static unsigned short unicode_title_data13[] = {
    0x03A0, 0x03A1, 0x03A3, 0x03A3, 0x03A4, 0x03A5, 0x03A6, 0x03A7, 0x03A8, 0x03A9, 0x03AA,
    0x03AB, 0x038C, 0x038E, 0x038F, 0xFFFF, 0x0392, 0x0398, 0xFFFF, 0xFFFF, 0xFFFF, 0x03A6,
    0x03A0, 0x03CF, 0xFFFF, 0x03D8, 0xFFFF, 0x03DA, 0xFFFF, 0x03DC, 0xFFFF, 0x03DE, 0xFFFF,
    0x03E0, 0xFFFF, 0x03E2, 0xFFFF, 0x03E4, 0xFFFF, 0x03E6, 0xFFFF, 0x03E8, 0xFFFF, 0x03EA,
    0xFFFF, 0x03EC, 0xFFFF, 0x03EE, 0x039A, 0x03A1, 0x03F9, 0xFFFF, 0xFFFF, 0x0395, 0xFFFF,
    0xFFFF, 0x03F7, 0xFFFF, 0xFFFF, 0x03FA, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_title_data14[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416,
    0x0417, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F};
static unsigned short unicode_title_data15[] = {
    0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427, 0x0428, 0x0429, 0x042A,
    0x042B, 0x042C, 0x042D, 0x042E, 0x042F, 0x0400, 0x0401, 0x0402, 0x0403, 0x0404, 0x0405,
    0x0406, 0x0407, 0x0408, 0x0409, 0x040A, 0x040B, 0x040C, 0x040D, 0x040E, 0x040F, 0xFFFF,
    0x0460, 0xFFFF, 0x0462, 0xFFFF, 0x0464, 0xFFFF, 0x0466, 0xFFFF, 0x0468, 0xFFFF, 0x046A,
    0xFFFF, 0x046C, 0xFFFF, 0x046E, 0xFFFF, 0x0470, 0xFFFF, 0x0472, 0xFFFF, 0x0474, 0xFFFF,
    0x0476, 0xFFFF, 0x0478, 0xFFFF, 0x047A, 0xFFFF, 0x047C, 0xFFFF, 0x047E};
static unsigned short unicode_title_data16[] = {
    0xFFFF, 0x0480, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0x048A, 0xFFFF, 0x048C, 0xFFFF, 0x048E, 0xFFFF, 0x0490, 0xFFFF, 0x0492, 0xFFFF, 0x0494,
    0xFFFF, 0x0496, 0xFFFF, 0x0498, 0xFFFF, 0x049A, 0xFFFF, 0x049C, 0xFFFF, 0x049E, 0xFFFF,
    0x04A0, 0xFFFF, 0x04A2, 0xFFFF, 0x04A4, 0xFFFF, 0x04A6, 0xFFFF, 0x04A8, 0xFFFF, 0x04AA,
    0xFFFF, 0x04AC, 0xFFFF, 0x04AE, 0xFFFF, 0x04B0, 0xFFFF, 0x04B2, 0xFFFF, 0x04B4, 0xFFFF,
    0x04B6, 0xFFFF, 0x04B8, 0xFFFF, 0x04BA, 0xFFFF, 0x04BC, 0xFFFF, 0x04BE};
static unsigned short unicode_title_data17[] = {
    0xFFFF, 0xFFFF, 0x04C1, 0xFFFF, 0x04C3, 0xFFFF, 0x04C5, 0xFFFF, 0x04C7, 0xFFFF, 0x04C9,
    0xFFFF, 0x04CB, 0xFFFF, 0x04CD, 0x04C0, 0xFFFF, 0x04D0, 0xFFFF, 0x04D2, 0xFFFF, 0x04D4,
    0xFFFF, 0x04D6, 0xFFFF, 0x04D8, 0xFFFF, 0x04DA, 0xFFFF, 0x04DC, 0xFFFF, 0x04DE, 0xFFFF,
    0x04E0, 0xFFFF, 0x04E2, 0xFFFF, 0x04E4, 0xFFFF, 0x04E6, 0xFFFF, 0x04E8, 0xFFFF, 0x04EA,
    0xFFFF, 0x04EC, 0xFFFF, 0x04EE, 0xFFFF, 0x04F0, 0xFFFF, 0x04F2, 0xFFFF, 0x04F4, 0xFFFF,
    0x04F6, 0xFFFF, 0x04F8, 0xFFFF, 0x04FA, 0xFFFF, 0x04FC, 0xFFFF, 0x04FE};
static unsigned short unicode_title_data18[] = {
    0xFFFF, 0x0500, 0xFFFF, 0x0502, 0xFFFF, 0x0504, 0xFFFF, 0x0506, 0xFFFF, 0x0508, 0xFFFF,
    0x050A, 0xFFFF, 0x050C, 0xFFFF, 0x050E, 0xFFFF, 0x0510, 0xFFFF, 0x0512, 0xFFFF, 0x0514,
    0xFFFF, 0x0516, 0xFFFF, 0x0518, 0xFFFF, 0x051A, 0xFFFF, 0x051C, 0xFFFF, 0x051E, 0xFFFF,
    0x0520, 0xFFFF, 0x0522, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_title_data19[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0x0531, 0x0532, 0x0533, 0x0534, 0x0535, 0x0536, 0x0537, 0x0538, 0x0539, 0x053A, 0x053B,
    0x053C, 0x053D, 0x053E, 0x053F, 0x0540, 0x0541, 0x0542, 0x0543, 0x0544, 0x0545, 0x0546,
    0x0547, 0x0548, 0x0549, 0x054A, 0x054B, 0x054C, 0x054D, 0x054E, 0x054F};
static unsigned short unicode_title_data20[] = {
    0x0550, 0x0551, 0x0552, 0x0553, 0x0554, 0x0555, 0x0556, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_title_data21[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xA77D, 0xFFFF, 0xFFFF, 0xFFFF, 0x2C63, 0xFFFF, 0xFFFF};
static unsigned short unicode_title_data22[] = {
    0xFFFF, 0x1E00, 0xFFFF, 0x1E02, 0xFFFF, 0x1E04, 0xFFFF, 0x1E06, 0xFFFF, 0x1E08, 0xFFFF,
    0x1E0A, 0xFFFF, 0x1E0C, 0xFFFF, 0x1E0E, 0xFFFF, 0x1E10, 0xFFFF, 0x1E12, 0xFFFF, 0x1E14,
    0xFFFF, 0x1E16, 0xFFFF, 0x1E18, 0xFFFF, 0x1E1A, 0xFFFF, 0x1E1C, 0xFFFF, 0x1E1E, 0xFFFF,
    0x1E20, 0xFFFF, 0x1E22, 0xFFFF, 0x1E24, 0xFFFF, 0x1E26, 0xFFFF, 0x1E28, 0xFFFF, 0x1E2A,
    0xFFFF, 0x1E2C, 0xFFFF, 0x1E2E, 0xFFFF, 0x1E30, 0xFFFF, 0x1E32, 0xFFFF, 0x1E34, 0xFFFF,
    0x1E36, 0xFFFF, 0x1E38, 0xFFFF, 0x1E3A, 0xFFFF, 0x1E3C, 0xFFFF, 0x1E3E};
static unsigned short unicode_title_data23[] = {
    0xFFFF, 0x1E40, 0xFFFF, 0x1E42, 0xFFFF, 0x1E44, 0xFFFF, 0x1E46, 0xFFFF, 0x1E48, 0xFFFF,
    0x1E4A, 0xFFFF, 0x1E4C, 0xFFFF, 0x1E4E, 0xFFFF, 0x1E50, 0xFFFF, 0x1E52, 0xFFFF, 0x1E54,
    0xFFFF, 0x1E56, 0xFFFF, 0x1E58, 0xFFFF, 0x1E5A, 0xFFFF, 0x1E5C, 0xFFFF, 0x1E5E, 0xFFFF,
    0x1E60, 0xFFFF, 0x1E62, 0xFFFF, 0x1E64, 0xFFFF, 0x1E66, 0xFFFF, 0x1E68, 0xFFFF, 0x1E6A,
    0xFFFF, 0x1E6C, 0xFFFF, 0x1E6E, 0xFFFF, 0x1E70, 0xFFFF, 0x1E72, 0xFFFF, 0x1E74, 0xFFFF,
    0x1E76, 0xFFFF, 0x1E78, 0xFFFF, 0x1E7A, 0xFFFF, 0x1E7C, 0xFFFF, 0x1E7E};
static unsigned short unicode_title_data24[] = {
    0xFFFF, 0x1E80, 0xFFFF, 0x1E82, 0xFFFF, 0x1E84, 0xFFFF, 0x1E86, 0xFFFF, 0x1E88, 0xFFFF,
    0x1E8A, 0xFFFF, 0x1E8C, 0xFFFF, 0x1E8E, 0xFFFF, 0x1E90, 0xFFFF, 0x1E92, 0xFFFF, 0x1E94,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1E60, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0x1EA0, 0xFFFF, 0x1EA2, 0xFFFF, 0x1EA4, 0xFFFF, 0x1EA6, 0xFFFF, 0x1EA8, 0xFFFF, 0x1EAA,
    0xFFFF, 0x1EAC, 0xFFFF, 0x1EAE, 0xFFFF, 0x1EB0, 0xFFFF, 0x1EB2, 0xFFFF, 0x1EB4, 0xFFFF,
    0x1EB6, 0xFFFF, 0x1EB8, 0xFFFF, 0x1EBA, 0xFFFF, 0x1EBC, 0xFFFF, 0x1EBE};
static unsigned short unicode_title_data25[] = {
    0xFFFF, 0x1EC0, 0xFFFF, 0x1EC2, 0xFFFF, 0x1EC4, 0xFFFF, 0x1EC6, 0xFFFF, 0x1EC8, 0xFFFF,
    0x1ECA, 0xFFFF, 0x1ECC, 0xFFFF, 0x1ECE, 0xFFFF, 0x1ED0, 0xFFFF, 0x1ED2, 0xFFFF, 0x1ED4,
    0xFFFF, 0x1ED6, 0xFFFF, 0x1ED8, 0xFFFF, 0x1EDA, 0xFFFF, 0x1EDC, 0xFFFF, 0x1EDE, 0xFFFF,
    0x1EE0, 0xFFFF, 0x1EE2, 0xFFFF, 0x1EE4, 0xFFFF, 0x1EE6, 0xFFFF, 0x1EE8, 0xFFFF, 0x1EEA,
    0xFFFF, 0x1EEC, 0xFFFF, 0x1EEE, 0xFFFF, 0x1EF0, 0xFFFF, 0x1EF2, 0xFFFF, 0x1EF4, 0xFFFF,
    0x1EF6, 0xFFFF, 0x1EF8, 0xFFFF, 0x1EFA, 0xFFFF, 0x1EFC, 0xFFFF, 0x1EFE};
static unsigned short unicode_title_data26[] = {
    0x1F08, 0x1F09, 0x1F0A, 0x1F0B, 0x1F0C, 0x1F0D, 0x1F0E, 0x1F0F, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1F18, 0x1F19, 0x1F1A, 0x1F1B, 0x1F1C, 0x1F1D,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1F28,
    0x1F29, 0x1F2A, 0x1F2B, 0x1F2C, 0x1F2D, 0x1F2E, 0x1F2F, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1F38, 0x1F39, 0x1F3A, 0x1F3B, 0x1F3C, 0x1F3D, 0x1F3E,
    0x1F3F, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_title_data27[] = {
    0x1F48, 0x1F49, 0x1F4A, 0x1F4B, 0x1F4C, 0x1F4D, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1F59, 0xFFFF, 0x1F5B, 0xFFFF, 0x1F5D,
    0xFFFF, 0x1F5F, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1F68,
    0x1F69, 0x1F6A, 0x1F6B, 0x1F6C, 0x1F6D, 0x1F6E, 0x1F6F, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1FBA, 0x1FBB, 0x1FC8, 0x1FC9, 0x1FCA, 0x1FCB, 0x1FDA,
    0x1FDB, 0x1FF8, 0x1FF9, 0x1FEA, 0x1FEB, 0x1FFA, 0x1FFB, 0xFFFF, 0xFFFF};
static unsigned short unicode_title_data28[] = {
    0x1F88, 0x1F89, 0x1F8A, 0x1F8B, 0x1F8C, 0x1F8D, 0x1F8E, 0x1F8F, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1F98, 0x1F99, 0x1F9A, 0x1F9B, 0x1F9C, 0x1F9D,
    0x1F9E, 0x1F9F, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1FA8,
    0x1FA9, 0x1FAA, 0x1FAB, 0x1FAC, 0x1FAD, 0x1FAE, 0x1FAF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1FB8, 0x1FB9, 0xFFFF, 0x1FBC, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0399, 0xFFFF};
static unsigned short unicode_title_data29[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0x1FCC, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1FD8, 0x1FD9, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1FE8,
    0x1FE9, 0xFFFF, 0xFFFF, 0xFFFF, 0x1FEC, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1FFC, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_title_data30[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0x2132, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x2160, 0x2161, 0x2162, 0x2163, 0x2164, 0x2165, 0x2166,
    0x2167, 0x2168, 0x2169, 0x216A, 0x216B, 0x216C, 0x216D, 0x216E, 0x216F};
static unsigned short unicode_title_data31[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x2183, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_title_data32[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x24B6, 0x24B7, 0x24B8, 0x24B9, 0x24BA, 0x24BB,
    0x24BC, 0x24BD, 0x24BE, 0x24BF, 0x24C0, 0x24C1, 0x24C2, 0x24C3, 0x24C4, 0x24C5, 0x24C6,
    0x24C7, 0x24C8, 0x24C9, 0x24CA, 0x24CB, 0x24CC, 0x24CD, 0x24CE, 0x24CF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_title_data33[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x2C00, 0x2C01, 0x2C02, 0x2C03, 0x2C04, 0x2C05, 0x2C06,
    0x2C07, 0x2C08, 0x2C09, 0x2C0A, 0x2C0B, 0x2C0C, 0x2C0D, 0x2C0E, 0x2C0F};
static unsigned short unicode_title_data34[] = {
    0x2C10, 0x2C11, 0x2C12, 0x2C13, 0x2C14, 0x2C15, 0x2C16, 0x2C17, 0x2C18, 0x2C19, 0x2C1A,
    0x2C1B, 0x2C1C, 0x2C1D, 0x2C1E, 0x2C1F, 0x2C20, 0x2C21, 0x2C22, 0x2C23, 0x2C24, 0x2C25,
    0x2C26, 0x2C27, 0x2C28, 0x2C29, 0x2C2A, 0x2C2B, 0x2C2C, 0x2C2D, 0x2C2E, 0xFFFF, 0xFFFF,
    0x2C60, 0xFFFF, 0xFFFF, 0xFFFF, 0x023A, 0x023E, 0xFFFF, 0x2C67, 0xFFFF, 0x2C69, 0xFFFF,
    0x2C6B, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x2C72, 0xFFFF, 0xFFFF, 0x2C75,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_title_data35[] = {
    0xFFFF, 0x2C80, 0xFFFF, 0x2C82, 0xFFFF, 0x2C84, 0xFFFF, 0x2C86, 0xFFFF, 0x2C88, 0xFFFF,
    0x2C8A, 0xFFFF, 0x2C8C, 0xFFFF, 0x2C8E, 0xFFFF, 0x2C90, 0xFFFF, 0x2C92, 0xFFFF, 0x2C94,
    0xFFFF, 0x2C96, 0xFFFF, 0x2C98, 0xFFFF, 0x2C9A, 0xFFFF, 0x2C9C, 0xFFFF, 0x2C9E, 0xFFFF,
    0x2CA0, 0xFFFF, 0x2CA2, 0xFFFF, 0x2CA4, 0xFFFF, 0x2CA6, 0xFFFF, 0x2CA8, 0xFFFF, 0x2CAA,
    0xFFFF, 0x2CAC, 0xFFFF, 0x2CAE, 0xFFFF, 0x2CB0, 0xFFFF, 0x2CB2, 0xFFFF, 0x2CB4, 0xFFFF,
    0x2CB6, 0xFFFF, 0x2CB8, 0xFFFF, 0x2CBA, 0xFFFF, 0x2CBC, 0xFFFF, 0x2CBE};
static unsigned short unicode_title_data36[] = {
    0xFFFF, 0x2CC0, 0xFFFF, 0x2CC2, 0xFFFF, 0x2CC4, 0xFFFF, 0x2CC6, 0xFFFF, 0x2CC8, 0xFFFF,
    0x2CCA, 0xFFFF, 0x2CCC, 0xFFFF, 0x2CCE, 0xFFFF, 0x2CD0, 0xFFFF, 0x2CD2, 0xFFFF, 0x2CD4,
    0xFFFF, 0x2CD6, 0xFFFF, 0x2CD8, 0xFFFF, 0x2CDA, 0xFFFF, 0x2CDC, 0xFFFF, 0x2CDE, 0xFFFF,
    0x2CE0, 0xFFFF, 0x2CE2, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_title_data37[] = {
    0x10A0, 0x10A1, 0x10A2, 0x10A3, 0x10A4, 0x10A5, 0x10A6, 0x10A7, 0x10A8, 0x10A9, 0x10AA,
    0x10AB, 0x10AC, 0x10AD, 0x10AE, 0x10AF, 0x10B0, 0x10B1, 0x10B2, 0x10B3, 0x10B4, 0x10B5,
    0x10B6, 0x10B7, 0x10B8, 0x10B9, 0x10BA, 0x10BB, 0x10BC, 0x10BD, 0x10BE, 0x10BF, 0x10C0,
    0x10C1, 0x10C2, 0x10C3, 0x10C4, 0x10C5, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_title_data38[] = {
    0xFFFF, 0xA640, 0xFFFF, 0xA642, 0xFFFF, 0xA644, 0xFFFF, 0xA646, 0xFFFF, 0xA648, 0xFFFF,
    0xA64A, 0xFFFF, 0xA64C, 0xFFFF, 0xA64E, 0xFFFF, 0xA650, 0xFFFF, 0xA652, 0xFFFF, 0xA654,
    0xFFFF, 0xA656, 0xFFFF, 0xA658, 0xFFFF, 0xA65A, 0xFFFF, 0xA65C, 0xFFFF, 0xA65E, 0xFFFF,
    0xFFFF, 0xFFFF, 0xA662, 0xFFFF, 0xA664, 0xFFFF, 0xA666, 0xFFFF, 0xA668, 0xFFFF, 0xA66A,
    0xFFFF, 0xA66C, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_title_data39[] = {
    0xFFFF, 0xA680, 0xFFFF, 0xA682, 0xFFFF, 0xA684, 0xFFFF, 0xA686, 0xFFFF, 0xA688, 0xFFFF,
    0xA68A, 0xFFFF, 0xA68C, 0xFFFF, 0xA68E, 0xFFFF, 0xA690, 0xFFFF, 0xA692, 0xFFFF, 0xA694,
    0xFFFF, 0xA696, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_title_data40[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xA722, 0xFFFF, 0xA724, 0xFFFF, 0xA726, 0xFFFF, 0xA728, 0xFFFF, 0xA72A,
    0xFFFF, 0xA72C, 0xFFFF, 0xA72E, 0xFFFF, 0xFFFF, 0xFFFF, 0xA732, 0xFFFF, 0xA734, 0xFFFF,
    0xA736, 0xFFFF, 0xA738, 0xFFFF, 0xA73A, 0xFFFF, 0xA73C, 0xFFFF, 0xA73E};
static unsigned short unicode_title_data41[] = {
    0xFFFF, 0xA740, 0xFFFF, 0xA742, 0xFFFF, 0xA744, 0xFFFF, 0xA746, 0xFFFF, 0xA748, 0xFFFF,
    0xA74A, 0xFFFF, 0xA74C, 0xFFFF, 0xA74E, 0xFFFF, 0xA750, 0xFFFF, 0xA752, 0xFFFF, 0xA754,
    0xFFFF, 0xA756, 0xFFFF, 0xA758, 0xFFFF, 0xA75A, 0xFFFF, 0xA75C, 0xFFFF, 0xA75E, 0xFFFF,
    0xA760, 0xFFFF, 0xA762, 0xFFFF, 0xA764, 0xFFFF, 0xA766, 0xFFFF, 0xA768, 0xFFFF, 0xA76A,
    0xFFFF, 0xA76C, 0xFFFF, 0xA76E, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xA779, 0xFFFF, 0xA77B, 0xFFFF, 0xFFFF, 0xA77E};
static unsigned short unicode_title_data42[] = {
    0xFFFF, 0xA780, 0xFFFF, 0xA782, 0xFFFF, 0xA784, 0xFFFF, 0xA786, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xA78B, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_title_data43[] = {
    0xFFFF, 0xFF21, 0xFF22, 0xFF23, 0xFF24, 0xFF25, 0xFF26, 0xFF27, 0xFF28, 0xFF29, 0xFF2A,
    0xFF2B, 0xFF2C, 0xFF2D, 0xFF2E, 0xFF2F, 0xFF30, 0xFF31, 0xFF32, 0xFF33, 0xFF34, 0xFF35,
    0xFF36, 0xFF37, 0xFF38, 0xFF39, 0xFF3A, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};

static unsigned short* unicode_title_data_table[UNICODE_TITLE_BLOCK_COUNT] = {
    unicode_title_data0,  unicode_title_data1,  unicode_title_data2,  unicode_title_data3,
    unicode_title_data4,  unicode_title_data5,  unicode_title_data6,  unicode_title_data7,
    unicode_title_data8,  unicode_title_data9,  unicode_title_data10, unicode_title_data11,
    unicode_title_data12, unicode_title_data13, unicode_title_data14, unicode_title_data15,
    unicode_title_data16, unicode_title_data17, unicode_title_data18, unicode_title_data19,
    unicode_title_data20, unicode_title_data21, unicode_title_data22, unicode_title_data23,
    unicode_title_data24, unicode_title_data25, unicode_title_data26, unicode_title_data27,
    unicode_title_data28, unicode_title_data29, unicode_title_data30, unicode_title_data31,
    unicode_title_data32, unicode_title_data33, unicode_title_data34, unicode_title_data35,
    unicode_title_data36, unicode_title_data37, unicode_title_data38, unicode_title_data39,
    unicode_title_data40, unicode_title_data41, unicode_title_data42, unicode_title_data43};
/* Generated by builder. Do not modify. End unicode_title_tables */

SQLITE_EXPORT u16 sqlite3_unicode_title(u16 c) {
    u16 index = unicode_title_indexes[(c) >> UNICODE_TITLE_BLOCK_SHIFT];
    u8 position = (c)&UNICODE_TITLE_BLOCK_MASK;
    u16(p) = (unicode_title_data_table[index][unicode_title_positions[index][position]]);
    int l = unicode_title_positions[index][position + 1] - unicode_title_positions[index][position];

    return ((l == 1) && ((p) == 0xFFFF)) ? c : p;
}
#endif

#ifdef SQLITE3_UNICODE_UNACC
/* Generated by builder. Do not modify. Start unicode_unacc_defines */
#define UNICODE_UNACC_BLOCK_SHIFT 5
#define UNICODE_UNACC_BLOCK_MASK ((1 << UNICODE_UNACC_BLOCK_SHIFT) - 1)
#define UNICODE_UNACC_BLOCK_SIZE (1 << UNICODE_UNACC_BLOCK_SHIFT)
#define UNICODE_UNACC_BLOCK_COUNT 239
#define UNICODE_UNACC_INDEXES_SIZE (0x10000 >> UNICODE_UNACC_BLOCK_SHIFT)
/* Generated by builder. Do not modify. End unicode_unacc_defines */

/* Generated by builder. Do not modify. Start unicode_unacc_tables */

static unsigned short unicode_unacc_indexes[UNICODE_UNACC_INDEXES_SIZE] = {
    0,   0,   0,   0,   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,
    15,  16,  17,  18,  19,  0,   0,   0,   20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,
    31,  32,  33,  34,  0,   0,   35,  0,   0,   0,   0,   36,  0,   37,  38,  39,  40,  41,  0,
    0,   42,  43,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   44,  45,  0,
    0,   0,   46,  47,  0,   48,  49,  0,   0,   0,   0,   0,   0,   0,   50,  0,   51,  0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   52,
    0,   0,   0,   53,  54,  0,   55,  0,   56,  57,  0,   0,   0,   0,   0,   58,  0,   0,   0,
    0,   0,   59,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   60,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   61,  62,  63,  64,  65,  66,  0,   0,   67,  68,  69,  70,  71,  72,  73,
    74,  75,  76,  77,  78,  79,  80,  81,  82,  83,  84,  85,  86,  87,  88,  0,   0,   89,  90,
    91,  92,  93,  94,  95,  96,  97,  98,  99,  100, 101, 102, 0,   103, 0,   104, 0,   105, 106,
    0,   107, 108, 0,   0,   0,   109, 110, 111, 112, 113, 0,   0,   0,   0,   0,   114, 0,   115,
    116, 0,   0,   0,   117, 0,   0,   0,   0,   0,   0,   0,   0,   0,   118, 119, 0,   0,   0,
    0,   0,   0,   0,   0,   120, 121, 122, 0,   123, 124, 125, 126, 127, 128, 129, 130, 131, 132,
    133, 134, 135, 136, 0,   0,   0,   0,   0,   0,   0,   137, 138, 139, 0,   0,   0,   0,   0,
    0,   0,   140, 0,   0,   0,   0,   141, 0,   0,   0,   142, 0,   0,   143, 144, 145, 146, 147,
    148, 149, 150, 0,   151, 152, 153, 154, 155, 156, 157, 158, 0,   159, 160, 161, 162, 0,   0,
    0,   163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    179, 0,   180, 0,   0,   0,   0,   181, 182, 183, 0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   184, 185, 186,
    187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 0,   199, 200, 201, 202, 203, 204,
    205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
    224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238};

static unsigned char
    unicode_unacc_positions[UNICODE_UNACC_BLOCK_COUNT][UNICODE_UNACC_BLOCK_SIZE + 1] = {
        /* 0 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 1 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 31, 34, 37, 38},
        /* 2 */ {0,  1,  2,  3,  4,  5,  6,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17,
                 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33},
        /* 3 */ {0,  1,  2,  3,  4,  5,  6,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17,
                 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33},
        /* 4 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 5 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                 17, 18, 20, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 35},
        /* 6 */ {0,  2,  3,  4,  5,  6,  7,  8,  9,  10, 12, 13, 14, 15, 16, 17, 18,
                 19, 20, 22, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36},
        /* 7 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 8 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 9 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 10 */ {0,  1,  2,  3,  4,  6,  8,  10, 12, 14, 16, 18, 20, 22, 23, 24, 25,
                  26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41},
        /* 11 */ {0,  1,  2,  4,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18,
                  19, 21, 23, 25, 26, 27, 28, 29, 30, 31, 32, 33, 35, 37, 38, 39},
        /* 12 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 13 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 14 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 15 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 16 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 17 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 18 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 19 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 20 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 21 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 22 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 23 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 24 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 25 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 26 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 27 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 28 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 29 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 30 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 31 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 32 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 33 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 34 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 35 */ {0,  1,  2,  3,  4,  5,  6,  7,  9,  10, 11, 12, 13, 14, 15, 16, 17,
                  18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33},
        /* 36 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 37 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 23, 25, 27, 29, 30, 31, 32, 33, 34, 35, 36},
        /* 38 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 39 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 40 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 41 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 42 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 43 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 44 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 45 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 46 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 47 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 48 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 49 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 50 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 51 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 52 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 53 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 54 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 30, 32, 33, 34},
        /* 55 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 56 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 57 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 58 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 59 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 60 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 61 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 62 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 15, 16, 17,
                  18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33},
        /* 63 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 64 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 65 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 66 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 67 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 68 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 69 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 70 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 71 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 28, 29, 30, 31, 32, 33},
        /* 72 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 73 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 74 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 75 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 76 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 77 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 78 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 79 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 80 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 81 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 82 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 83 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 84 */ {0,  1,  2,  3,  4,  5,  7,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
                  20, 21, 22, 24, 27, 28, 30, 33, 34, 35, 36, 37, 39, 40, 41, 42},
        /* 85 */ {0,  1,  2,  3,  4,  5,  6,  7,  9,  11, 13, 14, 15, 16, 17, 18, 19,
                  20, 21, 22, 23, 24, 25, 26, 30, 31, 32, 33, 34, 35, 36, 37, 38},
        /* 86 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 87 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 88 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  10, 11, 12, 13, 14, 15, 16, 17,
                  18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33},
        /* 89 */ {0,  3,  6,  7,  9,  10, 13, 16, 17, 18, 20, 21, 22, 23, 24, 25, 26,
                  27, 28, 29, 30, 31, 32, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43},
        /* 90 */ {0,  2,  5,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
                  21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 34, 35, 36, 37, 38},
        /* 91 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 22, 25, 28, 31, 34, 37, 40, 43, 46, 49, 52, 55, 57},
        /* 92 */ {0,  1,  3,  6,  8,  9,  11, 14, 18, 20, 21, 23, 26, 27, 28, 29, 30,
                  31, 33, 36, 38, 39, 41, 44, 48, 50, 51, 53, 56, 57, 58, 59, 60},
        /* 93 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 94 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 95 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 96 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 97 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 98 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 14, 17, 18, 20,
                  23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38},
        /* 99 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 100 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 101 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 102 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 103 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 104 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 105 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 106 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 107 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 108 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 109 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  11, 13, 15, 17, 19, 21, 23,
                   25, 27, 29, 31, 34, 37, 40, 43, 46, 49, 52, 55, 58, 62, 66, 70},
        /* 110 */ {0,  4,  8,  12, 16, 20, 24, 28, 32, 34, 36, 38, 40, 42, 44, 46, 48,
                   50, 53, 56, 59, 62, 65, 68, 71, 74, 77, 80, 83, 86, 89, 92, 95},
        /* 111 */ {0,  3,  6,  9,  12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 48,
                   51, 54, 57, 60, 63, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76},
        /* 112 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 113 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 114 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 115 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 116 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 117 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 118 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 119 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 120 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 121 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 122 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 123 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 124 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 125 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 126 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 127 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 16, 17, 18, 19,
                   20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35},
        /* 128 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 129 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 130 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 23, 25, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37},
        /* 131 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 132 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 133 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 134 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 135 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 136 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 137 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 138 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 139 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 140 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 141 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 142 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 143 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 144 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 145 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 146 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 147 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 148 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 149 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 150 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 151 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 152 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 153 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 154 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 155 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 33},
        /* 156 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 157 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 158 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 33},
        /* 159 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 160 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 161 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 162 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 163 */ {0,  3,  6,  9,  12, 15, 18, 21, 24, 27, 30, 33, 36,  39,  42,  46, 50,
                   54, 58, 62, 66, 70, 74, 78, 82, 86, 90, 94, 98, 102, 109, 115, 116},
        /* 164 */ {0,  3,  6,  9,  12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 48,
                   51, 54, 57, 60, 63, 66, 69, 72, 75, 78, 81, 84, 87, 90, 93, 96},
        /* 165 */ {0,  3,  6,  9,  12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
                   27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51, 53, 55, 57},
        /* 166 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 16, 18,
                   20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 47, 51, 53, 54},
        /* 167 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 168 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47},
        /* 169 */ {0,  2,  4,  6,  8,  10, 12, 14, 16, 18, 21, 24, 27, 29, 32, 34, 37,
                   38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53},
        /* 170 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 171 */ {0,  4,  8,  12, 15, 19, 22, 25, 30, 34, 37,  40,  43,  47,  51,  54, 57,
                   59, 62, 66, 70, 72, 77, 83, 88, 91, 96, 101, 105, 108, 111, 114, 118},
        /* 172 */ {0,  5,  9,  12, 15, 18, 20, 22, 24, 26, 29, 32, 37, 40, 44,  49, 52,
                   54, 56, 61, 65, 70, 73, 78, 80, 83, 86, 89, 92, 95, 99, 102, 104},
        /* 173 */ {0,  3,  6,  9,  13, 16, 19, 22, 27, 31, 33, 38, 40, 44, 48, 51, 54,
                   57, 61, 63, 66, 70, 72, 77, 80, 82, 84, 86, 88, 90, 92, 94, 96},
        /* 174 */ {0,  2,  4,  7,  10, 13, 16, 19, 22, 25, 28, 31, 34, 37, 40, 43, 46,
                   49, 52, 54, 56, 59, 61, 63, 65, 68, 71, 73, 75, 77, 79, 81, 85},
        /* 175 */ {0,  2,  4,  6,  8,  10, 12, 14, 16, 19, 23, 25, 27, 29, 31, 33, 35,
                   37, 40, 43, 46, 49, 51, 53, 55, 57, 59, 61, 63, 65, 67, 69, 72},
        /* 176 */ {0,  3,  5,  8,  11, 14, 16, 19, 22, 26, 28, 31, 34, 37, 40, 45, 51,
                   53, 55, 57, 59, 61, 63, 65, 67, 69, 71, 73, 75, 77, 79, 81, 83},
        /* 177 */ {0,  2,  4,  8,  10, 12, 14, 18, 21, 23, 25, 27, 29, 31, 33, 35, 37,
                   39, 41, 44, 46, 48, 51, 54, 56, 60, 63, 65, 67, 69, 71, 74, 77},
        /* 178 */ {0,  2,  4,  6,  8,  10, 12, 14, 16, 18, 21, 24, 27, 30, 33, 36, 39,
                   42, 45, 48, 51, 54, 57, 60, 63, 66, 69, 72, 75, 78, 81, 84, 87},
        /* 179 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 180 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 181 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 182 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 183 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 184 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 185 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 186 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 187 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 188 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 189 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 190 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 191 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 192 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 193 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 194 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 195 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 196 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 197 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 198 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 199 */ {0,  2,  4,  6,  9,  12, 14, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
                   26, 27, 28, 30, 32, 34, 36, 38, 39, 40, 41, 42, 43, 44, 45, 46},
        /* 200 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 201 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 17,
                   18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33},
        /* 202 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 203 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 204 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 205 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 31, 32, 33},
        /* 206 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 12, 14, 16, 18, 20, 22,
                   24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 47, 48, 49, 50},
        /* 207 */ {0,  2,  4,  6,  8,  10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32,
                   34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64},
        /* 208 */ {0,  2,  4,  6,  8,  10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32,
                   34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64},
        /* 209 */ {0,  2,  4,  6,  8,  10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32,
                   34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 55, 56, 57, 58, 59},
        /* 210 */ {0,  1,  2,  3,  4,  6,  8,  10, 12, 14, 16, 18, 20, 22, 24, 26, 28,
                   30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60},
        /* 211 */ {0,  2,  4,  6,  8,  10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32,
                   33, 35, 37, 39, 41, 43, 45, 47, 49, 51, 53, 55, 57, 59, 61, 63},
        /* 212 */ {0,  2,  4,  6,  8,  10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32,
                   34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64},
        /* 213 */ {0,  2,  4,  6,  8,  10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32,
                   34, 36, 38, 40, 42, 44, 46, 48, 50, 51, 53, 55, 57, 59, 61, 63},
        /* 214 */ {0,  2,  4,  6,  8,  10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32,
                   34, 36, 37, 38, 39, 41, 43, 45, 47, 49, 51, 53, 55, 57, 59, 61},
        /* 215 */ {0,  2,  4,  6,  8,  10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32,
                   34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64},
        /* 216 */ {0,  2,  4,  6,  8,  10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32,
                   34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 57, 58, 59, 60},
        /* 217 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   19, 22, 25, 28, 31, 34, 37, 40, 43, 46, 49, 52, 55, 58, 61, 64},
        /* 218 */ {0,  3,  6,  9,  12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 48,
                   51, 54, 57, 60, 63, 66, 69, 72, 75, 78, 81, 84, 87, 90, 93, 96},
        /* 219 */ {0,  3,  6,  9,  12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 48,
                   49, 50, 53, 56, 59, 62, 65, 68, 71, 74, 77, 80, 83, 86, 89, 92},
        /* 220 */ {0,  3,  6,  9,  12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 48,
                   51, 54, 57, 60, 63, 66, 69, 72, 75, 78, 81, 84, 87, 90, 93, 96},
        /* 221 */ {0,  3,  6,  9,  12, 15, 18, 21, 24, 25, 26, 27, 28, 29, 30, 31, 32,
                   33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48},
        /* 222 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   19, 22, 26, 30, 34, 38, 42, 46, 50, 53, 71, 79, 83, 84, 85, 86},
        /* 223 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 28, 29, 30, 31, 32, 33, 34},
        /* 224 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33},
        /* 225 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 226 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 227 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 228 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 229 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 230 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 23, 25, 27, 29, 31, 33, 35, 37, 38, 39, 40},
        /* 231 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 232 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 233 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 234 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 235 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 236 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 237 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
        /* 238 */ {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32}};

static unsigned short unicode_unacc_data0[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data1[] = {
    0x0020, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0020, 0xFFFF,
    0x0061, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0020, 0xFFFF, 0xFFFF, 0x0032, 0x0033,
    0x0020, 0x03BC, 0xFFFF, 0xFFFF, 0x0020, 0x0031, 0x006F, 0xFFFF, 0x0031, 0x2044,
    0x0034, 0x0031, 0x2044, 0x0032, 0x0033, 0x2044, 0x0034, 0xFFFF};
static unsigned short unicode_unacc_data2[] = {
    0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0045, 0x0043, 0x0045, 0x0045,
    0x0045, 0x0045, 0x0049, 0x0049, 0x0049, 0x0049, 0xFFFF, 0x004E, 0x004F, 0x004F, 0x004F,
    0x004F, 0x004F, 0xFFFF, 0x004F, 0x0055, 0x0055, 0x0055, 0x0055, 0x0059, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data3[] = {
    0x0061, 0x0061, 0x0061, 0x0061, 0x0061, 0x0061, 0x0061, 0x0065, 0x0063, 0x0065, 0x0065,
    0x0065, 0x0065, 0x0069, 0x0069, 0x0069, 0x0069, 0xFFFF, 0x006E, 0x006F, 0x006F, 0x006F,
    0x006F, 0x006F, 0xFFFF, 0x006F, 0x0075, 0x0075, 0x0075, 0x0075, 0x0079, 0xFFFF, 0x0079};
static unsigned short unicode_unacc_data4[] = {
    0x0041, 0x0061, 0x0041, 0x0061, 0x0041, 0x0061, 0x0043, 0x0063, 0x0043, 0x0063, 0x0043,
    0x0063, 0x0043, 0x0063, 0x0044, 0x0064, 0x0044, 0x0064, 0x0045, 0x0065, 0x0045, 0x0065,
    0x0045, 0x0065, 0x0045, 0x0065, 0x0045, 0x0065, 0x0047, 0x0067, 0x0047, 0x0067};
static unsigned short unicode_unacc_data5[] = {
    0x0047, 0x0067, 0x0047, 0x0067, 0x0048, 0x0068, 0x0048, 0x0068, 0x0049, 0x0069, 0x0049, 0x0069,
    0x0049, 0x0069, 0x0049, 0x0069, 0x0049, 0xFFFF, 0x0049, 0x004A, 0x0069, 0x006A, 0x004A, 0x006A,
    0x004B, 0x006B, 0xFFFF, 0x004C, 0x006C, 0x004C, 0x006C, 0x004C, 0x006C, 0x004C, 0x00B7};
static unsigned short unicode_unacc_data6[] = {
    0x006C, 0x00B7, 0x004C, 0x006C, 0x004E, 0x006E, 0x004E, 0x006E, 0x004E, 0x006E, 0x02BC, 0x006E,
    0xFFFF, 0xFFFF, 0x004F, 0x006F, 0x004F, 0x006F, 0x004F, 0x006F, 0x004F, 0x0045, 0x006F, 0x0065,
    0x0052, 0x0072, 0x0052, 0x0072, 0x0052, 0x0072, 0x0053, 0x0073, 0x0053, 0x0073, 0x0053, 0x0073};
static unsigned short unicode_unacc_data7[] = {
    0x0053, 0x0073, 0x0054, 0x0074, 0x0054, 0x0074, 0x0054, 0x0074, 0x0055, 0x0075, 0x0055,
    0x0075, 0x0055, 0x0075, 0x0055, 0x0075, 0x0055, 0x0075, 0x0055, 0x0075, 0x0057, 0x0077,
    0x0059, 0x0079, 0x0059, 0x005A, 0x007A, 0x005A, 0x007A, 0x005A, 0x007A, 0x0073};
static unsigned short unicode_unacc_data8[] = {
    0x0062, 0x0042, 0x0042, 0x0062, 0xFFFF, 0xFFFF, 0xFFFF, 0x0043, 0x0063, 0xFFFF, 0x0044,
    0x0044, 0x0064, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0046, 0x0066, 0x0047, 0xFFFF, 0xFFFF,
    0xFFFF, 0x0049, 0x004B, 0x006B, 0x006C, 0xFFFF, 0xFFFF, 0x004E, 0x006E, 0x004F};
static unsigned short unicode_unacc_data9[] = {
    0x004F, 0x006F, 0xFFFF, 0xFFFF, 0x0050, 0x0070, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0x0074, 0x0054, 0x0074, 0x0054, 0x0055, 0x0075, 0xFFFF, 0x0056, 0x0059, 0x0079, 0x005A,
    0x007A, 0xFFFF, 0xFFFF, 0xFFFF, 0x0292, 0xFFFF, 0xFFFF, 0xFFFF, 0x0296, 0xFFFF};
static unsigned short unicode_unacc_data10[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0044, 0x005A, 0x0044, 0x007A, 0x0064, 0x007A, 0x004C,
    0x004A, 0x004C, 0x006A, 0x006C, 0x006A, 0x004E, 0x004A, 0x004E, 0x006A, 0x006E, 0x006A,
    0x0041, 0x0061, 0x0049, 0x0069, 0x004F, 0x006F, 0x0055, 0x0075, 0x0055, 0x0075, 0x0055,
    0x0075, 0x0055, 0x0075, 0x0055, 0x0075, 0xFFFF, 0x0041, 0x0061};
static unsigned short unicode_unacc_data11[] = {
    0x0041, 0x0061, 0x0041, 0x0045, 0x0061, 0x0065, 0x0047, 0x0067, 0x0047, 0x0067,
    0x004B, 0x006B, 0x004F, 0x006F, 0x004F, 0x006F, 0x01B7, 0x0292, 0x006A, 0x0044,
    0x005A, 0x0044, 0x007A, 0x0064, 0x007A, 0x0047, 0x0067, 0xFFFF, 0xFFFF, 0x004E,
    0x006E, 0x0041, 0x0061, 0x0041, 0x0045, 0x0061, 0x0065, 0x004F, 0x006F};
static unsigned short unicode_unacc_data12[] = {
    0x0041, 0x0061, 0x0041, 0x0061, 0x0045, 0x0065, 0x0045, 0x0065, 0x0049, 0x0069, 0x0049,
    0x0069, 0x004F, 0x006F, 0x004F, 0x006F, 0x0052, 0x0072, 0x0052, 0x0072, 0x0055, 0x0075,
    0x0055, 0x0075, 0x0053, 0x0073, 0x0054, 0x0074, 0xFFFF, 0xFFFF, 0x0048, 0x0068};
static unsigned short unicode_unacc_data13[] = {
    0x004E, 0x0064, 0xFFFF, 0xFFFF, 0x005A, 0x007A, 0x0041, 0x0061, 0x0045, 0x0065, 0x004F,
    0x006F, 0x004F, 0x006F, 0x004F, 0x006F, 0x004F, 0x006F, 0x0059, 0x0079, 0x006C, 0x006E,
    0x0074, 0xFFFF, 0xFFFF, 0xFFFF, 0x0041, 0x0043, 0x0063, 0x004C, 0x0054, 0x0073};
static unsigned short unicode_unacc_data14[] = {
    0x007A, 0xFFFF, 0xFFFF, 0x0042, 0xFFFF, 0xFFFF, 0x0045, 0x0065, 0x004A, 0x006A, 0xFFFF,
    0x0071, 0x0052, 0x0072, 0x0059, 0x0079, 0xFFFF, 0xFFFF, 0xFFFF, 0x0062, 0xFFFF, 0x0063,
    0x0064, 0x0064, 0xFFFF, 0xFFFF, 0x0259, 0xFFFF, 0xFFFF, 0x025C, 0xFFFF, 0x0237};
static unsigned short unicode_unacc_data15[] = {
    0x0067, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0068, 0xA727, 0x0069, 0xFFFF, 0xFFFF,
    0x006C, 0x006C, 0x006C, 0xFFFF, 0xFFFF, 0x026F, 0x006D, 0x006E, 0x006E, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0279, 0x0279, 0x0072, 0x0072, 0x0072, 0xFFFF};
static unsigned short unicode_unacc_data16[] = {
    0xFFFF, 0xFFFF, 0x0073, 0xFFFF, 0x0237, 0xFFFF, 0x0283, 0xFFFF, 0x0074, 0xFFFF, 0xFFFF,
    0x0076, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x007A, 0x007A, 0xFFFF, 0x0292, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0262, 0xFFFF, 0x006A, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data17[] = {
    0x0071, 0x0294, 0xFFFF, 0xFFFF, 0xFFFF, 0x02A3, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0x0265, 0x0265, 0x0068, 0x0068, 0x006A, 0x0072, 0x0279, 0x0279,
    0x0281, 0x0077, 0x0079, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data18[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data19[] = {
    0x0263, 0x006C, 0x0073, 0x0078, 0x0295, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data20[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x02B9, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0020, 0xFFFF, 0xFFFF, 0xFFFF, 0x003B, 0xFFFF};
static unsigned short unicode_unacc_data21[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0020, 0x0020, 0x0391, 0x00B7, 0x0395, 0x0397, 0x0399,
    0xFFFF, 0x039F, 0xFFFF, 0x03A5, 0x03A9, 0x03B9, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data22[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0399,
    0x03A5, 0x03B1, 0x03B5, 0x03B7, 0x03B9, 0x03C5, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data23[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x03B9,
    0x03C5, 0x03BF, 0x03C5, 0x03C9, 0xFFFF, 0x03B2, 0x03B8, 0x03A5, 0x03A5, 0x03A5, 0x03C6,
    0x03C0, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data24[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x03BA, 0x03C1, 0x03C2, 0xFFFF, 0x0398, 0x03B5,
    0xFFFF, 0xFFFF, 0xFFFF, 0x03A3, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data25[] = {
    0x0415, 0x0415, 0xFFFF, 0x0413, 0xFFFF, 0xFFFF, 0xFFFF, 0x0406, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0x041A, 0x0418, 0x0423, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0x0418, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data26[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0x0438, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data27[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0435, 0x0435, 0xFFFF, 0x0433, 0xFFFF, 0xFFFF,
    0xFFFF, 0x0456, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x043A, 0x0438, 0x0443, 0xFFFF};
static unsigned short unicode_unacc_data28[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0x0474, 0x0475, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0460, 0x0461, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data29[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0418,
    0x0438, 0xFFFF, 0xFFFF, 0x0420, 0x0440, 0x0413, 0x0433, 0x0413, 0x0433, 0x0413, 0x0433,
    0x0416, 0x0436, 0x0417, 0x0437, 0x041A, 0x043A, 0x041A, 0x043A, 0x041A, 0x043A};
static unsigned short unicode_unacc_data30[] = {
    0xFFFF, 0xFFFF, 0x041D, 0x043D, 0xFFFF, 0xFFFF, 0x041F, 0x043F, 0xFFFF, 0xFFFF, 0x0421,
    0x0441, 0x0422, 0x0442, 0xFFFF, 0xFFFF, 0x04AE, 0x04AF, 0x0425, 0x0445, 0xFFFF, 0xFFFF,
    0x0427, 0x0447, 0x0427, 0x0447, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x04BC, 0x04BD};
static unsigned short unicode_unacc_data31[] = {
    0xFFFF, 0x0416, 0x0436, 0x041A, 0x043A, 0x041B, 0x043B, 0x041D, 0x043D, 0x041D, 0x043D,
    0xFFFF, 0xFFFF, 0x041C, 0x043C, 0xFFFF, 0x0410, 0x0430, 0x0410, 0x0430, 0xFFFF, 0xFFFF,
    0x0415, 0x0435, 0xFFFF, 0xFFFF, 0x04D8, 0x04D9, 0x0416, 0x0436, 0x0417, 0x0437};
static unsigned short unicode_unacc_data32[] = {
    0xFFFF, 0xFFFF, 0x0418, 0x0438, 0x0418, 0x0438, 0x041E, 0x043E, 0xFFFF, 0xFFFF, 0x04E8,
    0x04E9, 0x042D, 0x044D, 0x0423, 0x0443, 0x0423, 0x0443, 0x0423, 0x0443, 0x0427, 0x0447,
    0x0413, 0x0433, 0x042B, 0x044B, 0x0413, 0x0433, 0x0425, 0x0445, 0x0425, 0x0445};
static unsigned short unicode_unacc_data33[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x041B, 0x043B, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data34[] = {
    0x041B, 0x043B, 0x041D, 0x043D, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data35[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0565, 0x0582, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data36[] = {
    0xFFFF, 0xFFFF, 0x0627, 0x0627, 0x0648, 0x0627, 0x064A, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x06A9, 0x06A9, 0x06CC, 0x06CC, 0x06CC};
static unsigned short unicode_unacc_data37[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0627, 0x0627, 0xFFFF, 0x0627, 0x0674, 0x0648,
    0x0674, 0x06C7, 0x0674, 0x064A, 0x0674, 0xFFFF, 0xFFFF, 0xFFFF, 0x062A, 0x062A, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data38[] = {
    0xFFFF, 0x062D, 0x062D, 0xFFFF, 0xFFFF, 0x062D, 0xFFFF, 0xFFFF, 0xFFFF, 0x062F, 0x062F,
    0x062F, 0xFFFF, 0xFFFF, 0xFFFF, 0x062F, 0x062F, 0xFFFF, 0x0631, 0x0631, 0x0631, 0x0631,
    0x0631, 0x0631, 0xFFFF, 0x0631, 0x0633, 0x0633, 0x0633, 0x0635, 0x0635, 0x0637};
static unsigned short unicode_unacc_data39[] = {
    0x0639, 0xFFFF, 0x0641, 0x0641, 0xFFFF, 0x0641, 0xFFFF, 0x0642, 0x0642, 0xFFFF, 0xFFFF,
    0x0643, 0x0643, 0xFFFF, 0x0643, 0xFFFF, 0x06AF, 0xFFFF, 0x06AF, 0xFFFF, 0x06AF, 0x0644,
    0x0644, 0x0644, 0x0644, 0x0646, 0xFFFF, 0xFFFF, 0x0646, 0x0646, 0xFFFF, 0x0686};
static unsigned short unicode_unacc_data40[] = {
    0x06D5, 0xFFFF, 0x06C1, 0xFFFF, 0x0648, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0648,
    0xFFFF, 0xFFFF, 0x064A, 0x064A, 0x0648, 0xFFFF, 0x064A, 0xFFFF, 0x06D2, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data41[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0x062F, 0x0631, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0634, 0x0636, 0x063A, 0xFFFF, 0xFFFF, 0x0647};
static unsigned short unicode_unacc_data42[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0628, 0x0628, 0x0628, 0x0628, 0x0628, 0x0628,
    0x0628, 0x062D, 0x062D, 0x062F, 0x062F, 0x0631, 0x0633, 0x0639, 0x0639, 0x0639};
static unsigned short unicode_unacc_data43[] = {
    0x0641, 0x0641, 0x06A9, 0x06A9, 0x06A9, 0x0645, 0x0645, 0x0646, 0x0646, 0x0646, 0x0644,
    0x0631, 0x0631, 0x0633, 0x062D, 0x062D, 0x0633, 0x0631, 0x062D, 0x0627, 0x0627, 0x06CC,
    0x06CC, 0x06CC, 0x0648, 0x0648, 0x06D2, 0x06D2, 0x062D, 0x0633, 0x0633, 0x0643};
static unsigned short unicode_unacc_data44[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0928, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0930, 0xFFFF, 0xFFFF, 0x0933, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data45[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0x0915, 0x0916, 0x0917, 0x091C, 0x0921, 0x0922, 0x092B, 0x092F};
static unsigned short unicode_unacc_data46[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x09A1, 0x09A2, 0xFFFF, 0x09AF};
static unsigned short unicode_unacc_data47[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x09B0, 0x09B0, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data48[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0A32, 0xFFFF, 0xFFFF,
    0x0A38, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data49[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0x0A16, 0x0A17, 0x0A1C, 0xFFFF, 0xFFFF, 0x0A2B, 0xFFFF};
static unsigned short unicode_unacc_data50[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0B21, 0x0B22, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data51[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0B92, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data52[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0E32, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data53[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0EB2, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data54[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0EAB, 0x0E99, 0x0EAB, 0x0EA1, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data55[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0x0F0B, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data56[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0x0F42, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0x0F4C, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0F51, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0x0F56, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0F5B, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data57[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0F40, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data58[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1025, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data59[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x10DC, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data60[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1B05, 0xFFFF, 0x1B07, 0xFFFF, 0x1B09,
    0xFFFF, 0x1B0B, 0xFFFF, 0x1B0D, 0xFFFF, 0xFFFF, 0xFFFF, 0x1B11, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data61[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0x029F, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1D11, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data62[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0x0041, 0x0041, 0x0045, 0x0042, 0xFFFF, 0x0044, 0x0045, 0x018E, 0x0047, 0x0048,
    0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0xFFFF, 0x004F, 0x0222, 0x0050, 0x0052};
static unsigned short unicode_unacc_data63[] = {
    0x0054, 0x0055, 0x0057, 0x0061, 0x0250, 0x0251, 0x1D02, 0x0062, 0x0064, 0x0065, 0x0259,
    0x025B, 0x025C, 0x0067, 0xFFFF, 0x006B, 0x006D, 0x014B, 0x006F, 0x0254, 0x1D16, 0x1D17,
    0x0070, 0x0074, 0x0075, 0x1D1D, 0x026F, 0x0076, 0x1D25, 0x03B2, 0x03B3, 0x03B4};
static unsigned short unicode_unacc_data64[] = {
    0x03C6, 0x03C7, 0x0069, 0x0072, 0x0075, 0x0076, 0x03B2, 0x03B3, 0x03C1, 0x03C6, 0x03C7,
    0xFFFF, 0x0062, 0x0064, 0x0066, 0x006D, 0x006E, 0x0070, 0x0072, 0x0072, 0x0073, 0x0074,
    0x007A, 0xFFFF, 0x043D, 0xFFFF, 0xFFFF, 0xFFFF, 0x0269, 0x0070, 0xFFFF, 0x028A};
static unsigned short unicode_unacc_data65[] = {
    0x0062, 0x0064, 0x0066, 0x0067, 0x006B, 0x006C, 0x006D, 0x006E, 0x0070, 0x0072, 0x0073,
    0x0283, 0x0076, 0x0078, 0x007A, 0x0061, 0x0251, 0x0064, 0x0065, 0x025B, 0x025C, 0x0259,
    0x0069, 0x0254, 0x0283, 0x0075, 0x0292, 0x0252, 0x0063, 0x0063, 0x00F0, 0x025C};
static unsigned short unicode_unacc_data66[] = {
    0x0066, 0x0237, 0x0261, 0x0265, 0x0069, 0x0269, 0x026A, 0x1D7B, 0x006A, 0x006C, 0x006C,
    0x029F, 0x006D, 0x026F, 0x006E, 0x006E, 0x0274, 0x0275, 0x0278, 0x0073, 0x0283, 0x0074,
    0x0289, 0x028A, 0x1D1C, 0x0076, 0x028C, 0x007A, 0x007A, 0x007A, 0x0292, 0x03B8};
static unsigned short unicode_unacc_data67[] = {
    0x0041, 0x0061, 0x0042, 0x0062, 0x0042, 0x0062, 0x0042, 0x0062, 0x0043, 0x0063, 0x0044,
    0x0064, 0x0044, 0x0064, 0x0044, 0x0064, 0x0044, 0x0064, 0x0044, 0x0064, 0x0045, 0x0065,
    0x0045, 0x0065, 0x0045, 0x0065, 0x0045, 0x0065, 0x0045, 0x0065, 0x0046, 0x0066};
static unsigned short unicode_unacc_data68[] = {
    0x0047, 0x0067, 0x0048, 0x0068, 0x0048, 0x0068, 0x0048, 0x0068, 0x0048, 0x0068, 0x0048,
    0x0068, 0x0049, 0x0069, 0x0049, 0x0069, 0x004B, 0x006B, 0x004B, 0x006B, 0x004B, 0x006B,
    0x004C, 0x006C, 0x004C, 0x006C, 0x004C, 0x006C, 0x004C, 0x006C, 0x004D, 0x006D};
static unsigned short unicode_unacc_data69[] = {
    0x004D, 0x006D, 0x004D, 0x006D, 0x004E, 0x006E, 0x004E, 0x006E, 0x004E, 0x006E, 0x004E,
    0x006E, 0x004F, 0x006F, 0x004F, 0x006F, 0x004F, 0x006F, 0x004F, 0x006F, 0x0050, 0x0070,
    0x0050, 0x0070, 0x0052, 0x0072, 0x0052, 0x0072, 0x0052, 0x0072, 0x0052, 0x0072};
static unsigned short unicode_unacc_data70[] = {
    0x0053, 0x0073, 0x0053, 0x0073, 0x0053, 0x0073, 0x0053, 0x0073, 0x0053, 0x0073, 0x0054,
    0x0074, 0x0054, 0x0074, 0x0054, 0x0074, 0x0054, 0x0074, 0x0055, 0x0075, 0x0055, 0x0075,
    0x0055, 0x0075, 0x0055, 0x0075, 0x0055, 0x0075, 0x0056, 0x0076, 0x0056, 0x0076};
static unsigned short unicode_unacc_data71[] = {
    0x0057, 0x0077, 0x0057, 0x0077, 0x0057, 0x0077, 0x0057, 0x0077, 0x0057, 0x0077, 0x0058,
    0x0078, 0x0058, 0x0078, 0x0059, 0x0079, 0x005A, 0x007A, 0x005A, 0x007A, 0x005A, 0x007A,
    0x0068, 0x0074, 0x0077, 0x0079, 0x0061, 0x02BE, 0x0073, 0x0073, 0x0073, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data72[] = {
    0x0041, 0x0061, 0x0041, 0x0061, 0x0041, 0x0061, 0x0041, 0x0061, 0x0041, 0x0061, 0x0041,
    0x0061, 0x0041, 0x0061, 0x0041, 0x0061, 0x0041, 0x0061, 0x0041, 0x0061, 0x0041, 0x0061,
    0x0041, 0x0061, 0x0045, 0x0065, 0x0045, 0x0065, 0x0045, 0x0065, 0x0045, 0x0065};
static unsigned short unicode_unacc_data73[] = {
    0x0045, 0x0065, 0x0045, 0x0065, 0x0045, 0x0065, 0x0045, 0x0065, 0x0049, 0x0069, 0x0049,
    0x0069, 0x004F, 0x006F, 0x004F, 0x006F, 0x004F, 0x006F, 0x004F, 0x006F, 0x004F, 0x006F,
    0x004F, 0x006F, 0x004F, 0x006F, 0x004F, 0x006F, 0x004F, 0x006F, 0x004F, 0x006F};
static unsigned short unicode_unacc_data74[] = {
    0x004F, 0x006F, 0x004F, 0x006F, 0x0055, 0x0075, 0x0055, 0x0075, 0x0055, 0x0075, 0x0055,
    0x0075, 0x0055, 0x0075, 0x0055, 0x0075, 0x0055, 0x0075, 0x0059, 0x0079, 0x0059, 0x0079,
    0x0059, 0x0079, 0x0059, 0x0079, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0059, 0x0079};
static unsigned short unicode_unacc_data75[] = {
    0x03B1, 0x03B1, 0x03B1, 0x03B1, 0x03B1, 0x03B1, 0x03B1, 0x03B1, 0x0391, 0x0391, 0x0391,
    0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x03B5, 0x03B5, 0x03B5, 0x03B5, 0x03B5, 0x03B5,
    0xFFFF, 0xFFFF, 0x0395, 0x0395, 0x0395, 0x0395, 0x0395, 0x0395, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data76[] = {
    0x03B7, 0x03B7, 0x03B7, 0x03B7, 0x03B7, 0x03B7, 0x03B7, 0x03B7, 0x0397, 0x0397, 0x0397,
    0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x03B9, 0x03B9, 0x03B9, 0x03B9, 0x03B9, 0x03B9,
    0x03B9, 0x03B9, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399};
static unsigned short unicode_unacc_data77[] = {
    0x03BF, 0x03BF, 0x03BF, 0x03BF, 0x03BF, 0x03BF, 0xFFFF, 0xFFFF, 0x039F, 0x039F, 0x039F,
    0x039F, 0x039F, 0x039F, 0xFFFF, 0xFFFF, 0x03C5, 0x03C5, 0x03C5, 0x03C5, 0x03C5, 0x03C5,
    0x03C5, 0x03C5, 0xFFFF, 0x03A5, 0xFFFF, 0x03A5, 0xFFFF, 0x03A5, 0xFFFF, 0x03A5};
static unsigned short unicode_unacc_data78[] = {
    0x03C9, 0x03C9, 0x03C9, 0x03C9, 0x03C9, 0x03C9, 0x03C9, 0x03C9, 0x03A9, 0x03A9, 0x03A9,
    0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03B1, 0x03B1, 0x03B5, 0x03B5, 0x03B7, 0x03B7,
    0x03B9, 0x03B9, 0x03BF, 0x03BF, 0x03C5, 0x03C5, 0x03C9, 0x03C9, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data79[] = {
    0x03B1, 0x03B1, 0x03B1, 0x03B1, 0x03B1, 0x03B1, 0x03B1, 0x03B1, 0x0391, 0x0391, 0x0391,
    0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x03B7, 0x03B7, 0x03B7, 0x03B7, 0x03B7, 0x03B7,
    0x03B7, 0x03B7, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397};
static unsigned short unicode_unacc_data80[] = {
    0x03C9, 0x03C9, 0x03C9, 0x03C9, 0x03C9, 0x03C9, 0x03C9, 0x03C9, 0x03A9, 0x03A9, 0x03A9,
    0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03B1, 0x03B1, 0x03B1, 0x03B1, 0x03B1, 0xFFFF,
    0x03B1, 0x03B1, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0020, 0x03B9, 0x0020};
static unsigned short unicode_unacc_data81[] = {
    0x0020, 0x0020, 0x03B7, 0x03B7, 0x03B7, 0xFFFF, 0x03B7, 0x03B7, 0x0395, 0x0395, 0x0397,
    0x0397, 0x0397, 0x0020, 0x0020, 0x0020, 0x03B9, 0x03B9, 0x03B9, 0x03B9, 0xFFFF, 0xFFFF,
    0x03B9, 0x03B9, 0x0399, 0x0399, 0x0399, 0x0399, 0xFFFF, 0x0020, 0x0020, 0x0020};
static unsigned short unicode_unacc_data82[] = {
    0x03C5, 0x03C5, 0x03C5, 0x03C5, 0x03C1, 0x03C1, 0x03C5, 0x03C5, 0x03A5, 0x03A5, 0x03A5,
    0x03A5, 0x03A1, 0x0020, 0x0020, 0x0060, 0xFFFF, 0xFFFF, 0x03C9, 0x03C9, 0x03C9, 0xFFFF,
    0x03C9, 0x03C9, 0x039F, 0x039F, 0x03A9, 0x03A9, 0x03A9, 0x0020, 0x0020, 0xFFFF};
static unsigned short unicode_unacc_data83[] = {
    0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x2010, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0x0020, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data84[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0020, 0xFFFF, 0xFFFF, 0xFFFF,
    0x2032, 0x2032, 0x2032, 0x2032, 0x2032, 0xFFFF, 0x2035, 0x2035, 0x2035, 0x2035, 0x2035,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0021, 0x0021, 0xFFFF, 0x0020, 0xFFFF};
static unsigned short unicode_unacc_data85[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x005B, 0x005D, 0x003F, 0x003F, 0x003F,
    0x0021, 0x0021, 0x003F, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x2032, 0x2032, 0x2032, 0x2032,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0020};
static unsigned short unicode_unacc_data86[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0030, 0x0069, 0xFFFF, 0xFFFF, 0x0034, 0x0035,
    0x0036, 0x0037, 0x0038, 0x0039, 0x002B, 0x2212, 0x003D, 0x0028, 0x0029, 0x006E};
static unsigned short unicode_unacc_data87[] = {
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x002B,
    0x2212, 0x003D, 0x0028, 0x0029, 0xFFFF, 0x0061, 0x0065, 0x006F, 0x0078, 0x0259, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data88[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0052, 0x0073, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data89[] = {
    0x0061, 0x002F, 0x0063, 0x0061, 0x002F, 0x0073, 0x0043, 0x00B0, 0x0043, 0xFFFF, 0x0063,
    0x002F, 0x006F, 0x0063, 0x002F, 0x0075, 0x0190, 0xFFFF, 0x00B0, 0x0046, 0x0067, 0x0048,
    0x0048, 0x0048, 0x0068, 0x0068, 0x0049, 0x0049, 0x004C, 0x006C, 0xFFFF, 0x004E, 0x004E,
    0x006F, 0xFFFF, 0xFFFF, 0x0050, 0x0051, 0x0052, 0x0052, 0x0052, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data90[] = {
    0x0053, 0x004D, 0x0054, 0x0045, 0x004C, 0x0054, 0x004D, 0xFFFF, 0x005A, 0xFFFF,
    0x03A9, 0xFFFF, 0x005A, 0xFFFF, 0x004B, 0x0041, 0x0042, 0x0043, 0xFFFF, 0x0065,
    0x0045, 0x0046, 0xFFFF, 0x004D, 0x006F, 0x05D0, 0x05D1, 0x05D2, 0x05D3, 0x0069,
    0xFFFF, 0x0046, 0x0041, 0x0058, 0x03C0, 0x03B3, 0x0393, 0x03A0};
static unsigned short unicode_unacc_data91[] = {
    0x2211, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0044, 0x0064, 0x0065, 0x0069, 0x006A, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0031, 0x2044, 0x0033, 0x0032, 0x2044,
    0x0033, 0x0031, 0x2044, 0x0035, 0x0032, 0x2044, 0x0035, 0x0033, 0x2044, 0x0035, 0x0034, 0x2044,
    0x0035, 0x0031, 0x2044, 0x0036, 0x0035, 0x2044, 0x0036, 0x0031, 0x2044, 0x0038, 0x0033, 0x2044,
    0x0038, 0x0035, 0x2044, 0x0038, 0x0037, 0x2044, 0x0038, 0x0031, 0x2044};
static unsigned short unicode_unacc_data92[] = {
    0x0049, 0x0049, 0x0049, 0x0049, 0x0049, 0x0049, 0x0049, 0x0056, 0x0056, 0x0056, 0x0049, 0x0056,
    0x0049, 0x0049, 0x0056, 0x0049, 0x0049, 0x0049, 0x0049, 0x0058, 0x0058, 0x0058, 0x0049, 0x0058,
    0x0049, 0x0049, 0x004C, 0x0043, 0x0044, 0x004D, 0x0069, 0x0069, 0x0069, 0x0069, 0x0069, 0x0069,
    0x0069, 0x0076, 0x0076, 0x0076, 0x0069, 0x0076, 0x0069, 0x0069, 0x0076, 0x0069, 0x0069, 0x0069,
    0x0069, 0x0078, 0x0078, 0x0078, 0x0069, 0x0078, 0x0069, 0x0069, 0x006C, 0x0063, 0x0064, 0x006D};
static unsigned short unicode_unacc_data93[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x2190, 0x2192, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data94[] = {
    0xFFFF, 0xFFFF, 0x2190, 0x2192, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x2195, 0x2190, 0x2192,
    0x2190, 0x2192, 0xFFFF, 0x2194, 0xFFFF, 0x2191, 0x2191, 0x2193, 0x2193, 0x2192, 0x2193,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data95[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0x21D0, 0x21D4, 0x21D2, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x2191, 0x2193};
static unsigned short unicode_unacc_data96[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0x21EB, 0x21EB, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0x2190, 0x2192, 0x2194, 0x2190, 0x2192, 0x2194, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data97[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x2203, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x2208, 0xFFFF,
    0xFFFF, 0x220B, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data98[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x2223, 0xFFFF, 0x2225, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0x222B, 0x222B, 0x222B, 0x222B, 0x222B, 0xFFFF, 0x222E, 0x222E,
    0x222E, 0x222E, 0x222E, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data99[] = {
    0xFFFF, 0x223C, 0xFFFF, 0xFFFF, 0x2243, 0xFFFF, 0xFFFF, 0x2245, 0xFFFF, 0x2248, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data100[] = {
    0x003D, 0xFFFF, 0x2261, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0x224D, 0x003C, 0x003E, 0x2264, 0x2265, 0xFFFF, 0xFFFF, 0x2272, 0x2273,
    0xFFFF, 0xFFFF, 0x2276, 0x2277, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data101[] = {
    0x227A, 0x227B, 0xFFFF, 0xFFFF, 0x2282, 0x2283, 0xFFFF, 0xFFFF, 0x2286, 0x2287, 0x2282,
    0x2283, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data102[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0x22A2, 0x22A8, 0x22A9, 0x22AB, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x221F, 0xFFFF};
static unsigned short unicode_unacc_data103[] = {
    0x227C, 0x227D, 0x2291, 0x2292, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x22B2,
    0x22B3, 0x22B4, 0x22B5, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x2208, 0x2208, 0x220A, 0x2208,
    0x2208, 0x220A, 0x2208, 0x2208, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data104[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x3008, 0x3009,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data105[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x221F, 0xFFFF, 0xFFFF, 0x007C};
static unsigned short unicode_unacc_data106[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x25A1, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data107[] = {
    0xFFFF, 0x23C9, 0x23CA, 0xFFFF, 0x23C9, 0x23CA, 0xFFFF, 0x23C9, 0x23CA, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data108[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0x232C, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data109[] = {
    0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x0031, 0x0030, 0x0031,
    0x0031, 0x0031, 0x0032, 0x0031, 0x0033, 0x0031, 0x0034, 0x0031, 0x0035, 0x0031, 0x0036, 0x0031,
    0x0037, 0x0031, 0x0038, 0x0031, 0x0039, 0x0032, 0x0030, 0x0028, 0x0031, 0x0029, 0x0028, 0x0032,
    0x0029, 0x0028, 0x0033, 0x0029, 0x0028, 0x0034, 0x0029, 0x0028, 0x0035, 0x0029, 0x0028, 0x0036,
    0x0029, 0x0028, 0x0037, 0x0029, 0x0028, 0x0038, 0x0029, 0x0028, 0x0039, 0x0029, 0x0028, 0x0031,
    0x0030, 0x0029, 0x0028, 0x0031, 0x0031, 0x0029, 0x0028, 0x0031, 0x0032, 0x0029};
static unsigned short unicode_unacc_data110[] = {
    0x0028, 0x0031, 0x0033, 0x0029, 0x0028, 0x0031, 0x0034, 0x0029, 0x0028, 0x0031, 0x0035, 0x0029,
    0x0028, 0x0031, 0x0036, 0x0029, 0x0028, 0x0031, 0x0037, 0x0029, 0x0028, 0x0031, 0x0038, 0x0029,
    0x0028, 0x0031, 0x0039, 0x0029, 0x0028, 0x0032, 0x0030, 0x0029, 0x0031, 0x002E, 0x0032, 0x002E,
    0x0033, 0x002E, 0x0034, 0x002E, 0x0035, 0x002E, 0x0036, 0x002E, 0x0037, 0x002E, 0x0038, 0x002E,
    0x0039, 0x002E, 0x0031, 0x0030, 0x002E, 0x0031, 0x0031, 0x002E, 0x0031, 0x0032, 0x002E, 0x0031,
    0x0033, 0x002E, 0x0031, 0x0034, 0x002E, 0x0031, 0x0035, 0x002E, 0x0031, 0x0036, 0x002E, 0x0031,
    0x0037, 0x002E, 0x0031, 0x0038, 0x002E, 0x0031, 0x0039, 0x002E, 0x0032, 0x0030, 0x002E, 0x0028,
    0x0061, 0x0029, 0x0028, 0x0062, 0x0029, 0x0028, 0x0063, 0x0029, 0x0028, 0x0064, 0x0029};
static unsigned short unicode_unacc_data111[] = {
    0x0028, 0x0065, 0x0029, 0x0028, 0x0066, 0x0029, 0x0028, 0x0067, 0x0029, 0x0028, 0x0068,
    0x0029, 0x0028, 0x0069, 0x0029, 0x0028, 0x006A, 0x0029, 0x0028, 0x006B, 0x0029, 0x0028,
    0x006C, 0x0029, 0x0028, 0x006D, 0x0029, 0x0028, 0x006E, 0x0029, 0x0028, 0x006F, 0x0029,
    0x0028, 0x0070, 0x0029, 0x0028, 0x0071, 0x0029, 0x0028, 0x0072, 0x0029, 0x0028, 0x0073,
    0x0029, 0x0028, 0x0074, 0x0029, 0x0028, 0x0075, 0x0029, 0x0028, 0x0076, 0x0029, 0x0028,
    0x0077, 0x0029, 0x0028, 0x0078, 0x0029, 0x0028, 0x0079, 0x0029, 0x0028, 0x007A, 0x0029,
    0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004A};
static unsigned short unicode_unacc_data112[] = {
    0x004B, 0x004C, 0x004D, 0x004E, 0x004F, 0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055,
    0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066,
    0x0067, 0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F, 0x0070};
static unsigned short unicode_unacc_data113[] = {
    0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007A, 0x0030,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data114[] = {
    0xFFFF, 0xFFFF, 0x25A1, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data115[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0x25A1, 0x25B3, 0xFFFF, 0xFFFF, 0xFFFF, 0x25A1, 0x25A1, 0x25A1, 0x25A1, 0x25CB, 0x25CB,
    0x25CB, 0x25CB, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data116[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x2610, 0x2610, 0xFFFF, 0x2602, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data117[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x25CB, 0x25CB, 0x25CF, 0x25CF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data118[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x25C7, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x22A5};
static unsigned short unicode_unacc_data119[] = {
    0xFFFF, 0xFFFF, 0x27E1, 0x27E1, 0x25A1, 0x25A1, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data120[] = {
    0xFFFF, 0xFFFF, 0x21D0, 0x21D2, 0x21D4, 0xFFFF, 0xFFFF, 0xFFFF, 0x2193, 0x2191, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x2192, 0xFFFF, 0xFFFF, 0x2192, 0x2192,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data121[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0x2196, 0x2197, 0x2198, 0x2199, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x293A, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data122[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x2192, 0x2190, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data123[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0x005B, 0x005D, 0x005B, 0x005D, 0x005B, 0x005D, 0x3008, 0x3009, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x2220, 0xFFFF};
static unsigned short unicode_unacc_data124[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x2220, 0x29A3, 0xFFFF, 0xFFFF, 0x2221, 0x2221, 0x2221,
    0x2221, 0x2221, 0x2221, 0x2221, 0x2221, 0xFFFF, 0x2205, 0x2205, 0x2205, 0x2205, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data125[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x22C8, 0x22C8, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data126[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x29E3, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x25C6,
    0xFFFF, 0x25CB, 0x25CF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0x002F, 0x005C, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data127[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0x222B, 0x222B, 0x222B, 0x222B, 0xFFFF, 0x222B, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0x222B, 0x222B, 0x222B, 0x222B, 0x222B, 0x222B, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data128[] = {
    0xFFFF, 0xFFFF, 0x002B, 0x002B, 0x002B, 0x002B, 0x002B, 0x002B, 0x002B, 0x2212, 0x2212,
    0x2212, 0x2212, 0xFFFF, 0xFFFF, 0xFFFF, 0x00D7, 0x00D7, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data129[] = {
    0x2229, 0x222A, 0x222A, 0x2229, 0x2229, 0x222A, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x2227, 0x2228, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x2227, 0x2228, 0x2227, 0x2228, 0x2227, 0x2227};
static unsigned short unicode_unacc_data130[] = {
    0x2227, 0xFFFF, 0x2228, 0x2228, 0xFFFF, 0xFFFF, 0x003D, 0xFFFF, 0xFFFF, 0xFFFF,
    0x223C, 0x223C, 0xFFFF, 0xFFFF, 0xFFFF, 0x2248, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0x003A, 0x003A, 0x003D, 0x003D, 0x003D, 0x003D, 0x003D, 0x003D, 0x003D, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x2A7D};
static unsigned short unicode_unacc_data131[] = {
    0x2A7E, 0x2A7D, 0x2A7E, 0x2A7D, 0x2A7E, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0x2A95, 0x2A96, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data132[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0x2AA1, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0x003D, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data133[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0x2286, 0x2287, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x22D4, 0xFFFF, 0x2ADD, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data134[] = {
    0xFFFF, 0x27C2, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x2ADF, 0x2AE0, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0x2223, 0x007C, 0x007C, 0x22A4, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data135[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0x2192, 0x2192, 0x2190, 0x2190, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data136[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0x2190, 0x2190, 0x2190, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data137[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x2C24, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data138[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x2C54,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data139[] = {
    0x004C, 0x006C, 0x004C, 0x0050, 0x0052, 0x0061, 0x0074, 0x0048, 0x0068, 0x004B, 0x006B,
    0x005A, 0x007A, 0xFFFF, 0x004D, 0xFFFF, 0xFFFF, 0x0076, 0x0057, 0x0077, 0x0076, 0xFFFF,
    0xFFFF, 0xFFFF, 0x0065, 0x0279, 0x006F, 0xFFFF, 0x006A, 0x0056, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data140[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x2D61, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data141[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x2010, 0x007E, 0xFFFF, 0xFFFF, 0x007E, 0x007E};
static unsigned short unicode_unacc_data142[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x6BCD};
static unsigned short unicode_unacc_data143[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x9F9F, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data144[] = {
    0x4E00, 0x4E28, 0x4E36, 0x4E3F, 0x4E59, 0x4E85, 0x4E8C, 0x4EA0, 0x4EBA, 0x513F, 0x5165,
    0x516B, 0x5182, 0x5196, 0x51AB, 0x51E0, 0x51F5, 0x5200, 0x529B, 0x52F9, 0x5315, 0x531A,
    0x5338, 0x5341, 0x535C, 0x5369, 0x5382, 0x53B6, 0x53C8, 0x53E3, 0x56D7, 0x571F};
static unsigned short unicode_unacc_data145[] = {
    0x58EB, 0x5902, 0x590A, 0x5915, 0x5927, 0x5973, 0x5B50, 0x5B80, 0x5BF8, 0x5C0F, 0x5C22,
    0x5C38, 0x5C6E, 0x5C71, 0x5DDB, 0x5DE5, 0x5DF1, 0x5DFE, 0x5E72, 0x5E7A, 0x5E7F, 0x5EF4,
    0x5EFE, 0x5F0B, 0x5F13, 0x5F50, 0x5F61, 0x5F73, 0x5FC3, 0x6208, 0x6236, 0x624B};
static unsigned short unicode_unacc_data146[] = {
    0x652F, 0x6534, 0x6587, 0x6597, 0x65A4, 0x65B9, 0x65E0, 0x65E5, 0x66F0, 0x6708, 0x6728,
    0x6B20, 0x6B62, 0x6B79, 0x6BB3, 0x6BCB, 0x6BD4, 0x6BDB, 0x6C0F, 0x6C14, 0x6C34, 0x706B,
    0x722A, 0x7236, 0x723B, 0x723F, 0x7247, 0x7259, 0x725B, 0x72AC, 0x7384, 0x7389};
static unsigned short unicode_unacc_data147[] = {
    0x74DC, 0x74E6, 0x7518, 0x751F, 0x7528, 0x7530, 0x758B, 0x7592, 0x7676, 0x767D, 0x76AE,
    0x76BF, 0x76EE, 0x77DB, 0x77E2, 0x77F3, 0x793A, 0x79B8, 0x79BE, 0x7A74, 0x7ACB, 0x7AF9,
    0x7C73, 0x7CF8, 0x7F36, 0x7F51, 0x7F8A, 0x7FBD, 0x8001, 0x800C, 0x8012, 0x8033};
static unsigned short unicode_unacc_data148[] = {
    0x807F, 0x8089, 0x81E3, 0x81EA, 0x81F3, 0x81FC, 0x820C, 0x821B, 0x821F, 0x826E, 0x8272,
    0x8278, 0x864D, 0x866B, 0x8840, 0x884C, 0x8863, 0x897E, 0x898B, 0x89D2, 0x8A00, 0x8C37,
    0x8C46, 0x8C55, 0x8C78, 0x8C9D, 0x8D64, 0x8D70, 0x8DB3, 0x8EAB, 0x8ECA, 0x8F9B};
static unsigned short unicode_unacc_data149[] = {
    0x8FB0, 0x8FB5, 0x9091, 0x9149, 0x91C6, 0x91CC, 0x91D1, 0x9577, 0x9580, 0x961C, 0x96B6,
    0x96B9, 0x96E8, 0x9751, 0x975E, 0x9762, 0x9769, 0x97CB, 0x97ED, 0x97F3, 0x9801, 0x98A8,
    0x98DB, 0x98DF, 0x9996, 0x9999, 0x99AC, 0x9AA8, 0x9AD8, 0x9ADF, 0x9B25, 0x9B2F};
static unsigned short unicode_unacc_data150[] = {
    0x9B32, 0x9B3C, 0x9B5A, 0x9CE5, 0x9E75, 0x9E7F, 0x9EA5, 0x9EBB, 0x9EC3, 0x9ECD, 0x9ED1,
    0x9EF9, 0x9EFD, 0x9F0E, 0x9F13, 0x9F20, 0x9F3B, 0x9F4A, 0x9F52, 0x9F8D, 0x9F9C, 0x9FA0,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data151[] = {
    0x0020, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data152[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0x3012, 0xFFFF, 0x5341, 0x5344, 0x5345, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data153[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0x304B, 0xFFFF, 0x304D, 0xFFFF, 0x304F, 0xFFFF, 0x3051, 0xFFFF, 0x3053, 0xFFFF,
    0x3055, 0xFFFF, 0x3057, 0xFFFF, 0x3059, 0xFFFF, 0x305B, 0xFFFF, 0x305D, 0xFFFF};
static unsigned short unicode_unacc_data154[] = {
    0x305F, 0xFFFF, 0x3061, 0xFFFF, 0xFFFF, 0x3064, 0xFFFF, 0x3066, 0xFFFF, 0x3068, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x306F, 0x306F, 0xFFFF, 0x3072, 0x3072, 0xFFFF,
    0x3075, 0x3075, 0xFFFF, 0x3078, 0x3078, 0xFFFF, 0x307B, 0x307B, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data155[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x3046, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0020, 0x0020, 0xFFFF, 0x309D, 0x3088, 0x308A};
static unsigned short unicode_unacc_data156[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0x30AB, 0xFFFF, 0x30AD, 0xFFFF, 0x30AF, 0xFFFF, 0x30B1, 0xFFFF, 0x30B3, 0xFFFF,
    0x30B5, 0xFFFF, 0x30B7, 0xFFFF, 0x30B9, 0xFFFF, 0x30BB, 0xFFFF, 0x30BD, 0xFFFF};
static unsigned short unicode_unacc_data157[] = {
    0x30BF, 0xFFFF, 0x30C1, 0xFFFF, 0xFFFF, 0x30C4, 0xFFFF, 0x30C6, 0xFFFF, 0x30C8, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x30CF, 0x30CF, 0xFFFF, 0x30D2, 0x30D2, 0xFFFF,
    0x30D5, 0x30D5, 0xFFFF, 0x30D8, 0x30D8, 0xFFFF, 0x30DB, 0x30DB, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data158[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x30A6, 0xFFFF,
    0xFFFF, 0x30EF, 0x30F0, 0x30F1, 0x30F2, 0xFFFF, 0xFFFF, 0xFFFF, 0x30FD, 0x30B3, 0x30C8};
static unsigned short unicode_unacc_data159[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x1100, 0x1101, 0x11AA, 0x1102, 0x11AC,
    0x11AD, 0x1103, 0x1104, 0x1105, 0x11B0, 0x11B1, 0x11B2, 0x11B3, 0x11B4, 0x11B5};
static unsigned short unicode_unacc_data160[] = {
    0x111A, 0x1106, 0x1107, 0x1108, 0x1121, 0x1109, 0x110A, 0x110B, 0x110C, 0x110D, 0x110E,
    0x110F, 0x1110, 0x1111, 0x1112, 0x1161, 0x1162, 0x1163, 0x1164, 0x1165, 0x1166, 0x1167,
    0x1168, 0x1169, 0x116A, 0x116B, 0x116C, 0x116D, 0x116E, 0x116F, 0x1170, 0x1171};
static unsigned short unicode_unacc_data161[] = {
    0x1172, 0x1173, 0x1174, 0x1175, 0x1160, 0x1114, 0x1115, 0x11C7, 0x11C8, 0x11CC, 0x11CE,
    0x11D3, 0x11D7, 0x11D9, 0x111C, 0x11DD, 0x11DF, 0x111D, 0x111E, 0x1120, 0x1122, 0x1123,
    0x1127, 0x1129, 0x112B, 0x112C, 0x112D, 0x112E, 0x112F, 0x1132, 0x1136, 0x1140};
static unsigned short unicode_unacc_data162[] = {
    0x1147, 0x114C, 0x11F1, 0x11F2, 0x1157, 0x1158, 0x1159, 0x1184, 0x1185, 0x1188, 0x1191,
    0x1192, 0x1194, 0x119E, 0x11A1, 0xFFFF, 0xFFFF, 0xFFFF, 0x4E00, 0x4E8C, 0x4E09, 0x56DB,
    0x4E0A, 0x4E2D, 0x4E0B, 0x7532, 0x4E59, 0x4E19, 0x4E01, 0x5929, 0x5730, 0x4EBA};
static unsigned short unicode_unacc_data163[] = {
    0x0028, 0x1100, 0x0029, 0x0028, 0x1102, 0x0029, 0x0028, 0x1103, 0x0029, 0x0028, 0x1105, 0x0029,
    0x0028, 0x1106, 0x0029, 0x0028, 0x1107, 0x0029, 0x0028, 0x1109, 0x0029, 0x0028, 0x110B, 0x0029,
    0x0028, 0x110C, 0x0029, 0x0028, 0x110E, 0x0029, 0x0028, 0x110F, 0x0029, 0x0028, 0x1110, 0x0029,
    0x0028, 0x1111, 0x0029, 0x0028, 0x1112, 0x0029, 0x0028, 0x1100, 0x1161, 0x0029, 0x0028, 0x1102,
    0x1161, 0x0029, 0x0028, 0x1103, 0x1161, 0x0029, 0x0028, 0x1105, 0x1161, 0x0029, 0x0028, 0x1106,
    0x1161, 0x0029, 0x0028, 0x1107, 0x1161, 0x0029, 0x0028, 0x1109, 0x1161, 0x0029, 0x0028, 0x110B,
    0x1161, 0x0029, 0x0028, 0x110C, 0x1161, 0x0029, 0x0028, 0x110E, 0x1161, 0x0029, 0x0028, 0x110F,
    0x1161, 0x0029, 0x0028, 0x1110, 0x1161, 0x0029, 0x0028, 0x1111, 0x1161, 0x0029, 0x0028, 0x1112,
    0x1161, 0x0029, 0x0028, 0x110C, 0x116E, 0x0029, 0x0028, 0x110B, 0x1169, 0x110C, 0x1165, 0x11AB,
    0x0029, 0x0028, 0x110B, 0x1169, 0x1112, 0x116E, 0x0029, 0xFFFF};
static unsigned short unicode_unacc_data164[] = {
    0x0028, 0x4E00, 0x0029, 0x0028, 0x4E8C, 0x0029, 0x0028, 0x4E09, 0x0029, 0x0028, 0x56DB, 0x0029,
    0x0028, 0x4E94, 0x0029, 0x0028, 0x516D, 0x0029, 0x0028, 0x4E03, 0x0029, 0x0028, 0x516B, 0x0029,
    0x0028, 0x4E5D, 0x0029, 0x0028, 0x5341, 0x0029, 0x0028, 0x6708, 0x0029, 0x0028, 0x706B, 0x0029,
    0x0028, 0x6C34, 0x0029, 0x0028, 0x6728, 0x0029, 0x0028, 0x91D1, 0x0029, 0x0028, 0x571F, 0x0029,
    0x0028, 0x65E5, 0x0029, 0x0028, 0x682A, 0x0029, 0x0028, 0x6709, 0x0029, 0x0028, 0x793E, 0x0029,
    0x0028, 0x540D, 0x0029, 0x0028, 0x7279, 0x0029, 0x0028, 0x8CA1, 0x0029, 0x0028, 0x795D, 0x0029,
    0x0028, 0x52B4, 0x0029, 0x0028, 0x4EE3, 0x0029, 0x0028, 0x547C, 0x0029, 0x0028, 0x5B66, 0x0029,
    0x0028, 0x76E3, 0x0029, 0x0028, 0x4F01, 0x0029, 0x0028, 0x8CC7, 0x0029, 0x0028, 0x5354, 0x0029};
static unsigned short unicode_unacc_data165[] = {
    0x0028, 0x796D, 0x0029, 0x0028, 0x4F11, 0x0029, 0x0028, 0x81EA, 0x0029, 0x0028, 0x81F3, 0x0029,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0x0050, 0x0054, 0x0045, 0x0032, 0x0031, 0x0032, 0x0032, 0x0032, 0x0033, 0x0032, 0x0034, 0x0032,
    0x0035, 0x0032, 0x0036, 0x0032, 0x0037, 0x0032, 0x0038, 0x0032, 0x0039, 0x0033, 0x0030, 0x0033,
    0x0031, 0x0033, 0x0032, 0x0033, 0x0033, 0x0033, 0x0034, 0x0033, 0x0035};
static unsigned short unicode_unacc_data166[] = {
    0x1100, 0x1102, 0x1103, 0x1105, 0x1106, 0x1107, 0x1109, 0x110B, 0x110C, 0x110E, 0x110F,
    0x1110, 0x1111, 0x1112, 0x1100, 0x1161, 0x1102, 0x1161, 0x1103, 0x1161, 0x1105, 0x1161,
    0x1106, 0x1161, 0x1107, 0x1161, 0x1109, 0x1161, 0x110B, 0x1161, 0x110C, 0x1161, 0x110E,
    0x1161, 0x110F, 0x1161, 0x1110, 0x1161, 0x1111, 0x1161, 0x1112, 0x1161, 0x110E, 0x1161,
    0x11B7, 0x1100, 0x1169, 0x110C, 0x116E, 0x110B, 0x1174, 0x110B, 0x116E, 0xFFFF};
static unsigned short unicode_unacc_data167[] = {
    0x4E00, 0x4E8C, 0x4E09, 0x56DB, 0x4E94, 0x516D, 0x4E03, 0x516B, 0x4E5D, 0x5341, 0x6708,
    0x706B, 0x6C34, 0x6728, 0x91D1, 0x571F, 0x65E5, 0x682A, 0x6709, 0x793E, 0x540D, 0x7279,
    0x8CA1, 0x795D, 0x52B4, 0x79D8, 0x7537, 0x5973, 0x9069, 0x512A, 0x5370, 0x6CE8};
static unsigned short unicode_unacc_data168[] = {
    0x9805, 0x4F11, 0x5199, 0x6B63, 0x4E0A, 0x4E2D, 0x4E0B, 0x5DE6, 0x53F3, 0x533B, 0x5B97, 0x5B66,
    0x76E3, 0x4F01, 0x8CC7, 0x5354, 0x591C, 0x0033, 0x0036, 0x0033, 0x0037, 0x0033, 0x0038, 0x0033,
    0x0039, 0x0034, 0x0030, 0x0034, 0x0031, 0x0034, 0x0032, 0x0034, 0x0033, 0x0034, 0x0034, 0x0034,
    0x0035, 0x0034, 0x0036, 0x0034, 0x0037, 0x0034, 0x0038, 0x0034, 0x0039, 0x0035, 0x0030};
static unsigned short unicode_unacc_data169[] = {
    0x0031, 0x6708, 0x0032, 0x6708, 0x0033, 0x6708, 0x0034, 0x6708, 0x0035, 0x6708, 0x0036,
    0x6708, 0x0037, 0x6708, 0x0038, 0x6708, 0x0039, 0x6708, 0x0031, 0x0030, 0x6708, 0x0031,
    0x0031, 0x6708, 0x0031, 0x0032, 0x6708, 0x0048, 0x0067, 0x0065, 0x0072, 0x0067, 0x0065,
    0x0056, 0x004C, 0x0054, 0x0044, 0x30A2, 0x30A4, 0x30A6, 0x30A8, 0x30AA, 0x30AB, 0x30AD,
    0x30AF, 0x30B1, 0x30B3, 0x30B5, 0x30B7, 0x30B9, 0x30BB, 0x30BD, 0x30BF};
static unsigned short unicode_unacc_data170[] = {
    0x30C1, 0x30C4, 0x30C6, 0x30C8, 0x30CA, 0x30CB, 0x30CC, 0x30CD, 0x30CE, 0x30CF, 0x30D2,
    0x30D5, 0x30D8, 0x30DB, 0x30DE, 0x30DF, 0x30E0, 0x30E1, 0x30E2, 0x30E4, 0x30E6, 0x30E8,
    0x30E9, 0x30EA, 0x30EB, 0x30EC, 0x30ED, 0x30EF, 0x30F0, 0x30F1, 0x30F2, 0xFFFF};
static unsigned short unicode_unacc_data171[] = {
    0x30A2, 0x30FC, 0x30C8, 0x30CF, 0x30A2, 0x30EB, 0x30D5, 0x30A1, 0x30A2, 0x30F3, 0x30A2, 0x30D8,
    0x30A2, 0x30FC, 0x30EB, 0x30A4, 0x30CB, 0x30F3, 0x30AF, 0x30A4, 0x30F3, 0x30C1, 0x30A6, 0x30A9,
    0x30F3, 0x30A8, 0x30B9, 0x30AF, 0x30FC, 0x30C8, 0x30A8, 0x30FC, 0x30AB, 0x30FC, 0x30AA, 0x30F3,
    0x30B9, 0x30AA, 0x30FC, 0x30E0, 0x30AB, 0x30A4, 0x30EA, 0x30AB, 0x30E9, 0x30C3, 0x30C8, 0x30AB,
    0x30ED, 0x30EA, 0x30FC, 0x30ED, 0x30F3, 0x30AB, 0x30F3, 0x30DE, 0x30AB, 0x30AD, 0x30AB, 0x30CB,
    0x30FC, 0x30AD, 0x30AD, 0x30E5, 0x30EA, 0x30FC, 0x30EB, 0x30FC, 0x30AD, 0x30BF, 0x30AD, 0x30ED,
    0x30AD, 0x30ED, 0x30E9, 0x30E0, 0x30AF, 0x30AD, 0x30ED, 0x30E1, 0x30FC, 0x30C8, 0x30EB, 0x30AD,
    0x30ED, 0x30EF, 0x30C3, 0x30C8, 0x30E9, 0x30E0, 0x30AF, 0x30E9, 0x30E0, 0x30C8, 0x30F3, 0x30AF,
    0x30AF, 0x30EB, 0x30A4, 0x30ED, 0x30BB, 0x30AF, 0x30ED, 0x30FC, 0x30CD, 0x30B1, 0x30FC, 0x30B9,
    0x30B3, 0x30EB, 0x30CA, 0x30B3, 0x30FC, 0x30DB, 0x30B5, 0x30A4, 0x30AF, 0x30EB};
static unsigned short unicode_unacc_data172[] = {
    0x30B5, 0x30F3, 0x30C1, 0x30FC, 0x30E0, 0x30B7, 0x30EA, 0x30F3, 0x30AF, 0x30BB, 0x30F3, 0x30C1,
    0x30BB, 0x30F3, 0x30C8, 0x30FC, 0x30B9, 0x30BF, 0x30B7, 0x30C6, 0x30EB, 0x30C8, 0x30C8, 0x30F3,
    0x30CA, 0x30CE, 0x30CE, 0x30C3, 0x30C8, 0x30CF, 0x30A4, 0x30C4, 0x30FC, 0x30BB, 0x30F3, 0x30C8,
    0x30CF, 0x30FC, 0x30C4, 0x30CF, 0x30FC, 0x30EC, 0x30EB, 0x30CF, 0x30A2, 0x30B9, 0x30C8, 0x30EB,
    0x30D2, 0x30AF, 0x30EB, 0x30D2, 0x30B3, 0x30D2, 0x30EB, 0x30D2, 0x30D5, 0x30A1, 0x30E9, 0x30C3,
    0x30C8, 0x30D5, 0x30A3, 0x30FC, 0x30C8, 0x30C3, 0x30B7, 0x30A7, 0x30EB, 0x30D5, 0x30D5, 0x30E9,
    0x30F3, 0x30D8, 0x30AF, 0x30BF, 0x30FC, 0x30EB, 0x30BD, 0x30D8, 0x30CB, 0x30D2, 0x30D8, 0x30D8,
    0x30EB, 0x30C4, 0x30F3, 0x30B9, 0x30D8, 0x30FC, 0x30D8, 0x30B7, 0x30FC, 0x30BF, 0x30D8, 0x30A4,
    0x30F3, 0x30C8, 0x30DB, 0x30EB, 0x30C8, 0x30DB, 0x30DB, 0x30F3};
static unsigned short unicode_unacc_data173[] = {
    0x30F3, 0x30DB, 0x30C8, 0x30DB, 0x30FC, 0x30EB, 0x30DB, 0x30FC, 0x30F3, 0x30DE, 0x30A4, 0x30AF,
    0x30ED, 0x30DE, 0x30A4, 0x30EB, 0x30DE, 0x30C3, 0x30CF, 0x30DE, 0x30EB, 0x30AF, 0x30DE, 0x30F3,
    0x30B7, 0x30E7, 0x30F3, 0x30DF, 0x30AF, 0x30ED, 0x30F3, 0x30DF, 0x30EA, 0x30DF, 0x30EA, 0x30FC,
    0x30EB, 0x30CF, 0x30E1, 0x30AB, 0x30E1, 0x30C8, 0x30F3, 0x30AB, 0x30E1, 0x30FC, 0x30C8, 0x30EB,
    0x30E4, 0x30FC, 0x30C8, 0x30E4, 0x30FC, 0x30EB, 0x30E6, 0x30A2, 0x30F3, 0x30EA, 0x30C3, 0x30C8,
    0x30EB, 0x30EA, 0x30E9, 0x30EB, 0x30FC, 0x30D2, 0x30EB, 0x30FC, 0x30EB, 0x30D5, 0x30EC, 0x30E0,
    0x30EC, 0x30F3, 0x30C8, 0x30F3, 0x30B1, 0x30EF, 0x30C3, 0x30C8, 0x0030, 0x70B9, 0x0031, 0x70B9,
    0x0032, 0x70B9, 0x0033, 0x70B9, 0x0034, 0x70B9, 0x0035, 0x70B9, 0x0036, 0x70B9, 0x0037, 0x70B9};
static unsigned short unicode_unacc_data174[] = {
    0x0038, 0x70B9, 0x0039, 0x70B9, 0x0031, 0x0030, 0x70B9, 0x0031, 0x0031, 0x70B9, 0x0031,
    0x0032, 0x70B9, 0x0031, 0x0033, 0x70B9, 0x0031, 0x0034, 0x70B9, 0x0031, 0x0035, 0x70B9,
    0x0031, 0x0036, 0x70B9, 0x0031, 0x0037, 0x70B9, 0x0031, 0x0038, 0x70B9, 0x0031, 0x0039,
    0x70B9, 0x0032, 0x0030, 0x70B9, 0x0032, 0x0031, 0x70B9, 0x0032, 0x0032, 0x70B9, 0x0032,
    0x0033, 0x70B9, 0x0032, 0x0034, 0x70B9, 0x0068, 0x0050, 0x0061, 0x0064, 0x0061, 0x0041,
    0x0055, 0x0062, 0x0061, 0x0072, 0x006F, 0x0056, 0x0070, 0x0063, 0x0064, 0x006D, 0x0064,
    0x006D, 0x0032, 0x0064, 0x006D, 0x0033, 0x0049, 0x0055, 0x5E73, 0x6210, 0x662D, 0x548C,
    0x5927, 0x6B63, 0x660E, 0x6CBB, 0x682A, 0x5F0F, 0x4F1A, 0x793E};
static unsigned short unicode_unacc_data175[] = {
    0x0070, 0x0041, 0x006E, 0x0041, 0x03BC, 0x0041, 0x006D, 0x0041, 0x006B, 0x0041, 0x004B, 0x0042,
    0x004D, 0x0042, 0x0047, 0x0042, 0x0063, 0x0061, 0x006C, 0x006B, 0x0063, 0x0061, 0x006C, 0x0070,
    0x0046, 0x006E, 0x0046, 0x03BC, 0x0046, 0x03BC, 0x0067, 0x006D, 0x0067, 0x006B, 0x0067, 0x0048,
    0x007A, 0x006B, 0x0048, 0x007A, 0x004D, 0x0048, 0x007A, 0x0047, 0x0048, 0x007A, 0x0054, 0x0048,
    0x007A, 0x03BC, 0x006C, 0x006D, 0x006C, 0x0064, 0x006C, 0x006B, 0x006C, 0x0066, 0x006D, 0x006E,
    0x006D, 0x03BC, 0x006D, 0x006D, 0x006D, 0x0063, 0x006D, 0x006B, 0x006D, 0x006D, 0x006D, 0x0032};
static unsigned short unicode_unacc_data176[] = {
    0x0063, 0x006D, 0x0032, 0x006D, 0x0032, 0x006B, 0x006D, 0x0032, 0x006D, 0x006D, 0x0033, 0x0063,
    0x006D, 0x0033, 0x006D, 0x0033, 0x006B, 0x006D, 0x0033, 0x006D, 0x2215, 0x0073, 0x006D, 0x2215,
    0x0073, 0x0032, 0x0050, 0x0061, 0x006B, 0x0050, 0x0061, 0x004D, 0x0050, 0x0061, 0x0047, 0x0050,
    0x0061, 0x0072, 0x0061, 0x0064, 0x0072, 0x0061, 0x0064, 0x2215, 0x0073, 0x0072, 0x0061, 0x0064,
    0x2215, 0x0073, 0x0032, 0x0070, 0x0073, 0x006E, 0x0073, 0x03BC, 0x0073, 0x006D, 0x0073, 0x0070,
    0x0056, 0x006E, 0x0056, 0x03BC, 0x0056, 0x006D, 0x0056, 0x006B, 0x0056, 0x004D, 0x0056, 0x0070,
    0x0057, 0x006E, 0x0057, 0x03BC, 0x0057, 0x006D, 0x0057, 0x006B, 0x0057, 0x004D, 0x0057};
static unsigned short unicode_unacc_data177[] = {
    0x006B, 0x03A9, 0x004D, 0x03A9, 0x0061, 0x002E, 0x006D, 0x002E, 0x0042, 0x0071, 0x0063,
    0x0063, 0x0063, 0x0064, 0x0043, 0x2215, 0x006B, 0x0067, 0x0043, 0x006F, 0x002E, 0x0064,
    0x0042, 0x0047, 0x0079, 0x0068, 0x0061, 0x0048, 0x0050, 0x0069, 0x006E, 0x004B, 0x004B,
    0x004B, 0x004D, 0x006B, 0x0074, 0x006C, 0x006D, 0x006C, 0x006E, 0x006C, 0x006F, 0x0067,
    0x006C, 0x0078, 0x006D, 0x0062, 0x006D, 0x0069, 0x006C, 0x006D, 0x006F, 0x006C, 0x0050,
    0x0048, 0x0070, 0x002E, 0x006D, 0x002E, 0x0050, 0x0050, 0x004D, 0x0050, 0x0052, 0x0073,
    0x0072, 0x0053, 0x0076, 0x0057, 0x0062, 0x0056, 0x2215, 0x006D, 0x0041, 0x2215, 0x006D};
static unsigned short unicode_unacc_data178[] = {
    0x0031, 0x65E5, 0x0032, 0x65E5, 0x0033, 0x65E5, 0x0034, 0x65E5, 0x0035, 0x65E5, 0x0036,
    0x65E5, 0x0037, 0x65E5, 0x0038, 0x65E5, 0x0039, 0x65E5, 0x0031, 0x0030, 0x65E5, 0x0031,
    0x0031, 0x65E5, 0x0031, 0x0032, 0x65E5, 0x0031, 0x0033, 0x65E5, 0x0031, 0x0034, 0x65E5,
    0x0031, 0x0035, 0x65E5, 0x0031, 0x0036, 0x65E5, 0x0031, 0x0037, 0x65E5, 0x0031, 0x0038,
    0x65E5, 0x0031, 0x0039, 0x65E5, 0x0032, 0x0030, 0x65E5, 0x0032, 0x0031, 0x65E5, 0x0032,
    0x0032, 0x65E5, 0x0032, 0x0033, 0x65E5, 0x0032, 0x0034, 0x65E5, 0x0032, 0x0035, 0x65E5,
    0x0032, 0x0036, 0x65E5, 0x0032, 0x0037, 0x65E5, 0x0032, 0x0038, 0x65E5, 0x0032, 0x0039,
    0x65E5, 0x0033, 0x0030, 0x65E5, 0x0033, 0x0031, 0x65E5, 0x0067, 0x0061, 0x006C};
static unsigned short unicode_unacc_data179[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x042B, 0x044B, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data180[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0422,
    0x0442, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data181[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xA72C, 0xA72D, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xA738, 0xA739, 0xFFFF, 0xFFFF, 0xFFFF, 0x2184};
static unsigned short unicode_unacc_data182[] = {
    0x004B, 0x006B, 0x004B, 0x006B, 0x004B, 0x006B, 0xFFFF, 0xFFFF, 0x004C, 0x006C, 0x004F,
    0x006F, 0x004F, 0x006F, 0xFFFF, 0xFFFF, 0x0050, 0x0070, 0x0050, 0x0070, 0x0050, 0x0070,
    0x0051, 0x0071, 0x0051, 0x0071, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0056, 0x0076};
static unsigned short unicode_unacc_data183[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x00DE, 0x00FE, 0x00DE, 0x00FE, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xA76F, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data184[] = {
    0x8C48, 0x66F4, 0x8ECA, 0x8CC8, 0x6ED1, 0x4E32, 0x53E5, 0x9F9C, 0x9F9C, 0x5951, 0x91D1,
    0x5587, 0x5948, 0x61F6, 0x7669, 0x7F85, 0x863F, 0x87BA, 0x88F8, 0x908F, 0x6A02, 0x6D1B,
    0x70D9, 0x73DE, 0x843D, 0x916A, 0x99F1, 0x4E82, 0x5375, 0x6B04, 0x721B, 0x862D};
static unsigned short unicode_unacc_data185[] = {
    0x9E1E, 0x5D50, 0x6FEB, 0x85CD, 0x8964, 0x62C9, 0x81D8, 0x881F, 0x5ECA, 0x6717, 0x6D6A,
    0x72FC, 0x90CE, 0x4F86, 0x51B7, 0x52DE, 0x64C4, 0x6AD3, 0x7210, 0x76E7, 0x8001, 0x8606,
    0x865C, 0x8DEF, 0x9732, 0x9B6F, 0x9DFA, 0x788C, 0x797F, 0x7DA0, 0x83C9, 0x9304};
static unsigned short unicode_unacc_data186[] = {
    0x9E7F, 0x8AD6, 0x58DF, 0x5F04, 0x7C60, 0x807E, 0x7262, 0x78CA, 0x8CC2, 0x96F7, 0x58D8,
    0x5C62, 0x6A13, 0x6DDA, 0x6F0F, 0x7D2F, 0x7E37, 0x964B, 0x52D2, 0x808B, 0x51DC, 0x51CC,
    0x7A1C, 0x7DBE, 0x83F1, 0x9675, 0x8B80, 0x62CF, 0x6A02, 0x8AFE, 0x4E39, 0x5BE7};
static unsigned short unicode_unacc_data187[] = {
    0x6012, 0x7387, 0x7570, 0x5317, 0x78FB, 0x4FBF, 0x5FA9, 0x4E0D, 0x6CCC, 0x6578, 0x7D22,
    0x53C3, 0x585E, 0x7701, 0x8449, 0x8AAA, 0x6BBA, 0x8FB0, 0x6C88, 0x62FE, 0x82E5, 0x63A0,
    0x7565, 0x4EAE, 0x5169, 0x51C9, 0x6881, 0x7CE7, 0x826F, 0x8AD2, 0x91CF, 0x52F5};
static unsigned short unicode_unacc_data188[] = {
    0x5442, 0x5973, 0x5EEC, 0x65C5, 0x6FFE, 0x792A, 0x95AD, 0x9A6A, 0x9E97, 0x9ECE, 0x529B,
    0x66C6, 0x6B77, 0x8F62, 0x5E74, 0x6190, 0x6200, 0x649A, 0x6F23, 0x7149, 0x7489, 0x79CA,
    0x7DF4, 0x806F, 0x8F26, 0x84EE, 0x9023, 0x934A, 0x5217, 0x52A3, 0x54BD, 0x70C8};
static unsigned short unicode_unacc_data189[] = {
    0x88C2, 0x8AAA, 0x5EC9, 0x5FF5, 0x637B, 0x6BAE, 0x7C3E, 0x7375, 0x4EE4, 0x56F9, 0x5BE7,
    0x5DBA, 0x601C, 0x73B2, 0x7469, 0x7F9A, 0x8046, 0x9234, 0x96F6, 0x9748, 0x9818, 0x4F8B,
    0x79AE, 0x91B4, 0x96B8, 0x60E1, 0x4E86, 0x50DA, 0x5BEE, 0x5C3F, 0x6599, 0x6A02};
static unsigned short unicode_unacc_data190[] = {
    0x71CE, 0x7642, 0x84FC, 0x907C, 0x9F8D, 0x6688, 0x962E, 0x5289, 0x677B, 0x67F3, 0x6D41,
    0x6E9C, 0x7409, 0x7559, 0x786B, 0x7D10, 0x985E, 0x516D, 0x622E, 0x9678, 0x502B, 0x5D19,
    0x6DEA, 0x8F2A, 0x5F8B, 0x6144, 0x6817, 0x7387, 0x9686, 0x5229, 0x540F, 0x5C65};
static unsigned short unicode_unacc_data191[] = {
    0x6613, 0x674E, 0x68A8, 0x6CE5, 0x7406, 0x75E2, 0x7F79, 0x88CF, 0x88E1, 0x91CC, 0x96E2,
    0x533F, 0x6EBA, 0x541D, 0x71D0, 0x7498, 0x85FA, 0x96A3, 0x9C57, 0x9E9F, 0x6797, 0x6DCB,
    0x81E8, 0x7ACB, 0x7B20, 0x7C92, 0x72C0, 0x7099, 0x8B58, 0x4EC0, 0x8336, 0x523A};
static unsigned short unicode_unacc_data192[] = {
    0x5207, 0x5EA6, 0x62D3, 0x7CD6, 0x5B85, 0x6D1E, 0x66B4, 0x8F3B, 0x884C, 0x964D, 0x898B,
    0x5ED3, 0x5140, 0x55C0, 0xFFFF, 0xFFFF, 0x585A, 0xFFFF, 0x6674, 0xFFFF, 0xFFFF, 0x51DE,
    0x732A, 0x76CA, 0x793C, 0x795E, 0x7965, 0x798F, 0x9756, 0x7CBE, 0x7FBD, 0xFFFF};
static unsigned short unicode_unacc_data193[] = {
    0x8612, 0xFFFF, 0x8AF8, 0xFFFF, 0xFFFF, 0x9038, 0x90FD, 0xFFFF, 0xFFFF, 0xFFFF, 0x98EF,
    0x98FC, 0x9928, 0x9DB4, 0xFFFF, 0xFFFF, 0x4FAE, 0x50E7, 0x514D, 0x52C9, 0x52E4, 0x5351,
    0x559D, 0x5606, 0x5668, 0x5840, 0x58A8, 0x5C64, 0x5C6E, 0x6094, 0x6168, 0x618E};
static unsigned short unicode_unacc_data194[] = {
    0x61F2, 0x654F, 0x65E2, 0x6691, 0x6885, 0x6D77, 0x6E1A, 0x6F22, 0x716E, 0x722B, 0x7422,
    0x7891, 0x793E, 0x7949, 0x7948, 0x7950, 0x7956, 0x795D, 0x798D, 0x798E, 0x7A40, 0x7A81,
    0x7BC0, 0x7DF4, 0x7E09, 0x7E41, 0x7F72, 0x8005, 0x81ED, 0x8279, 0x8279, 0x8457};
static unsigned short unicode_unacc_data195[] = {
    0x8910, 0x8996, 0x8B01, 0x8B39, 0x8CD3, 0x8D08, 0x8FB6, 0x9038, 0x96E3, 0x97FF, 0x983B,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x4E26, 0x51B5, 0x5168, 0x4F80, 0x5145, 0x5180,
    0x52C7, 0x52FA, 0x559D, 0x5555, 0x5599, 0x55E2, 0x585A, 0x58B3, 0x5944, 0x5954};
static unsigned short unicode_unacc_data196[] = {
    0x5A62, 0x5B28, 0x5ED2, 0x5ED9, 0x5F69, 0x5FAD, 0x60D8, 0x614E, 0x6108, 0x618E, 0x6160,
    0x61F2, 0x6234, 0x63C4, 0x641C, 0x6452, 0x6556, 0x6674, 0x6717, 0x671B, 0x6756, 0x6B79,
    0x6BBA, 0x6D41, 0x6EDB, 0x6ECB, 0x6F22, 0x701E, 0x716E, 0x77A7, 0x7235, 0x72AF};
static unsigned short unicode_unacc_data197[] = {
    0x732A, 0x7471, 0x7506, 0x753B, 0x761D, 0x761F, 0x76CA, 0x76DB, 0x76F4, 0x774A, 0x7740,
    0x78CC, 0x7AB1, 0x7BC0, 0x7C7B, 0x7D5B, 0x7DF4, 0x7F3E, 0x8005, 0x8352, 0x83EF, 0x8779,
    0x8941, 0x8986, 0x8996, 0x8ABF, 0x8AF8, 0x8ACB, 0x8B01, 0x8AFE, 0x8AED, 0x8B39};
static unsigned short unicode_unacc_data198[] = {
    0x8B8A, 0x8D08, 0x8F38, 0x9072, 0x9199, 0x9276, 0x967C, 0x96E3, 0x9756, 0x97DB, 0x97FF,
    0x980B, 0x983B, 0x9B12, 0x9F9C, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x4018, 0x4039, 0xFFFF,
    0xFFFF, 0xFFFF, 0x9F43, 0x9F8E, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data199[] = {
    0x0066, 0x0066, 0x0066, 0x0069, 0x0066, 0x006C, 0x0066, 0x0066, 0x0069, 0x0066, 0x0066, 0x006C,
    0x0074, 0x0073, 0x0073, 0x0074, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0574, 0x0576, 0x0574, 0x0565, 0x0574, 0x056B, 0x057E, 0x0576,
    0x0574, 0x056D, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x05D9, 0xFFFF, 0x05F2};
static unsigned short unicode_unacc_data200[] = {
    0x05E2, 0x05D0, 0x05D3, 0x05D4, 0x05DB, 0x05DC, 0x05DD, 0x05E8, 0x05EA, 0x002B, 0x05E9,
    0x05E9, 0x05E9, 0x05E9, 0x05D0, 0x05D0, 0x05D0, 0x05D1, 0x05D2, 0x05D3, 0x05D4, 0x05D5,
    0x05D6, 0xFFFF, 0x05D8, 0x05D9, 0x05DA, 0x05DB, 0x05DC, 0xFFFF, 0x05DE, 0xFFFF};
static unsigned short unicode_unacc_data201[] = {
    0x05E0, 0x05E1, 0xFFFF, 0x05E3, 0x05E4, 0xFFFF, 0x05E6, 0x05E7, 0x05E8, 0x05E9, 0x05EA,
    0x05D5, 0x05D1, 0x05DB, 0x05E4, 0x05D0, 0x05DC, 0x0671, 0x0671, 0x067B, 0x067B, 0x067B,
    0x067B, 0x067E, 0x067E, 0x067E, 0x067E, 0x0680, 0x0680, 0x0680, 0x0680, 0x067A, 0x067A};
static unsigned short unicode_unacc_data202[] = {
    0x067A, 0x067A, 0x067F, 0x067F, 0x067F, 0x067F, 0x0679, 0x0679, 0x0679, 0x0679, 0x06A4,
    0x06A4, 0x06A4, 0x06A4, 0x06A6, 0x06A6, 0x06A6, 0x06A6, 0x0684, 0x0684, 0x0684, 0x0684,
    0x0683, 0x0683, 0x0683, 0x0683, 0x0686, 0x0686, 0x0686, 0x0686, 0x0687, 0x0687};
static unsigned short unicode_unacc_data203[] = {
    0x0687, 0x0687, 0x068D, 0x068D, 0x068C, 0x068C, 0x068E, 0x068E, 0x0688, 0x0688, 0x0698,
    0x0698, 0x0691, 0x0691, 0x06A9, 0x06A9, 0x06A9, 0x06A9, 0x06AF, 0x06AF, 0x06AF, 0x06AF,
    0x06B3, 0x06B3, 0x06B3, 0x06B3, 0x06B1, 0x06B1, 0x06B1, 0x06B1, 0x06BA, 0x06BA};
static unsigned short unicode_unacc_data204[] = {
    0x06BB, 0x06BB, 0x06BB, 0x06BB, 0x06D5, 0x06D5, 0x06C1, 0x06C1, 0x06C1, 0x06C1, 0x06BE,
    0x06BE, 0x06BE, 0x06BE, 0x06D2, 0x06D2, 0x06D2, 0x06D2, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data205[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x06AD, 0x06AD, 0x06AD,
    0x06AD, 0x06C7, 0x06C7, 0x06C6, 0x06C6, 0x06C8, 0x06C8, 0x06C7, 0x0674, 0x06CB, 0x06CB};
static unsigned short unicode_unacc_data206[] = {
    0x06C5, 0x06C5, 0x06C9, 0x06C9, 0x06D0, 0x06D0, 0x06D0, 0x06D0, 0x0649, 0x0649,
    0x0627, 0x064A, 0x0627, 0x064A, 0x06D5, 0x064A, 0x06D5, 0x064A, 0x0648, 0x064A,
    0x0648, 0x064A, 0x06C7, 0x064A, 0x06C7, 0x064A, 0x06C6, 0x064A, 0x06C6, 0x064A,
    0x06C8, 0x064A, 0x06C8, 0x064A, 0x06D0, 0x064A, 0x06D0, 0x064A, 0x06D0, 0x064A,
    0x0649, 0x064A, 0x0649, 0x064A, 0x0649, 0x064A, 0x06CC, 0x06CC, 0x06CC, 0x06CC};
static unsigned short unicode_unacc_data207[] = {
    0x062C, 0x064A, 0x062D, 0x064A, 0x0645, 0x064A, 0x0649, 0x064A, 0x064A, 0x064A, 0x0628,
    0x062C, 0x0628, 0x062D, 0x0628, 0x062E, 0x0628, 0x0645, 0x0628, 0x0649, 0x0628, 0x064A,
    0x062A, 0x062C, 0x062A, 0x062D, 0x062A, 0x062E, 0x062A, 0x0645, 0x062A, 0x0649, 0x062A,
    0x064A, 0x062B, 0x062C, 0x062B, 0x0645, 0x062B, 0x0649, 0x062B, 0x064A, 0x062C, 0x062D,
    0x062C, 0x0645, 0x062D, 0x062C, 0x062D, 0x0645, 0x062E, 0x062C, 0x062E, 0x062D, 0x062E,
    0x0645, 0x0633, 0x062C, 0x0633, 0x062D, 0x0633, 0x062E, 0x0633, 0x0645};
static unsigned short unicode_unacc_data208[] = {
    0x0635, 0x062D, 0x0635, 0x0645, 0x0636, 0x062C, 0x0636, 0x062D, 0x0636, 0x062E, 0x0636,
    0x0645, 0x0637, 0x062D, 0x0637, 0x0645, 0x0638, 0x0645, 0x0639, 0x062C, 0x0639, 0x0645,
    0x063A, 0x062C, 0x063A, 0x0645, 0x0641, 0x062C, 0x0641, 0x062D, 0x0641, 0x062E, 0x0641,
    0x0645, 0x0641, 0x0649, 0x0641, 0x064A, 0x0642, 0x062D, 0x0642, 0x0645, 0x0642, 0x0649,
    0x0642, 0x064A, 0x0643, 0x0627, 0x0643, 0x062C, 0x0643, 0x062D, 0x0643, 0x062E, 0x0643,
    0x0644, 0x0643, 0x0645, 0x0643, 0x0649, 0x0643, 0x064A, 0x0644, 0x062C};
static unsigned short unicode_unacc_data209[] = {
    0x0644, 0x062D, 0x0644, 0x062E, 0x0644, 0x0645, 0x0644, 0x0649, 0x0644, 0x064A, 0x0645, 0x062C,
    0x0645, 0x062D, 0x0645, 0x062E, 0x0645, 0x0645, 0x0645, 0x0649, 0x0645, 0x064A, 0x0646, 0x062C,
    0x0646, 0x062D, 0x0646, 0x062E, 0x0646, 0x0645, 0x0646, 0x0649, 0x0646, 0x064A, 0x0647, 0x062C,
    0x0647, 0x0645, 0x0647, 0x0649, 0x0647, 0x064A, 0x064A, 0x062C, 0x064A, 0x062D, 0x064A, 0x062E,
    0x064A, 0x0645, 0x064A, 0x0649, 0x064A, 0x064A, 0x0630, 0x0631, 0x0649, 0x0020, 0x0020};
static unsigned short unicode_unacc_data210[] = {
    0x0020, 0x0020, 0x0020, 0x0020, 0x0631, 0x064A, 0x0632, 0x064A, 0x0645, 0x064A, 0x0646, 0x064A,
    0x0649, 0x064A, 0x064A, 0x064A, 0x0628, 0x0631, 0x0628, 0x0632, 0x0628, 0x0645, 0x0628, 0x0646,
    0x0628, 0x0649, 0x0628, 0x064A, 0x062A, 0x0631, 0x062A, 0x0632, 0x062A, 0x0645, 0x062A, 0x0646,
    0x062A, 0x0649, 0x062A, 0x064A, 0x062B, 0x0631, 0x062B, 0x0632, 0x062B, 0x0645, 0x062B, 0x0646,
    0x062B, 0x0649, 0x062B, 0x064A, 0x0641, 0x0649, 0x0641, 0x064A, 0x0642, 0x0649, 0x0642, 0x064A};
static unsigned short unicode_unacc_data211[] = {
    0x0643, 0x0627, 0x0643, 0x0644, 0x0643, 0x0645, 0x0643, 0x0649, 0x0643, 0x064A, 0x0644,
    0x0645, 0x0644, 0x0649, 0x0644, 0x064A, 0x0645, 0x0627, 0x0645, 0x0645, 0x0646, 0x0631,
    0x0646, 0x0632, 0x0646, 0x0645, 0x0646, 0x0646, 0x0646, 0x0649, 0x0646, 0x064A, 0x0649,
    0x064A, 0x0631, 0x064A, 0x0632, 0x064A, 0x0645, 0x064A, 0x0646, 0x064A, 0x0649, 0x064A,
    0x064A, 0x062C, 0x064A, 0x062D, 0x064A, 0x062E, 0x064A, 0x0645, 0x064A, 0x0647, 0x064A,
    0x0628, 0x062C, 0x0628, 0x062D, 0x0628, 0x062E, 0x0628, 0x0645};
static unsigned short unicode_unacc_data212[] = {
    0x0628, 0x0647, 0x062A, 0x062C, 0x062A, 0x062D, 0x062A, 0x062E, 0x062A, 0x0645, 0x062A,
    0x0647, 0x062B, 0x0645, 0x062C, 0x062D, 0x062C, 0x0645, 0x062D, 0x062C, 0x062D, 0x0645,
    0x062E, 0x062C, 0x062E, 0x0645, 0x0633, 0x062C, 0x0633, 0x062D, 0x0633, 0x062E, 0x0633,
    0x0645, 0x0635, 0x062D, 0x0635, 0x062E, 0x0635, 0x0645, 0x0636, 0x062C, 0x0636, 0x062D,
    0x0636, 0x062E, 0x0636, 0x0645, 0x0637, 0x062D, 0x0638, 0x0645, 0x0639, 0x062C, 0x0639,
    0x0645, 0x063A, 0x062C, 0x063A, 0x0645, 0x0641, 0x062C, 0x0641, 0x062D};
static unsigned short unicode_unacc_data213[] = {
    0x0641, 0x062E, 0x0641, 0x0645, 0x0642, 0x062D, 0x0642, 0x0645, 0x0643, 0x062C, 0x0643,
    0x062D, 0x0643, 0x062E, 0x0643, 0x0644, 0x0643, 0x0645, 0x0644, 0x062C, 0x0644, 0x062D,
    0x0644, 0x062E, 0x0644, 0x0645, 0x0644, 0x0647, 0x0645, 0x062C, 0x0645, 0x062D, 0x0645,
    0x062E, 0x0645, 0x0645, 0x0646, 0x062C, 0x0646, 0x062D, 0x0646, 0x062E, 0x0646, 0x0645,
    0x0646, 0x0647, 0x0647, 0x062C, 0x0647, 0x0645, 0x0647, 0x064A, 0x062C, 0x064A, 0x062D,
    0x064A, 0x062E, 0x064A, 0x0645, 0x064A, 0x0647, 0x0645, 0x064A};
static unsigned short unicode_unacc_data214[] = {
    0x0647, 0x064A, 0x0628, 0x0645, 0x0628, 0x0647, 0x062A, 0x0645, 0x062A, 0x0647, 0x062B,
    0x0645, 0x062B, 0x0647, 0x0633, 0x0645, 0x0633, 0x0647, 0x0634, 0x0645, 0x0634, 0x0647,
    0x0643, 0x0644, 0x0643, 0x0645, 0x0644, 0x0645, 0x0646, 0x0645, 0x0646, 0x0647, 0x064A,
    0x0645, 0x064A, 0x0647, 0x0640, 0x0640, 0x0640, 0x0637, 0x0649, 0x0637, 0x064A, 0x0639,
    0x0649, 0x0639, 0x064A, 0x063A, 0x0649, 0x063A, 0x064A, 0x0633, 0x0649, 0x0633, 0x064A,
    0x0634, 0x0649, 0x0634, 0x064A, 0x062D, 0x0649};
static unsigned short unicode_unacc_data215[] = {
    0x062D, 0x064A, 0x062C, 0x0649, 0x062C, 0x064A, 0x062E, 0x0649, 0x062E, 0x064A, 0x0635,
    0x0649, 0x0635, 0x064A, 0x0636, 0x0649, 0x0636, 0x064A, 0x0634, 0x062C, 0x0634, 0x062D,
    0x0634, 0x062E, 0x0634, 0x0645, 0x0634, 0x0631, 0x0633, 0x0631, 0x0635, 0x0631, 0x0636,
    0x0631, 0x0637, 0x0649, 0x0637, 0x064A, 0x0639, 0x0649, 0x0639, 0x064A, 0x063A, 0x0649,
    0x063A, 0x064A, 0x0633, 0x0649, 0x0633, 0x064A, 0x0634, 0x0649, 0x0634, 0x064A, 0x062D,
    0x0649, 0x062D, 0x064A, 0x062C, 0x0649, 0x062C, 0x064A, 0x062E, 0x0649};
static unsigned short unicode_unacc_data216[] = {
    0x062E, 0x064A, 0x0635, 0x0649, 0x0635, 0x064A, 0x0636, 0x0649, 0x0636, 0x064A, 0x0634, 0x062C,
    0x0634, 0x062D, 0x0634, 0x062E, 0x0634, 0x0645, 0x0634, 0x0631, 0x0633, 0x0631, 0x0635, 0x0631,
    0x0636, 0x0631, 0x0634, 0x062C, 0x0634, 0x062D, 0x0634, 0x062E, 0x0634, 0x0645, 0x0633, 0x0647,
    0x0634, 0x0647, 0x0637, 0x0645, 0x0633, 0x062C, 0x0633, 0x062D, 0x0633, 0x062E, 0x0634, 0x062C,
    0x0634, 0x062D, 0x0634, 0x062E, 0x0637, 0x0645, 0x0638, 0x0645, 0x0627, 0x0627, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data217[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x062A, 0x062C, 0x0645, 0x062A, 0x062D, 0x062C,
    0x062A, 0x062D, 0x062C, 0x062A, 0x062D, 0x0645, 0x062A, 0x062E, 0x0645, 0x062A, 0x0645,
    0x062C, 0x062A, 0x0645, 0x062D, 0x062A, 0x0645, 0x062E, 0x062C, 0x0645, 0x062D, 0x062C,
    0x0645, 0x062D, 0x062D, 0x0645, 0x064A, 0x062D, 0x0645, 0x0649, 0x0633, 0x062D, 0x062C,
    0x0633, 0x062C, 0x062D, 0x0633, 0x062C, 0x0649, 0x0633, 0x0645, 0x062D};
static unsigned short unicode_unacc_data218[] = {
    0x0633, 0x0645, 0x062D, 0x0633, 0x0645, 0x062C, 0x0633, 0x0645, 0x0645, 0x0633, 0x0645, 0x0645,
    0x0635, 0x062D, 0x062D, 0x0635, 0x062D, 0x062D, 0x0635, 0x0645, 0x0645, 0x0634, 0x062D, 0x0645,
    0x0634, 0x062D, 0x0645, 0x0634, 0x062C, 0x064A, 0x0634, 0x0645, 0x062E, 0x0634, 0x0645, 0x062E,
    0x0634, 0x0645, 0x0645, 0x0634, 0x0645, 0x0645, 0x0636, 0x062D, 0x0649, 0x0636, 0x062E, 0x0645,
    0x0636, 0x062E, 0x0645, 0x0637, 0x0645, 0x062D, 0x0637, 0x0645, 0x062D, 0x0637, 0x0645, 0x0645,
    0x0637, 0x0645, 0x064A, 0x0639, 0x062C, 0x0645, 0x0639, 0x0645, 0x0645, 0x0639, 0x0645, 0x0645,
    0x0639, 0x0645, 0x0649, 0x063A, 0x0645, 0x0645, 0x063A, 0x0645, 0x064A, 0x063A, 0x0645, 0x0649,
    0x0641, 0x062E, 0x0645, 0x0641, 0x062E, 0x0645, 0x0642, 0x0645, 0x062D, 0x0642, 0x0645, 0x0645};
static unsigned short unicode_unacc_data219[] = {
    0x0644, 0x062D, 0x0645, 0x0644, 0x062D, 0x064A, 0x0644, 0x062D, 0x0649, 0x0644, 0x062C, 0x062C,
    0x0644, 0x062C, 0x062C, 0x0644, 0x062E, 0x0645, 0x0644, 0x062E, 0x0645, 0x0644, 0x0645, 0x062D,
    0x0644, 0x0645, 0x062D, 0x0645, 0x062D, 0x062C, 0x0645, 0x062D, 0x0645, 0x0645, 0x062D, 0x064A,
    0x0645, 0x062C, 0x062D, 0x0645, 0x062C, 0x0645, 0x0645, 0x062E, 0x062C, 0x0645, 0x062E, 0x0645,
    0xFFFF, 0xFFFF, 0x0645, 0x062C, 0x062E, 0x0647, 0x0645, 0x062C, 0x0647, 0x0645, 0x0645, 0x0646,
    0x062D, 0x0645, 0x0646, 0x062D, 0x0649, 0x0646, 0x062C, 0x0645, 0x0646, 0x062C, 0x0645, 0x0646,
    0x062C, 0x0649, 0x0646, 0x0645, 0x064A, 0x0646, 0x0645, 0x0649, 0x064A, 0x0645, 0x0645, 0x064A,
    0x0645, 0x0645, 0x0628, 0x062E, 0x064A, 0x062A, 0x062C, 0x064A};
static unsigned short unicode_unacc_data220[] = {
    0x062A, 0x062C, 0x0649, 0x062A, 0x062E, 0x064A, 0x062A, 0x062E, 0x0649, 0x062A, 0x0645, 0x064A,
    0x062A, 0x0645, 0x0649, 0x062C, 0x0645, 0x064A, 0x062C, 0x062D, 0x0649, 0x062C, 0x0645, 0x0649,
    0x0633, 0x062E, 0x0649, 0x0635, 0x062D, 0x064A, 0x0634, 0x062D, 0x064A, 0x0636, 0x062D, 0x064A,
    0x0644, 0x062C, 0x064A, 0x0644, 0x0645, 0x064A, 0x064A, 0x062D, 0x064A, 0x064A, 0x062C, 0x064A,
    0x064A, 0x0645, 0x064A, 0x0645, 0x0645, 0x064A, 0x0642, 0x0645, 0x064A, 0x0646, 0x062D, 0x064A,
    0x0642, 0x0645, 0x062D, 0x0644, 0x062D, 0x0645, 0x0639, 0x0645, 0x064A, 0x0643, 0x0645, 0x064A,
    0x0646, 0x062C, 0x062D, 0x0645, 0x062E, 0x064A, 0x0644, 0x062C, 0x0645, 0x0643, 0x0645, 0x0645,
    0x0644, 0x062C, 0x0645, 0x0646, 0x062C, 0x062D, 0x062C, 0x062D, 0x064A, 0x062D, 0x062C, 0x064A};
static unsigned short unicode_unacc_data221[] = {
    0x0645, 0x062C, 0x064A, 0x0641, 0x0645, 0x064A, 0x0628, 0x062D, 0x064A, 0x0643, 0x0645, 0x0645,
    0x0639, 0x062C, 0x0645, 0x0635, 0x0645, 0x0645, 0x0633, 0x062E, 0x064A, 0x0646, 0x062C, 0x064A,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data222[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0635, 0x0644, 0x06D2, 0x0642, 0x0644, 0x06D2,
    0x0627, 0x0644, 0x0644, 0x0647, 0x0627, 0x0643, 0x0628, 0x0631, 0x0645, 0x062D, 0x0645,
    0x062F, 0x0635, 0x0644, 0x0639, 0x0645, 0x0631, 0x0633, 0x0648, 0x0644, 0x0639, 0x0644,
    0x064A, 0x0647, 0x0648, 0x0633, 0x0644, 0x0645, 0x0635, 0x0644, 0x0649, 0x0635, 0x0644,
    0x0649, 0x0020, 0x0627, 0x0644, 0x0644, 0x0647, 0x0020, 0x0639, 0x0644, 0x064A, 0x0647,
    0x0020, 0x0648, 0x0633, 0x0644, 0x0645, 0x062C, 0x0644, 0x0020, 0x062C, 0x0644, 0x0627,
    0x0644, 0x0647, 0x0631, 0x06CC, 0x0627, 0x0644, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data223[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x002C, 0x3001, 0x3002, 0x003A, 0x003B, 0x0021, 0x003F, 0x3016,
    0x3017, 0x002E, 0x002E, 0x002E, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data224[] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x002E, 0x002E, 0x2014, 0x2013, 0x005F, 0x005F,
    0x0028, 0x0029, 0x007B, 0x007D, 0x3014, 0x3015, 0x3010, 0x3011, 0x300A, 0x300B, 0x3008};
static unsigned short unicode_unacc_data225[] = {
    0x3009, 0x300C, 0x300D, 0x300E, 0x300F, 0xFFFF, 0xFFFF, 0x005B, 0x005D, 0x0020, 0x0020,
    0x0020, 0x0020, 0x005F, 0x005F, 0x005F, 0x002C, 0x3001, 0x002E, 0xFFFF, 0x003B, 0x003A,
    0x003F, 0x0021, 0x2014, 0x0028, 0x0029, 0x007B, 0x007D, 0x3014, 0x3015, 0x0023};
static unsigned short unicode_unacc_data226[] = {
    0x0026, 0x002A, 0x002B, 0x002D, 0x003C, 0x003E, 0x003D, 0xFFFF, 0x005C, 0x0024, 0x0025,
    0x0040, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0020, 0x0640, 0x0020, 0xFFFF, 0x0020, 0xFFFF,
    0x0020, 0x0640, 0x0020, 0x0640, 0x0020, 0x0640, 0x0020, 0x0640, 0x0020, 0x0640};
static unsigned short unicode_unacc_data227[] = {
    0x0621, 0x0627, 0x0627, 0x0627, 0x0627, 0x0648, 0x0648, 0x0627, 0x0627, 0x064A, 0x064A,
    0x064A, 0x064A, 0x0627, 0x0627, 0x0628, 0x0628, 0x0628, 0x0628, 0x0629, 0x0629, 0x062A,
    0x062A, 0x062A, 0x062A, 0x062B, 0x062B, 0x062B, 0x062B, 0x062C, 0x062C, 0x062C};
static unsigned short unicode_unacc_data228[] = {
    0x062C, 0x062D, 0x062D, 0x062D, 0x062D, 0x062E, 0x062E, 0x062E, 0x062E, 0x062F, 0x062F,
    0x0630, 0x0630, 0x0631, 0x0631, 0x0632, 0x0632, 0x0633, 0x0633, 0x0633, 0x0633, 0x0634,
    0x0634, 0x0634, 0x0634, 0x0635, 0x0635, 0x0635, 0x0635, 0x0636, 0x0636, 0x0636};
static unsigned short unicode_unacc_data229[] = {
    0x0636, 0x0637, 0x0637, 0x0637, 0x0637, 0x0638, 0x0638, 0x0638, 0x0638, 0x0639, 0x0639,
    0x0639, 0x0639, 0x063A, 0x063A, 0x063A, 0x063A, 0x0641, 0x0641, 0x0641, 0x0641, 0x0642,
    0x0642, 0x0642, 0x0642, 0x0643, 0x0643, 0x0643, 0x0643, 0x0644, 0x0644, 0x0644};
static unsigned short unicode_unacc_data230[] = {
    0x0644, 0x0645, 0x0645, 0x0645, 0x0645, 0x0646, 0x0646, 0x0646, 0x0646, 0x0647,
    0x0647, 0x0647, 0x0647, 0x0648, 0x0648, 0x0649, 0x0649, 0x064A, 0x064A, 0x064A,
    0x064A, 0x0644, 0x0627, 0x0644, 0x0627, 0x0644, 0x0627, 0x0644, 0x0627, 0x0644,
    0x0627, 0x0644, 0x0627, 0x0644, 0x0627, 0x0644, 0x0627, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data231[] = {
    0xFFFF, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002A,
    0x002B, 0x002C, 0x002D, 0x002E, 0x002F, 0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035,
    0x0036, 0x0037, 0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F};
static unsigned short unicode_unacc_data232[] = {
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004A,
    0x004B, 0x004C, 0x004D, 0x004E, 0x004F, 0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055,
    0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F};
static unsigned short unicode_unacc_data233[] = {
    0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006A,
    0x006B, 0x006C, 0x006D, 0x006E, 0x006F, 0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075,
    0x0076, 0x0077, 0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x2985};
static unsigned short unicode_unacc_data234[] = {
    0x2986, 0x3002, 0x300C, 0x300D, 0x3001, 0x30FB, 0x30F2, 0x30A1, 0x30A3, 0x30A5, 0x30A7,
    0x30A9, 0x30E3, 0x30E5, 0x30E7, 0x30C3, 0x30FC, 0x30A2, 0x30A4, 0x30A6, 0x30A8, 0x30AA,
    0x30AB, 0x30AD, 0x30AF, 0x30B1, 0x30B3, 0x30B5, 0x30B7, 0x30B9, 0x30BB, 0x30BD};
static unsigned short unicode_unacc_data235[] = {
    0x30BF, 0x30C1, 0x30C4, 0x30C6, 0x30C8, 0x30CA, 0x30CB, 0x30CC, 0x30CD, 0x30CE, 0x30CF,
    0x30D2, 0x30D5, 0x30D8, 0x30DB, 0x30DE, 0x30DF, 0x30E0, 0x30E1, 0x30E2, 0x30E4, 0x30E6,
    0x30E8, 0x30E9, 0x30EA, 0x30EB, 0x30EC, 0x30ED, 0x30EF, 0x30F3, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data236[] = {
    0x1160, 0x1100, 0x1101, 0x11AA, 0x1102, 0x11AC, 0x11AD, 0x1103, 0x1104, 0x1105, 0x11B0,
    0x11B1, 0x11B2, 0x11B3, 0x11B4, 0x11B5, 0x111A, 0x1106, 0x1107, 0x1108, 0x1121, 0x1109,
    0x110A, 0x110B, 0x110C, 0x110D, 0x110E, 0x110F, 0x1110, 0x1111, 0x1112, 0xFFFF};
static unsigned short unicode_unacc_data237[] = {
    0xFFFF, 0xFFFF, 0x1161, 0x1162, 0x1163, 0x1164, 0x1165, 0x1166, 0xFFFF, 0xFFFF, 0x1167,
    0x1168, 0x1169, 0x116A, 0x116B, 0x116C, 0xFFFF, 0xFFFF, 0x116D, 0x116E, 0x116F, 0x1170,
    0x1171, 0x1172, 0xFFFF, 0xFFFF, 0x1173, 0x1174, 0x1175, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned short unicode_unacc_data238[] = {
    0x00A2, 0x00A3, 0x00AC, 0x0020, 0x00A6, 0x00A5, 0x20A9, 0xFFFF, 0x2502, 0x2190, 0x2191,
    0x2192, 0x2193, 0x25A0, 0x25CB, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};

static unsigned short* unicode_unacc_data_table[UNICODE_UNACC_BLOCK_COUNT] = {
    unicode_unacc_data0,   unicode_unacc_data1,   unicode_unacc_data2,   unicode_unacc_data3,
    unicode_unacc_data4,   unicode_unacc_data5,   unicode_unacc_data6,   unicode_unacc_data7,
    unicode_unacc_data8,   unicode_unacc_data9,   unicode_unacc_data10,  unicode_unacc_data11,
    unicode_unacc_data12,  unicode_unacc_data13,  unicode_unacc_data14,  unicode_unacc_data15,
    unicode_unacc_data16,  unicode_unacc_data17,  unicode_unacc_data18,  unicode_unacc_data19,
    unicode_unacc_data20,  unicode_unacc_data21,  unicode_unacc_data22,  unicode_unacc_data23,
    unicode_unacc_data24,  unicode_unacc_data25,  unicode_unacc_data26,  unicode_unacc_data27,
    unicode_unacc_data28,  unicode_unacc_data29,  unicode_unacc_data30,  unicode_unacc_data31,
    unicode_unacc_data32,  unicode_unacc_data33,  unicode_unacc_data34,  unicode_unacc_data35,
    unicode_unacc_data36,  unicode_unacc_data37,  unicode_unacc_data38,  unicode_unacc_data39,
    unicode_unacc_data40,  unicode_unacc_data41,  unicode_unacc_data42,  unicode_unacc_data43,
    unicode_unacc_data44,  unicode_unacc_data45,  unicode_unacc_data46,  unicode_unacc_data47,
    unicode_unacc_data48,  unicode_unacc_data49,  unicode_unacc_data50,  unicode_unacc_data51,
    unicode_unacc_data52,  unicode_unacc_data53,  unicode_unacc_data54,  unicode_unacc_data55,
    unicode_unacc_data56,  unicode_unacc_data57,  unicode_unacc_data58,  unicode_unacc_data59,
    unicode_unacc_data60,  unicode_unacc_data61,  unicode_unacc_data62,  unicode_unacc_data63,
    unicode_unacc_data64,  unicode_unacc_data65,  unicode_unacc_data66,  unicode_unacc_data67,
    unicode_unacc_data68,  unicode_unacc_data69,  unicode_unacc_data70,  unicode_unacc_data71,
    unicode_unacc_data72,  unicode_unacc_data73,  unicode_unacc_data74,  unicode_unacc_data75,
    unicode_unacc_data76,  unicode_unacc_data77,  unicode_unacc_data78,  unicode_unacc_data79,
    unicode_unacc_data80,  unicode_unacc_data81,  unicode_unacc_data82,  unicode_unacc_data83,
    unicode_unacc_data84,  unicode_unacc_data85,  unicode_unacc_data86,  unicode_unacc_data87,
    unicode_unacc_data88,  unicode_unacc_data89,  unicode_unacc_data90,  unicode_unacc_data91,
    unicode_unacc_data92,  unicode_unacc_data93,  unicode_unacc_data94,  unicode_unacc_data95,
    unicode_unacc_data96,  unicode_unacc_data97,  unicode_unacc_data98,  unicode_unacc_data99,
    unicode_unacc_data100, unicode_unacc_data101, unicode_unacc_data102, unicode_unacc_data103,
    unicode_unacc_data104, unicode_unacc_data105, unicode_unacc_data106, unicode_unacc_data107,
    unicode_unacc_data108, unicode_unacc_data109, unicode_unacc_data110, unicode_unacc_data111,
    unicode_unacc_data112, unicode_unacc_data113, unicode_unacc_data114, unicode_unacc_data115,
    unicode_unacc_data116, unicode_unacc_data117, unicode_unacc_data118, unicode_unacc_data119,
    unicode_unacc_data120, unicode_unacc_data121, unicode_unacc_data122, unicode_unacc_data123,
    unicode_unacc_data124, unicode_unacc_data125, unicode_unacc_data126, unicode_unacc_data127,
    unicode_unacc_data128, unicode_unacc_data129, unicode_unacc_data130, unicode_unacc_data131,
    unicode_unacc_data132, unicode_unacc_data133, unicode_unacc_data134, unicode_unacc_data135,
    unicode_unacc_data136, unicode_unacc_data137, unicode_unacc_data138, unicode_unacc_data139,
    unicode_unacc_data140, unicode_unacc_data141, unicode_unacc_data142, unicode_unacc_data143,
    unicode_unacc_data144, unicode_unacc_data145, unicode_unacc_data146, unicode_unacc_data147,
    unicode_unacc_data148, unicode_unacc_data149, unicode_unacc_data150, unicode_unacc_data151,
    unicode_unacc_data152, unicode_unacc_data153, unicode_unacc_data154, unicode_unacc_data155,
    unicode_unacc_data156, unicode_unacc_data157, unicode_unacc_data158, unicode_unacc_data159,
    unicode_unacc_data160, unicode_unacc_data161, unicode_unacc_data162, unicode_unacc_data163,
    unicode_unacc_data164, unicode_unacc_data165, unicode_unacc_data166, unicode_unacc_data167,
    unicode_unacc_data168, unicode_unacc_data169, unicode_unacc_data170, unicode_unacc_data171,
    unicode_unacc_data172, unicode_unacc_data173, unicode_unacc_data174, unicode_unacc_data175,
    unicode_unacc_data176, unicode_unacc_data177, unicode_unacc_data178, unicode_unacc_data179,
    unicode_unacc_data180, unicode_unacc_data181, unicode_unacc_data182, unicode_unacc_data183,
    unicode_unacc_data184, unicode_unacc_data185, unicode_unacc_data186, unicode_unacc_data187,
    unicode_unacc_data188, unicode_unacc_data189, unicode_unacc_data190, unicode_unacc_data191,
    unicode_unacc_data192, unicode_unacc_data193, unicode_unacc_data194, unicode_unacc_data195,
    unicode_unacc_data196, unicode_unacc_data197, unicode_unacc_data198, unicode_unacc_data199,
    unicode_unacc_data200, unicode_unacc_data201, unicode_unacc_data202, unicode_unacc_data203,
    unicode_unacc_data204, unicode_unacc_data205, unicode_unacc_data206, unicode_unacc_data207,
    unicode_unacc_data208, unicode_unacc_data209, unicode_unacc_data210, unicode_unacc_data211,
    unicode_unacc_data212, unicode_unacc_data213, unicode_unacc_data214, unicode_unacc_data215,
    unicode_unacc_data216, unicode_unacc_data217, unicode_unacc_data218, unicode_unacc_data219,
    unicode_unacc_data220, unicode_unacc_data221, unicode_unacc_data222, unicode_unacc_data223,
    unicode_unacc_data224, unicode_unacc_data225, unicode_unacc_data226, unicode_unacc_data227,
    unicode_unacc_data228, unicode_unacc_data229, unicode_unacc_data230, unicode_unacc_data231,
    unicode_unacc_data232, unicode_unacc_data233, unicode_unacc_data234, unicode_unacc_data235,
    unicode_unacc_data236, unicode_unacc_data237, unicode_unacc_data238};
/* Generated by builder. Do not modify. End unicode_unacc_tables */

#define unicode_unacc(c, p, l)                                                              \
    {                                                                                       \
        unsigned short index = unicode_unacc_indexes[(c) >> UNICODE_UNACC_BLOCK_SHIFT];     \
        unsigned char position = (c)&UNICODE_UNACC_BLOCK_MASK;                              \
        (p) = &(unicode_unacc_data_table[index][unicode_unacc_positions[index][position]]); \
        (l) = unicode_unacc_positions[index][position + 1] -                                \
              unicode_unacc_positions[index][position];                                     \
        if ((l) == 1 && *(p) == 0xFFFF) {                                                   \
            (p) = 0;                                                                        \
            (l) = 0;                                                                        \
        }                                                                                   \
    }
SQLITE_EXPORT u16 sqlite3_unicode_unacc(u16 c, u16** p, int* l) {
    if (c < 0x80) {
        if (l) {
            *l = 1;
            *p = &c;
        }
        return c;
    } else {
        unsigned short index = unicode_unacc_indexes[(c) >> UNICODE_UNACC_BLOCK_SHIFT];
        unsigned char position = (c)&UNICODE_UNACC_BLOCK_MASK;
        unsigned short length =
            unicode_unacc_positions[index][position + 1] - unicode_unacc_positions[index][position];
        unsigned short* pointer =
            &(unicode_unacc_data_table[index][unicode_unacc_positions[index][position]]);

        if (l) {
            *l = length;
            *p = pointer;
        }
        return ((length == 1) && (*pointer == 0xFFFF)) ? c : *pointer;
    }
}
#endif
/*************************************************************************************************
 *************************************************************************************************
 *************************************************************************************************/

/*
** Check to see if this machine uses EBCDIC.  (Yes, believe it or
** not, there are still machines out there that use EBCDIC.)
*/
#if 'A' == '\301'
#define SQLITE_EBCDIC 1
#else
#define SQLITE_ASCII 1
#endif

/*
** Assuming zIn points to the first byte of a UTF-8 character,
** advance zIn to point to the first byte of the next UTF-8 character.
*/
#define SQLITE_SKIP_UTF8(zIn)               \
    {                                       \
        if ((*(zIn++)) >= 0xc0) {           \
            while ((*zIn & 0xc0) == 0x80) { \
                zIn++;                      \
            }                               \
        }                                   \
    }

/*
** pZ is a UTF-8 encoded unicode string. If nByte is less than zero,
** return the number of unicode characters in pZ up to (but not including)
** the first 0x00 byte. If nByte is not less than zero, return the
** number of unicode characters in the first nByte of pZ (or up to
** the first 0x00, whichever comes first).
*/
SQLITE_PRIVATE int sqlite3Utf8CharLen(const char* zIn, int nByte) {
    int r = 0;
    const u8* z = (const u8*)zIn;
    const u8* zTerm;
    if (nByte >= 0) {
        zTerm = &z[nByte];
    } else {
        zTerm = (const u8*)(-1);
    }
    assert(z <= zTerm);
    while (*z != 0 && z < zTerm) {
        SQLITE_SKIP_UTF8(z);
        r++;
    }
    return r;
}

/*
** This lookup table is used to help decode the first byte of
** a multi-byte UTF8 character.
*/
static const unsigned char utf8_lookup[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x00, 0x01, 0x02, 0x03, 0x00, 0x01, 0x00, 0x00,
};

/*
** Translate a single UTF-8 character.  Return the unicode value.
**
** During translation, assume that the byte that zTerm points
** is a 0x00.
**
** Write a pointer to the next unread byte back into *pzNext.
**
** Notes On Invalid UTF-8:
**
**  *  This routine never allows a 7-bit character (0x00 through 0x7f) to
**     be encoded as a multi-byte character.  Any multi-byte character that
**     attempts to encode a value between 0x00 and 0x7f is rendered as 0xfffd.
**
**  *  This routine never allows a UTF16 surrogate value to be encoded.
**     If a multi-byte character attempts to encode a value between
**     0xd800 and 0xe000 then it is rendered as 0xfffd.
**
**  *  Bytes in the range of 0x80 through 0xbf which occur as the first
**     byte of a character are interpreted as single-byte characters
**     and rendered as themselves even though they are technically
**     invalid characters.
**
**  *  This routine accepts an infinite number of different UTF8 encodings
**     for unicode values 0x80 and greater.  It do not change over-length
**     encodings to 0xfffd as some systems recommend.
*/
#define READ_UTF8(zIn, zTerm, c)                                                    \
    c = *(zIn++);                                                                   \
    if (c >= 0xc0) {                                                                \
        c = utf8_lookup[c - 0xc0];                                                  \
        while (zIn != zTerm && (*zIn & 0xc0) == 0x80) {                             \
            c = (c << 6) + (0x3f & *(zIn++));                                       \
        }                                                                           \
        if (c < 0x80 || (c & 0xFFFFF800) == 0xD800 || (c & 0xFFFFFFFE) == 0xFFFE) { \
            c = 0xFFFD;                                                             \
        }                                                                           \
    }
SQLITE_PRIVATE int sqlite3Utf8Read(
    const unsigned char* z,      /* First byte of UTF-8 character */
    const unsigned char* zTerm,  /* Pretend this byte is 0x00 */
    const unsigned char** pzNext /* Write first byte past UTF-8 char here */
) {
    int c;
    READ_UTF8(z, zTerm, c);
    *pzNext = z;
    return c;
}

/* An array to map all upper-case characters into their corresponding
** lower-case character.
**
** SQLite only considers US-ASCII (or EBCDIC) characters.  We do not
** handle case conversions for the UTF character set since the tables
** involved are nearly as big or bigger than SQLite itself.
*/
SQLITE_PRIVATE const unsigned char sqlite3UpperToLower[] = {
#ifdef SQLITE_ASCII
    0,
    1,
    2,
    3,
    4,
    5,
    6,
    7,
    8,
    9,
    10,
    11,
    12,
    13,
    14,
    15,
    16,
    17,
    18,
    19,
    20,
    21,
    22,
    23,
    24,
    25,
    26,
    27,
    28,
    29,
    30,
    31,
    32,
    33,
    34,
    35,
    36,
    37,
    38,
    39,
    40,
    41,
    42,
    43,
    44,
    45,
    46,
    47,
    48,
    49,
    50,
    51,
    52,
    53,
    54,
    55,
    56,
    57,
    58,
    59,
    60,
    61,
    62,
    63,
    64,
    97,
    98,
    99,
    100,
    101,
    102,
    103,
    104,
    105,
    106,
    107,
    108,
    109,
    110,
    111,
    112,
    113,
    114,
    115,
    116,
    117,
    118,
    119,
    120,
    121,
    122,
    91,
    92,
    93,
    94,
    95,
    96,
    97,
    98,
    99,
    100,
    101,
    102,
    103,
    104,
    105,
    106,
    107,
    108,
    109,
    110,
    111,
    112,
    113,
    114,
    115,
    116,
    117,
    118,
    119,
    120,
    121,
    122,
    123,
    124,
    125,
    126,
    127,
    128,
    129,
    130,
    131,
    132,
    133,
    134,
    135,
    136,
    137,
    138,
    139,
    140,
    141,
    142,
    143,
    144,
    145,
    146,
    147,
    148,
    149,
    150,
    151,
    152,
    153,
    154,
    155,
    156,
    157,
    158,
    159,
    160,
    161,
    162,
    163,
    164,
    165,
    166,
    167,
    168,
    169,
    170,
    171,
    172,
    173,
    174,
    175,
    176,
    177,
    178,
    179,
    180,
    181,
    182,
    183,
    184,
    185,
    186,
    187,
    188,
    189,
    190,
    191,
    192,
    193,
    194,
    195,
    196,
    197,
    198,
    199,
    200,
    201,
    202,
    203,
    204,
    205,
    206,
    207,
    208,
    209,
    210,
    211,
    212,
    213,
    214,
    215,
    216,
    217,
    218,
    219,
    220,
    221,
    222,
    223,
    224,
    225,
    226,
    227,
    228,
    229,
    230,
    231,
    232,
    233,
    234,
    235,
    236,
    237,
    238,
    239,
    240,
    241,
    242,
    243,
    244,
    245,
    246,
    247,
    248,
    249,
    250,
    251,
    252,
    253,
    254,
    255
#endif
#ifdef SQLITE_EBCDIC
    0,
    1,
    2,
    3,
    4,
    5,
    6,
    7,
    8,
    9,
    10,
    11,
    12,
    13,
    14,
    15, /* 0x */
    16,
    17,
    18,
    19,
    20,
    21,
    22,
    23,
    24,
    25,
    26,
    27,
    28,
    29,
    30,
    31, /* 1x */
    32,
    33,
    34,
    35,
    36,
    37,
    38,
    39,
    40,
    41,
    42,
    43,
    44,
    45,
    46,
    47, /* 2x */
    48,
    49,
    50,
    51,
    52,
    53,
    54,
    55,
    56,
    57,
    58,
    59,
    60,
    61,
    62,
    63, /* 3x */
    64,
    65,
    66,
    67,
    68,
    69,
    70,
    71,
    72,
    73,
    74,
    75,
    76,
    77,
    78,
    79, /* 4x */
    80,
    81,
    82,
    83,
    84,
    85,
    86,
    87,
    88,
    89,
    90,
    91,
    92,
    93,
    94,
    95, /* 5x */
    96,
    97,
    66,
    67,
    68,
    69,
    70,
    71,
    72,
    73,
    106,
    107,
    108,
    109,
    110,
    111, /* 6x */
    112,
    81,
    82,
    83,
    84,
    85,
    86,
    87,
    88,
    89,
    122,
    123,
    124,
    125,
    126,
    127, /* 7x */
    128,
    129,
    130,
    131,
    132,
    133,
    134,
    135,
    136,
    137,
    138,
    139,
    140,
    141,
    142,
    143, /* 8x */
    144,
    145,
    146,
    147,
    148,
    149,
    150,
    151,
    152,
    153,
    154,
    155,
    156,
    157,
    156,
    159, /* 9x */
    160,
    161,
    162,
    163,
    164,
    165,
    166,
    167,
    168,
    169,
    170,
    171,
    140,
    141,
    142,
    175, /* Ax */
    176,
    177,
    178,
    179,
    180,
    181,
    182,
    183,
    184,
    185,
    186,
    187,
    188,
    189,
    190,
    191, /* Bx */
    192,
    129,
    130,
    131,
    132,
    133,
    134,
    135,
    136,
    137,
    202,
    203,
    204,
    205,
    206,
    207, /* Cx */
    208,
    145,
    146,
    147,
    148,
    149,
    150,
    151,
    152,
    153,
    218,
    219,
    220,
    221,
    222,
    223, /* Dx */
    224,
    225,
    162,
    163,
    164,
    165,
    166,
    167,
    168,
    169,
    232,
    203,
    204,
    205,
    206,
    207, /* Ex */
    239,
    240,
    241,
    242,
    243,
    244,
    245,
    246,
    247,
    248,
    249,
    219,
    220,
    221,
    222,
    255, /* Fx */
#endif
};

/*
** For LIKE and GLOB matching on EBCDIC machines, assume that every
** character is exactly one byte in size.  Also, all characters are
** able to participate in upper-case-to-lower-case mappings in EBCDIC
** whereas only characters less than 0x80 do in ASCII.
*/
/*
** <sqlite3_unicode>
** The buit-in function has been extended to accomodate UTF-8 and UTF-16
** unicode strings containing characters over the 0x80 character limit as
** per the ASCII encoding imposed by SQlite.
**
** The functions below will use the sqlite3_unicode_fold() when
** SQLITE3_UNICODE_FOLD is defined and additonally sqlite_unicode_unacc()
** when SQLITE3_UNICODE_UNACC_AUTOMATIC is defined to normilize
** UTF-8 and UTF-16 encoded strings.
*/
#if defined(SQLITE_EBCDIC)
#define sqlite3Utf8Read(A, B, C) (*(A++))
#define GlogUpperToLower(A) A = sqlite3UpperToLower[A]
#else
#if defined(SQLITE3_UNICODE_UNACC) && defined(SQLITE3_UNICODE_UNACC_AUTOMATIC) && \
    defined(SQLITE3_UNICODE_FOLD)
#define GlogUpperToLower(A) A = sqlite3_unicode_fold(sqlite3_unicode_unacc(A, 0, 0))
#elif defined(SQLITE3_UNICODE_FOLD)
#define GlogUpperToLower(A) A = sqlite3_unicode_fold(A)
#else
#define GlogUpperToLower(A)         \
    if (A < 0x80) {                 \
        A = sqlite3UpperToLower[A]; \
    }
#endif
#endif

/*
** Maximum length (in bytes) of the pattern in a LIKE or GLOB
** operator.
*/
#ifndef SQLITE_MAX_LIKE_PATTERN_LENGTH
#define SQLITE_MAX_LIKE_PATTERN_LENGTH 50000
#endif

/*
** A structure defining how to do GLOB-style comparisons.
*/
struct compareInfo {
    u8 matchAll;
    u8 matchOne;
    u8 matchSet;
    u8 noCase;
};

/* The correct SQL-92 behavior is for the LIKE operator to ignore
** case.  Thus  'a' LIKE 'A' would be true. */
static const struct compareInfo likeInfoNorm = {'%', '_', 0, 1};

/*
** Compare two UTF-8 strings for equality where the first string can
** potentially be a "glob" expression.  Return true (1) if they
** are the same and false (0) if they are different.
**
** Globbing rules:
**
**      '*'       Matches any sequence of zero or more characters.
**
**      '?'       Matches exactly one character.
**
**     [...]      Matches one character from the enclosed list of
**                characters.
**
**     [^...]     Matches one character not in the enclosed list.
**
** With the [...] and [^...] matching, a ']' character can be included
** in the list by making it the first character after '[' or '^'.  A
** range of characters can be specified using '-'.  Example:
** "[a-z]" matches any single lower-case letter.  To match a '-', make
** it the last character in the list.
**
** This routine is usually quick, but can be N**2 in the worst case.
**
** Hints: to match '*' or '?', put them in "[]".  Like this:
**
**         abc[*]xyz        Matches "abc*xyz" only
*/
static int patternCompare(
    const u8* zPattern,              /* The glob pattern */
    const u8* zString,               /* The string to compare against the glob */
    const struct compareInfo* pInfo, /* Information about how to do the compare */
    const int esc                    /* The escape character */
) {
    int c, c2;
    int invert;
    int seen;
    u8 matchOne = pInfo->matchOne;
    u8 matchAll = pInfo->matchAll;
    u8 matchSet = pInfo->matchSet;
    u8 noCase = pInfo->noCase;
    int prevEscape = 0; /* True if the previous character was 'escape' */

    while ((c = sqlite3Utf8Read(zPattern, 0, &zPattern)) != 0) {
        if (!prevEscape && c == matchAll) {
            while ((c = sqlite3Utf8Read(zPattern, 0, &zPattern)) == matchAll || c == matchOne) {
                if (c == matchOne && sqlite3Utf8Read(zString, 0, &zString) == 0) {
                    return 0;
                }
            }
            if (c == 0) {
                return 1;
            } else if (c == esc) {
                c = sqlite3Utf8Read(zPattern, 0, &zPattern);
                if (c == 0) {
                    return 0;
                }
            } else if (c == matchSet) {
                assert(esc == 0);        /* This is GLOB, not LIKE */
                assert(matchSet < 0x80); /* '[' is a single-byte character */
                while (*zString && patternCompare(&zPattern[-1], zString, pInfo, esc) == 0) {
                    SQLITE_SKIP_UTF8(zString);
                }
                return *zString != 0;
            }
            while ((c2 = sqlite3Utf8Read(zString, 0, &zString)) != 0) {
                if (noCase) {
                    GlogUpperToLower(c2);
                    GlogUpperToLower(c);
                    while (c2 != 0 && c2 != c) {
                        c2 = sqlite3Utf8Read(zString, 0, &zString);
                        GlogUpperToLower(c2);
                    }
                } else {
                    while (c2 != 0 && c2 != c) {
                        c2 = sqlite3Utf8Read(zString, 0, &zString);
                    }
                }
                if (c2 == 0)
                    return 0;
                if (patternCompare(zPattern, zString, pInfo, esc))
                    return 1;
            }
            return 0;
        } else if (!prevEscape && c == matchOne) {
            if (sqlite3Utf8Read(zString, 0, &zString) == 0) {
                return 0;
            }
        } else if (c == matchSet) {
            int prior_c = 0;
            assert(esc == 0); /* This only occurs for GLOB, not LIKE */
            seen = 0;
            invert = 0;
            c = sqlite3Utf8Read(zString, 0, &zString);
            if (c == 0)
                return 0;
            c2 = sqlite3Utf8Read(zPattern, 0, &zPattern);
            if (c2 == '^') {
                invert = 1;
                c2 = sqlite3Utf8Read(zPattern, 0, &zPattern);
            }
            if (c2 == ']') {
                if (c == ']')
                    seen = 1;
                c2 = sqlite3Utf8Read(zPattern, 0, &zPattern);
            }
            while (c2 && c2 != ']') {
                if (c2 == '-' && zPattern[0] != ']' && zPattern[0] != 0 && prior_c > 0) {
                    c2 = sqlite3Utf8Read(zPattern, 0, &zPattern);
                    if (c >= prior_c && c <= c2)
                        seen = 1;
                    prior_c = 0;
                } else {
                    if (c == c2) {
                        seen = 1;
                    }
                    prior_c = c2;
                }
                c2 = sqlite3Utf8Read(zPattern, 0, &zPattern);
            }
            if (c2 == 0 || (seen ^ invert) == 0) {
                return 0;
            }
        } else if (esc == c && !prevEscape) {
            prevEscape = 1;
        } else {
            c2 = sqlite3Utf8Read(zString, 0, &zString);
            if (noCase) {
                GlogUpperToLower(c);
                GlogUpperToLower(c2);
            }
            if (c != c2) {
                return 0;
            }
            prevEscape = 0;
        }
    }
    return *zString == 0;
}

/*
** Count the number of times that the LIKE operator (or GLOB which is
** just a variation of LIKE) gets called.  This is used for testing
** only.
*/
#ifdef SQLITE_TEST
SQLITE_API int sqlite3_like_count = 0;
#endif

/*
** Implementation of the like() SQL function.  This function implements
** the build-in LIKE operator.  The first argument to the function is the
** pattern and the second argument is the string.  So, the SQL statements:
**
**       A LIKE B
**
** is implemented as like(B,A).
**
** This same function (with a different compareInfo structure) computes
** the GLOB operator.
*/
static void likeFunc(sqlite3_context* context, int argc, sqlite3_value** argv) {
    const unsigned char *zA, *zB;
    int escape = 0;
#if 0
  sqlite3 *db = sqlite3_context_db_handle(context);*/
#endif
    zB = sqlite3_value_text(argv[0]);
    zA = sqlite3_value_text(argv[1]);

    /* Limit the length of the LIKE or GLOB pattern to avoid problems
     ** of deep recursion and N*N behavior in patternCompare().
     */
#if 0
  if( sqlite3_value_bytes(argv[0]) >
        db->aLimit[SQLITE_LIMIT_LIKE_PATTERN_LENGTH] ){
#endif
#if 1
    if (sqlite3_value_bytes(argv[0]) > SQLITE_MAX_LIKE_PATTERN_LENGTH) {
#endif
        sqlite3_result_error(context, "LIKE or GLOB pattern too complex", -1);
        return;
    }

    assert(zB == sqlite3_value_text(argv[0])); /* Encoding did not change */

    if (argc == 3) {
        /* The escape character string must consist of a single UTF-8 character.
         ** Otherwise, return an error.
         */
        const unsigned char* zEsc = sqlite3_value_text(argv[2]);
        if (zEsc == 0)
            return;
        if (sqlite3Utf8CharLen((char*)zEsc, -1) != 1) {
            sqlite3_result_error(context, "ESCAPE expression must be a single character", -1);
            return;
        }
        escape = sqlite3Utf8Read(zEsc, 0, &zEsc);
    }
    if (zA && zB) {
        struct compareInfo* pInfo = sqlite3_user_data(context);
#ifdef SQLITE_TEST
        sqlite3_like_count++;
#endif

        sqlite3_result_int(context, patternCompare(zB, zA, pInfo, escape));
    }
}

/*
** Allocate nByte bytes of space using sqlite3_malloc(). If the
** allocation fails, call sqlite3_result_error_nomem() to notify
** the database handle that malloc() has failed.
*/
static void* contextMalloc(sqlite3_context* context, i64 nByte) {
    char* z;
#if 0
  if( nByte>sqlite3_context_db_handle(context)->aLimit[SQLITE_LIMIT_LENGTH] ){
    sqlite3_result_error_toobig(context);
    z = 0;
  }else{
#endif
    z = sqlite3_malloc((int)nByte);
    if (!z && nByte > 0) {
        sqlite3_result_error_nomem(context);
    }
#if 0
  }
#endif
    return z;
}

/*
** <sqlite3_unicode>
** Reallocate nByte bytes of space using sqlite3_realloc(). If the
** allocation fails, call sqlite3_result_error_nomem() to notify
** the database handle that malloc() has failed.
**
** SQlite has not supplied us with a reallocate function so we build our own.
*/
SQLITE_PRIVATE void* contextRealloc(sqlite3_context* context, void* pPrior, i64 nByte) {
    char* z = sqlite3_realloc(pPrior, (int)nByte);
    if (!z && nByte > 0) {
        sqlite3_result_error_nomem(context);
    }
    return z;
}

#if (defined(SQLITE3_UNICODE_FOLD) || defined(SQLITE3_UNICODE_LOWER) || \
     defined(SQLITE3_UNICODE_UPPER) || defined(SQLITE3_UNICODE_TITLE))
/*
** <sqlite3_unicode>
** Implementation of the FOLD(), UPPER(), LOWER(), TITLE() SQL functions.
** This function case folds each character in the supplied string to its
** single character equivalent.
**
** The conversion to be made depends on the contents of (sqlite3_context *)context
** where a pointer to a specific case conversion function is stored.
*/
SQLITE_PRIVATE void caseFunc(sqlite3_context* context, int argc, sqlite3_value** argv) {
    u16* z1;
    const u16* z2;
    int i, n;
    if (argc < 1 || SQLITE_NULL == sqlite3_value_type(argv[0]))
        return;
    z2 = (u16*)sqlite3_value_text16(argv[0]);
    n = sqlite3_value_bytes16(argv[0]);
    /* Verify that the call to _bytes() does not invalidate the _text() pointer */
    assert(z2 == (u16*)sqlite3_value_text16(argv[0]));
    if (z2) {
        z1 = contextMalloc(context, n + 2);
        if (z1) {
            typedef u16 (*PFN_CASEFUNC)(u16);
            memcpy(z1, z2, n + 2);
            for (i = 0; z1[i]; i++) {
                z1[i] = ((PFN_CASEFUNC)sqlite3_user_data(context))(z1[i]);
            }
            sqlite3_result_text16(context, z1, -1, sqlite3_free);
        }
    }
}
#endif

#ifdef SQLITE3_UNICODE_UNACC
/*
** <sqlite3_unicode>
** Implementation of the UNACCENT() SQL function.
** This function decomposes each character in the supplied string
** to its components and strips any accents present in the string.
**
** This function may result to a longer output string compared
** to the original input string. Memory has been properly reallocated
** to accomodate for the extra memory length required.
*/
SQLITE_PRIVATE void unaccFunc(sqlite3_context* context, int argc, sqlite3_value** argv) {
    u16* z1;
    const u16* z2;
    unsigned short* p;
    int i, o, n, l, k;
    if (argc < 1 || SQLITE_NULL == sqlite3_value_type(argv[0]))
        return;
    z2 = (u16*)sqlite3_value_text16(argv[0]);
    n = sqlite3_value_bytes16(argv[0]);
    /* Verify that the call to _bytes() does not invalidate the _text() pointer */
    assert(z2 == (u16*)sqlite3_value_text16(argv[0]));
    if (z2) {
        z1 = contextMalloc(context, n + 2);
        if (z1) {
            memcpy(z1, z2, n + 2);
            for (i = 0, o = 0; z2[i]; i++, o++) {
                unicode_unacc(z2[i], p, l);
                if (l > 0) {
                    if (l > 1) {
                        n += (l - 1) * sizeof(u16);
                        z1 = contextRealloc(context, z1, n + 2);
                    }
                    for (k = 0; k < l; k++)
                        z1[o + k] = p[k];
                    o += --k;
                } else
                    z1[o] = z2[i];
            }
            z1[o] = 0;
            sqlite3_result_text16(context, z1, -1, sqlite3_free);
        }
    }
}
#endif

#if defined(SQLITE3_UNICODE_COLLATE) && defined(SQLITE3_UNICODE_FOLD)

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

/*
** Some systems have stricmp().  Others have strcasecmp().  Because
** there is no consistency, we will define our own.
*/
/*
** <sqlite3_unicode>
** The buit-in function has been extended to accomodate UTF-8 and UTF-16
** unicode strings containing characters over the 0x80 character limit as
** per the ASCII encoding imposed by SQlite.
**
** The functions below will use the sqlite3_unicode_fold() when
** SQLITE3_UNICODE_FOLD is defined and additonally sqlite_unicode_unacc()
** when SQLITE3_UNICODE_UNACC_AUTOMATIC is defined to normilize
** UTF-8 and UTF-16 encoded strings and then compaire them for equality.
*/
SQLITE_PRIVATE int sqlite3StrNICmp(const unsigned char* zLeft, const unsigned char* zRight, int N) {
    const unsigned char* a = zLeft;
    const unsigned char* b = zRight;
    signed int ua = 0, ub = 0;
    int Z = 0;

    do {
        ua = sqlite3Utf8Read(a, 0, &a);
        ub = sqlite3Utf8Read(b, 0, &b);
        GlogUpperToLower(ua);
        GlogUpperToLower(ub);
        Z = (int)max(a - zLeft, b - zRight);
    } while (N > Z && *a != 0 && ua == ub);
    return N < 0 ? 0 : ua - ub;
}
SQLITE_PRIVATE int sqlite3StrNICmp16(const void* zLeft, const void* zRight, int N) {
    const unsigned short* a = zLeft;
    const unsigned short* b = zRight;
    signed int ua = 0, ub = 0;

    do {
        ua = *a;
        ub = *b;
        GlogUpperToLower(ua);
        GlogUpperToLower(ub);
        a++;
        b++;
    } while (--N > 0 && *a != 0 && ua == ub);
    return N < 0 ? 0 : ua - ub;
}

/*
** Another built-in collating sequence: NOCASE.
**
** This collating sequence is intended to be used for "case independant
** comparison". SQLite's knowledge of upper and lower case equivalents
** extends only to the 26 characters used in the English language.
**
** At the moment there is only a UTF-8 implementation.
*/
/*
** <sqlite3_unicode>
** The built-in collating sequence: NOCASE is extended to accomodate the
** unicode case folding mapping tables to normalize characters to their
** fold equivalents and test them for equality.
**
** Both UTF-8 and UTF-16 implementations are supported.
**
** (void *)encoding takes the following values
**   * SQLITE_UTF8  for UTF-8  encoded string comparison
**   * SQLITE_UFT16 for UTF-16 encoded string comparison
*/
SQLITE_EXPORT int sqlite3_unicode_collate(void* encoding,
                                          int nKey1,
                                          const void* pKey1,
                                          int nKey2,
                                          const void* pKey2) {
    (void)sqlite3UpperToLower;
    int r = 0;

    if ((void*)SQLITE_UTF8 == encoding)
        r = sqlite3StrNICmp((const unsigned char*)pKey1, (const unsigned char*)pKey2,
                            (nKey1 < nKey2) ? nKey1 : nKey2);
    else if ((void*)SQLITE_UTF16 == encoding)
        r = sqlite3StrNICmp16((const void*)pKey1, (const void*)pKey2,
                              (nKey1 < nKey2) ? nKey1 : nKey2);

    if (0 == r) {
        r = nKey1 - nKey2;
    }
    return r;
}
#endif

/*
** <sqlite3_unicode>
** Implementation of the UNICODE_VERSION(*) function.  The result is the version
** of the unicode library that is running.
*/
SQLITE_PRIVATE void versionFunc(sqlite3_context* context, int argc, sqlite3_value** argv) {
    sqlite3_result_text(context, SQLITE3_UNICODE_VERSION_STRING, -1, SQLITE_STATIC);
}

/*
** <sqlite3_unicode>
** Register the UNICODE extension functions with database db.
*/
SQLITE_EXPORT int sqlite3_unicode_init_impl(sqlite3* db) {
    struct FuncScalar {
        const char* zName; /* Function name */
        int nArg;          /* Number of arguments */
        int enc;           /* Optimal text encoding */
        void* pContext;    /* sqlite3_user_data() context */
        void (*xFunc)(sqlite3_context*, int, sqlite3_value**);
    } scalars[] = {
        {"unicode_version", 0, SQLITE_ANY, 0, versionFunc},

#ifdef SQLITE3_UNICODE_FOLD
        {"like", 2, SQLITE_UTF16, (void*)&likeInfoNorm, likeFunc},
        {"nlike", 2, SQLITE_ANY, (void*)&likeInfoNorm, likeFunc},
        {"like", 3, SQLITE_UTF16, (void*)&likeInfoNorm, likeFunc},
        {"nlike", 3, SQLITE_ANY, (void*)&likeInfoNorm, likeFunc},

        {"casefold", 1, SQLITE_ANY, (void*)sqlite3_unicode_fold, caseFunc},
#endif
#ifdef SQLITE3_UNICODE_LOWER
        {"lower", 1, SQLITE_UTF16, (void*)sqlite3_unicode_lower, caseFunc},
        {"nlower", 1, SQLITE_ANY, (void*)sqlite3_unicode_lower, caseFunc},
#endif
#ifdef SQLITE3_UNICODE_UPPER
        {"upper", 1, SQLITE_UTF16, (void*)sqlite3_unicode_upper, caseFunc},
        {"nupper", 1, SQLITE_ANY, (void*)sqlite3_unicode_upper, caseFunc},
#endif
#ifdef SQLITE3_UNICODE_TITLE
        {"title", 1, SQLITE_ANY, (void*)sqlite3_unicode_title, caseFunc},
        {"ntitle", 1, SQLITE_ANY, (void*)sqlite3_unicode_title, caseFunc},
#endif
#ifdef SQLITE3_UNICODE_UNACC
        {"unaccent", 1, SQLITE_ANY, 0, unaccFunc},
#endif
    };

    for (size_t i = 0; i < (sizeof(scalars) / sizeof(struct FuncScalar)); i++) {
        struct FuncScalar* p = &scalars[i];
        sqlite3_create_function(db, p->zName, p->nArg, p->enc, p->pContext, p->xFunc, 0, 0);
    }

#if defined(SQLITE3_UNICODE_COLLATE) && defined(SQLITE3_UNICODE_FOLD)
    /* Also override the default NOCASE UTF-8 case-insensitive collation sequence. */
    sqlite3_create_collation(db, "NOCASE", SQLITE_UTF8, (void*)SQLITE_UTF8,
                             sqlite3_unicode_collate);
    sqlite3_create_collation(db, "NOCASE", SQLITE_UTF16, (void*)SQLITE_UTF16,
                             sqlite3_unicode_collate);
#endif

    return SQLITE_OK;
}

int unicode_init(sqlite3* db) {
    sqlite3_unicode_init_impl(db);
    return SQLITE_OK;
}

#endif // SQLEAN_ENABLE_UNICODE
#ifdef SQLEAN_ENABLE_UUID
// ---------------------------------
// src/uuid/extension.c
// ---------------------------------
// Originally from the uuid SQLite exension, Public Domain
// https://www.sqlite.org/src/file/ext/misc/uuid.c

// Modified by Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean/

// Universally Unique IDentifiers (UUIDs) in SQLite

/*
 * This SQLite extension implements functions that handling RFC-4122 UUIDs
 * Three SQL functions are implemented:
 *
 *     uuid4()       - generate a version 4 UUID as a string
 *     uuid_str(X)   - convert a UUID X into a well-formed UUID string
 *     uuid_blob(X)  - convert a UUID X into a 16-byte blob
 *
 * The output from uuid4() and uuid_str(X) are always well-formed RFC-4122
 * UUID strings in this format:
 *
 *        xxxxxxxx-xxxx-Mxxx-Nxxx-xxxxxxxxxxxx
 *
 * All of the 'x', 'M', and 'N' values are lower-case hexadecimal digits.
 * The M digit indicates the "version".  For uuid4()-generated UUIDs, the
 * version is always "4" (a random UUID).  The upper three bits of N digit
 * are the "variant".  This library only supports variant 1 (indicated
 * by values of N between '8' and 'b') as those are overwhelming the most
 * common.  Other variants are for legacy compatibility only.
 *
 * The output of uuid_blob(X) is always a 16-byte blob. The UUID input
 * string is converted in network byte order (big-endian) in accordance
 * with RFC-4122 specifications for variant-1 UUIDs.  Note that network
 * byte order is *always* used, even if the input self-identifies as a
 * variant-2 UUID.
 *
 * The input X to the uuid_str() and uuid_blob() functions can be either
 * a string or a BLOB. If it is a BLOB it must be exactly 16 bytes in
 * length or else a NULL is returned.  If the input is a string it must
 * consist of 32 hexadecimal digits, upper or lower case, optionally
 * surrounded by {...} and with optional "-" characters interposed in the
 * middle.  The flexibility of input is inspired by the PostgreSQL
 * implementation of UUID functions that accept in all of the following
 * formats:
 *
 *     A0EEBC99-9C0B-4EF8-BB6D-6BB9BD380A11
 *     {a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11}
 *     a0eebc999c0b4ef8bb6d6bb9bd380a11
 *     a0ee-bc99-9c0b-4ef8-bb6d-6bb9-bd38-0a11
 *     {a0eebc99-9c0b4ef8-bb6d6bb9-bd380a11}
 *
 * If any of the above inputs are passed into uuid_str(), the output will
 * always be in the canonical RFC-4122 format:
 *
 *     a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11
 *
 * If the X input string has too few or too many digits or contains
 * stray characters other than {, }, or -, then NULL is returned.
 */
#include <assert.h>
#include <ctype.h>
#include <string.h>

SQLITE_EXTENSION_INIT3

#if !defined(SQLITE_ASCII) && !defined(SQLITE_EBCDIC)
#define SQLITE_ASCII 1
#endif

/*
 * Translate a single byte of Hex into an integer.
 * This routine only works if h really is a valid hexadecimal
 * character:  0..9a..fA..F
 */
static unsigned char sqlite3UuidHexToInt(int h) {
    assert((h >= '0' && h <= '9') || (h >= 'a' && h <= 'f') || (h >= 'A' && h <= 'F'));
#ifdef SQLITE_ASCII
    h += 9 * (1 & (h >> 6));
#endif
#ifdef SQLITE_EBCDIC
    h += 9 * (1 & ~(h >> 4));
#endif
    return (unsigned char)(h & 0xf);
}

/*
 * Convert a 16-byte BLOB into a well-formed RFC-4122 UUID.  The output
 * buffer zStr should be at least 37 bytes in length.   The output will
 * be zero-terminated.
 */
static void sqlite3_uuid_blob_to_str(const unsigned char* aBlob, /* Input blob */
                                     unsigned char* zStr         /* Write the answer here */
) {
    static const char zDigits[] = "0123456789abcdef";
    int i, k;
    unsigned char x;
    k = 0;
    for (i = 0, k = 0x550; i < 16; i++, k = k >> 1) {
        if (k & 1) {
            zStr[0] = '-';
            zStr++;
        }
        x = aBlob[i];
        zStr[0] = zDigits[x >> 4];
        zStr[1] = zDigits[x & 0xf];
        zStr += 2;
    }
    *zStr = 0;
}

/*
 * Attempt to parse a zero-terminated input string zStr into a binary
 * UUID.  Return 0 on success, or non-zero if the input string is not
 * parsable.
 */
static int sqlite3_uuid_str_to_blob(const unsigned char* zStr, /* Input string */
                                    unsigned char* aBlob       /* Write results here */
) {
    int i;
    if (zStr[0] == '{')
        zStr++;
    for (i = 0; i < 16; i++) {
        if (zStr[0] == '-')
            zStr++;
        if (isxdigit(zStr[0]) && isxdigit(zStr[1])) {
            aBlob[i] = (sqlite3UuidHexToInt(zStr[0]) << 4) + sqlite3UuidHexToInt(zStr[1]);
            zStr += 2;
        } else {
            return 1;
        }
    }
    if (zStr[0] == '}')
        zStr++;
    return zStr[0] != 0;
}

/*
 * Render sqlite3_value pIn as a 16-byte UUID blob.  Return a pointer
 * to the blob, or NULL if the input is not well-formed.
 */
static const unsigned char* sqlite3_uuid_input_to_blob(sqlite3_value* pIn, /* Input text */
                                                       unsigned char* pBuf /* output buffer */
) {
    switch (sqlite3_value_type(pIn)) {
        case SQLITE_TEXT: {
            const unsigned char* z = sqlite3_value_text(pIn);
            if (sqlite3_uuid_str_to_blob(z, pBuf))
                return 0;
            return pBuf;
        }
        case SQLITE_BLOB: {
            int n = sqlite3_value_bytes(pIn);
            return n == 16 ? sqlite3_value_blob(pIn) : 0;
        }
        default: {
            return 0;
        }
    }
}

/*
 * uuid_generate generates a version 4 UUID as a string
 */
static void uuid_generate(sqlite3_context* context, int argc, sqlite3_value** argv) {
    unsigned char aBlob[16];
    unsigned char zStr[37];
    (void)argc;
    (void)argv;
    sqlite3_randomness(16, aBlob);
    aBlob[6] = (aBlob[6] & 0x0f) + 0x40;
    aBlob[8] = (aBlob[8] & 0x3f) + 0x80;
    sqlite3_uuid_blob_to_str(aBlob, zStr);
    sqlite3_result_text(context, (char*)zStr, 36, SQLITE_TRANSIENT);
}

/*
 * uuid_str converts a UUID X into a well-formed UUID string.
 * X can be either a string or a blob.
 */
static void uuid_str(sqlite3_context* context, int argc, sqlite3_value** argv) {
    unsigned char aBlob[16];
    unsigned char zStr[37];
    const unsigned char* pBlob;
    (void)argc;
    pBlob = sqlite3_uuid_input_to_blob(argv[0], aBlob);
    if (pBlob == 0)
        return;
    sqlite3_uuid_blob_to_str(pBlob, zStr);
    sqlite3_result_text(context, (char*)zStr, 36, SQLITE_TRANSIENT);
}

/*
 * uuid_blob converts a UUID X into a 16-byte blob.
 * X can be either a string or a blob.
 */
static void uuid_blob(sqlite3_context* context, int argc, sqlite3_value** argv) {
    unsigned char aBlob[16];
    const unsigned char* pBlob;
    (void)argc;
    pBlob = sqlite3_uuid_input_to_blob(argv[0], aBlob);
    if (pBlob == 0)
        return;
    sqlite3_result_blob(context, pBlob, 16, SQLITE_TRANSIENT);
}

int uuid_init(sqlite3* db) {
    static const int flags = SQLITE_UTF8 | SQLITE_INNOCUOUS;
    static const int det_flags = SQLITE_UTF8 | SQLITE_INNOCUOUS | SQLITE_DETERMINISTIC;
    sqlite3_create_function(db, "uuid4", 0, flags, 0, uuid_generate, 0, 0);
    /* for postgresql compatibility */
    sqlite3_create_function(db, "gen_random_uuid", 0, flags, 0, uuid_generate, 0, 0);
    sqlite3_create_function(db, "uuid_str", 1, det_flags, 0, uuid_str, 0, 0);
    sqlite3_create_function(db, "uuid_blob", 1, det_flags, 0, uuid_blob, 0, 0);
    return SQLITE_OK;
}

#endif // SQLEAN_ENABLE_UUID
#ifdef SQLEAN_ENABLE_CRYPTO
// ---------------------------------
// src/crypto/base32.c
// ---------------------------------
// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Base32 encoding/decoding (RFC 4648)

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static const char base32_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

uint8_t* base32_encode(const uint8_t* src, size_t len, size_t* out_len) {
    *out_len = ((len + 4) / 5) * 8;
    uint8_t* encoded = malloc(*out_len + 1);
    if (encoded == NULL) {
        *out_len = 0;
        return NULL;
    }

    for (size_t i = 0, j = 0; i < len;) {
        uint32_t octet0 = i < len ? src[i++] : 0;
        uint32_t octet1 = i < len ? src[i++] : 0;
        uint32_t octet2 = i < len ? src[i++] : 0;
        uint32_t octet3 = i < len ? src[i++] : 0;
        uint32_t octet4 = i < len ? src[i++] : 0;

        encoded[j++] = base32_chars[octet0 >> 3];
        encoded[j++] = base32_chars[((octet0 & 0x07) << 2) | (octet1 >> 6)];
        encoded[j++] = base32_chars[(octet1 >> 1) & 0x1F];
        encoded[j++] = base32_chars[((octet1 & 0x01) << 4) | (octet2 >> 4)];
        encoded[j++] = base32_chars[((octet2 & 0x0F) << 1) | (octet3 >> 7)];
        encoded[j++] = base32_chars[(octet3 >> 2) & 0x1F];
        encoded[j++] = base32_chars[((octet3 & 0x03) << 3) | (octet4 >> 5)];
        encoded[j++] = base32_chars[octet4 & 0x1F];
    }

    if (len % 5 != 0) {
        size_t padding = 7 - (len % 5) * 8 / 5;
        for (size_t i = 0; i < padding; i++) {
            encoded[*out_len - padding + i] = '=';
        }
    }

    encoded[*out_len] = '\0';
    return encoded;
}

uint8_t* base32_decode(const uint8_t* src, size_t len, size_t* out_len) {
    while (len > 0 && src[len - 1] == '=') {
        len--;
    }
    *out_len = len * 5 / 8;
    uint8_t* decoded = malloc(*out_len);
    if (decoded == NULL) {
        *out_len = 0;
        return NULL;
    }

    size_t bits = 0, value = 0, count = 0;
    for (size_t i = 0; i < len; i++) {
        uint8_t c = src[i];
        if (c >= 'A' && c <= 'Z') {
            c -= 'A';
        } else if (c >= '2' && c <= '7') {
            c -= '2' - 26;
        } else {
            continue;
        }
        value = (value << 5) | c;
        bits += 5;
        if (bits >= 8) {
            decoded[count++] = (uint8_t)(value >> (bits - 8));
            bits -= 8;
        }
    }
    if (bits >= 5 || (value & ((1 << bits) - 1)) != 0) {
        free(decoded);
        return NULL;
    }
    *out_len = count;
    return decoded;
}

// ---------------------------------
// src/crypto/base64.c
// ---------------------------------
// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Base64 encoding/decoding (RFC 4648)

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static const char base64_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

uint8_t* base64_encode(const uint8_t* src, size_t len, size_t* out_len) {
    uint8_t* encoded = NULL;
    size_t i, j;
    uint32_t octets;

    *out_len = ((len + 2) / 3) * 4;
    encoded = malloc(*out_len + 1);
    if (encoded == NULL) {
        *out_len = 0;
        return NULL;
    }

    for (i = 0, j = 0; i < len; i += 3, j += 4) {
        octets =
            (src[i] << 16) | ((i + 1 < len ? src[i + 1] : 0) << 8) | (i + 2 < len ? src[i + 2] : 0);
        encoded[j] = base64_chars[(octets >> 18) & 0x3f];
        encoded[j + 1] = base64_chars[(octets >> 12) & 0x3f];
        encoded[j + 2] = base64_chars[(octets >> 6) & 0x3f];
        encoded[j + 3] = base64_chars[octets & 0x3f];
    }

    if (len % 3 == 1) {
        encoded[*out_len - 1] = '=';
        encoded[*out_len - 2] = '=';
    } else if (len % 3 == 2) {
        encoded[*out_len - 1] = '=';
    }

    encoded[*out_len] = '\0';
    return encoded;
}

static const uint8_t base64_table[] = {
    // Map base64 characters to their corresponding values
    ['A'] = 0,  ['B'] = 1,  ['C'] = 2,  ['D'] = 3,  ['E'] = 4,  ['F'] = 5,  ['G'] = 6,  ['H'] = 7,
    ['I'] = 8,  ['J'] = 9,  ['K'] = 10, ['L'] = 11, ['M'] = 12, ['N'] = 13, ['O'] = 14, ['P'] = 15,
    ['Q'] = 16, ['R'] = 17, ['S'] = 18, ['T'] = 19, ['U'] = 20, ['V'] = 21, ['W'] = 22, ['X'] = 23,
    ['Y'] = 24, ['Z'] = 25, ['a'] = 26, ['b'] = 27, ['c'] = 28, ['d'] = 29, ['e'] = 30, ['f'] = 31,
    ['g'] = 32, ['h'] = 33, ['i'] = 34, ['j'] = 35, ['k'] = 36, ['l'] = 37, ['m'] = 38, ['n'] = 39,
    ['o'] = 40, ['p'] = 41, ['q'] = 42, ['r'] = 43, ['s'] = 44, ['t'] = 45, ['u'] = 46, ['v'] = 47,
    ['w'] = 48, ['x'] = 49, ['y'] = 50, ['z'] = 51, ['0'] = 52, ['1'] = 53, ['2'] = 54, ['3'] = 55,
    ['4'] = 56, ['5'] = 57, ['6'] = 58, ['7'] = 59, ['8'] = 60, ['9'] = 61, ['+'] = 62, ['/'] = 63,
};

uint8_t* base64_decode(const uint8_t* src, size_t len, size_t* out_len) {
    if (len % 4 != 0) {
        return NULL;
    }

    size_t padding = 0;
    if (src[len - 1] == '=') {
        padding++;
    }
    if (src[len - 2] == '=') {
        padding++;
    }

    *out_len = (len / 4) * 3 - padding;
    uint8_t* decoded = malloc(*out_len);
    if (decoded == NULL) {
        *out_len = 0;
        return NULL;
    }

    for (size_t i = 0, j = 0; i < len; i += 4, j += 3) {
        uint32_t block = 0;
        for (size_t k = 0; k < 4; k++) {
            block <<= 6;
            if (src[i + k] == '=') {
                padding--;
            } else {
                uint8_t index = base64_table[src[i + k]];
                if (index == 0 && src[i + k] != 'A') {
                    free(decoded);
                    return NULL;
                }
                block |= index;
            }
        }

        decoded[j] = (block >> 16) & 0xFF;
        if (j + 1 < *out_len) {
            decoded[j + 1] = (block >> 8) & 0xFF;
        }
        if (j + 2 < *out_len) {
            decoded[j + 2] = block & 0xFF;
        }
    }

    return decoded;
}

// ---------------------------------
// src/crypto/base85.c
// ---------------------------------
// Originally by Fränz Friederes, MIT License
// https://github.com/cryptii/cryptii/blob/main/src/Encoder/Ascii85.js

// Modified by Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean/

// Base85 (Ascii85) encoding/decoding

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint8_t* base85_encode(const uint8_t* src, size_t len, size_t* out_len) {
    uint8_t* encoded = malloc(len * 5 / 4 + 5);
    if (encoded == NULL) {
        *out_len = 0;
        return NULL;
    }

    // Encode each tuple of 4 bytes
    uint32_t digits[5], tuple;
    size_t pos = 0;
    for (size_t i = 0; i < len; i += 4) {
        // Read 32-bit unsigned integer from bytes following the
        // big-endian convention (most significant byte first)
        tuple = (((src[i]) << 24) + ((src[i + 1] << 16) & 0xFF0000) + ((src[i + 2] << 8) & 0xFF00) +
                 ((src[i + 3]) & 0xFF));

        if (tuple > 0) {
            // Calculate 5 digits by repeatedly dividing
            // by 85 and taking the remainder
            for (size_t j = 0; j < 5; j++) {
                digits[4 - j] = tuple % 85;
                tuple = tuple / 85;
            }

            // Omit final characters added due to bytes of padding
            size_t num_padding = 0;
            if (len < i + 4) {
                num_padding = (i + 4) - len;
            }
            for (size_t j = 0; j < 5 - num_padding; j++) {
                encoded[pos++] = digits[j] + 33;
            }
        } else {
            // An all-zero tuple is encoded as a single character
            encoded[pos++] = 'z';
        }
    }

    *out_len = len * 5 / 4 + (len % 4 ? 1 : 0);
    encoded[*out_len] = '\0';
    return encoded;
}

uint8_t* base85_decode(const uint8_t* src, size_t len, size_t* out_len) {
    uint8_t* decoded = malloc(len * 4 / 5);
    if (decoded == NULL) {
        *out_len = 0;
        return NULL;
    }

    uint8_t digits[5], tupleBytes[4];
    uint32_t tuple;
    size_t pos = 0;
    for (size_t i = 0; i < len;) {
        if (src[i] == 'z') {
            // A single character encodes an all-zero tuple
            decoded[pos++] = 0;
            decoded[pos++] = 0;
            decoded[pos++] = 0;
            decoded[pos++] = 0;
            i++;
        } else {
            // Retrieve radix-85 digits of tuple
            for (int k = 0; k < 5; k++) {
                if (i + k < len) {
                    uint8_t digit = src[i + k] - 33;
                    if (digit < 0 || digit > 84) {
                        *out_len = 0;
                        free(decoded);
                        return NULL;
                    }
                    digits[k] = digit;
                } else {
                    digits[k] = 84;  // Pad with 'u'
                }
            }

            // Create 32-bit binary number from digits and handle padding
            // tuple = a * 85^4 + b * 85^3 + c * 85^2 + d * 85 + e
            tuple = digits[0] * 52200625 + digits[1] * 614125 + digits[2] * 7225 + digits[3] * 85 +
                    digits[4];

            // Get bytes from tuple
            tupleBytes[0] = (tuple >> 24) & 0xff;
            tupleBytes[1] = (tuple >> 16) & 0xff;
            tupleBytes[2] = (tuple >> 8) & 0xff;
            tupleBytes[3] = tuple & 0xff;

            // Remove bytes of padding
            int padding = 0;
            if (i + 4 >= len) {
                padding = i + 4 - len;
            }

            // Append bytes to result
            for (int k = 0; k < 4 - padding; k++) {
                decoded[pos++] = tupleBytes[k];
            }
            i += 5;
        }
    }

    *out_len = len * 4 / 5;
    return decoded;
}

// ---------------------------------
// src/crypto/extension.c
// ---------------------------------
// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// SQLite hash and encode/decode functions.

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

SQLITE_EXTENSION_INIT3


// encoder/decoder function
typedef uint8_t* (*encdec_fn)(const uint8_t* src, size_t len, size_t* out_len);

// Generic compute hash function. Algorithm is encoded in the user data field.
static void crypto_hash(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);

    if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
        return;
    }

    void* (*init_func)() = NULL;
    void (*update_func)(void*, void*, size_t) = NULL;
    int (*final_func)(void*, void*) = NULL;
    int algo = (intptr_t)sqlite3_user_data(context);

    switch (algo) {
        case 1: /* Hardened SHA1 */
            init_func = (void*)sha1_init;
            update_func = (void*)sha1_update;
            final_func = (void*)sha1_final;
            algo = 1;
            break;
        case 5: /* MD5 */
            init_func = (void*)md5_init;
            update_func = (void*)md5_update;
            final_func = (void*)md5_final;
            algo = 1;
            break;
        case 2256: /* SHA2-256 */
            init_func = (void*)sha256_init;
            update_func = (void*)sha256_update;
            final_func = (void*)sha256_final;
            algo = 1;
            break;
        case 2384: /* SHA2-384 */
            init_func = (void*)sha384_init;
            update_func = (void*)sha384_update;
            final_func = (void*)sha384_final;
            algo = 1;
            break;
        case 2512: /* SHA2-512 */
            init_func = (void*)sha512_init;
            update_func = (void*)sha512_update;
            final_func = (void*)sha512_final;
            algo = 1;
            break;
        default:
            sqlite3_result_error(context, "unknown algorithm", -1);
            return;
    }

    void* ctx = NULL;
    if (algo) {
        ctx = init_func();
    }
    if (!ctx) {
        sqlite3_result_error(context, "could not allocate algorithm context", -1);
        return;
    }

    void* data = NULL;
    if (sqlite3_value_type(argv[0]) == SQLITE_BLOB) {
        data = (void*)sqlite3_value_blob(argv[0]);
    } else {
        data = (void*)sqlite3_value_text(argv[0]);
    }

    size_t datalen = sqlite3_value_bytes(argv[0]);
    if (datalen > 0) {
        update_func(ctx, data, datalen);
    }

    unsigned char hash[128] = {0};
    int hashlen = final_func(ctx, hash);
    sqlite3_result_blob(context, hash, hashlen, SQLITE_TRANSIENT);
}

// Encodes binary data into a textual representation using the specified encoder.
static void encode(sqlite3_context* context, int argc, sqlite3_value** argv, encdec_fn encode_fn) {
    assert(argc == 1);
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
        sqlite3_result_null(context);
        return;
    }
    size_t source_len = sqlite3_value_bytes(argv[0]);
    const uint8_t* source = (uint8_t*)sqlite3_value_blob(argv[0]);
    size_t result_len = 0;
    const char* result = (char*)encode_fn(source, source_len, &result_len);
    sqlite3_result_text(context, result, -1, free);
}

// Encodes binary data into a textual representation using the specified algorithm.
// encode('hello', 'base64') = 'aGVsbG8='
static void crypto_encode(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);
    size_t n = sqlite3_value_bytes(argv[1]);
    const char* format = (char*)sqlite3_value_text(argv[1]);
    if (strncmp(format, "base32", n) == 0) {
        encode(context, 1, argv, base32_encode);
        return;
    }
    if (strncmp(format, "base64", n) == 0) {
        encode(context, 1, argv, base64_encode);
        return;
    }
    if (strncmp(format, "base85", n) == 0) {
        encode(context, 1, argv, base85_encode);
        return;
    }
    if (strncmp(format, "hex", n) == 0) {
        encode(context, 1, argv, hex_encode);
        return;
    }
    if (strncmp(format, "url", n) == 0) {
        encode(context, 1, argv, url_encode);
        return;
    }
    sqlite3_result_error(context, "unknown encoding", -1);
}

// Decodes binary data from a textual representation using the specified decoder.
static void decode(sqlite3_context* context, int argc, sqlite3_value** argv, encdec_fn decode_fn) {
    assert(argc == 1);
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
        sqlite3_result_null(context);
        return;
    }

    size_t source_len = sqlite3_value_bytes(argv[0]);
    const uint8_t* source = (uint8_t*)sqlite3_value_text(argv[0]);
    if (source_len == 0) {
        sqlite3_result_zeroblob(context, 0);
        return;
    }

    size_t result_len = 0;
    const uint8_t* result = decode_fn(source, source_len, &result_len);
    if (result == NULL) {
        sqlite3_result_error(context, "invalid input string", -1);
        return;
    }

    sqlite3_result_blob(context, result, result_len, free);
}

// Decodes binary data from a textual representation using the specified algorithm.
// decode('aGVsbG8=', 'base64') = cast('hello' as blob)
static void crypto_decode(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);
    size_t n = sqlite3_value_bytes(argv[1]);
    const char* format = (char*)sqlite3_value_text(argv[1]);
    if (strncmp(format, "base32", n) == 0) {
        decode(context, 1, argv, base32_decode);
        return;
    }
    if (strncmp(format, "base64", n) == 0) {
        decode(context, 1, argv, base64_decode);
        return;
    }
    if (strncmp(format, "base85", n) == 0) {
        decode(context, 1, argv, base85_decode);
        return;
    }
    if (strncmp(format, "hex", n) == 0) {
        decode(context, 1, argv, hex_decode);
        return;
    }
    if (strncmp(format, "url", n) == 0) {
        decode(context, 1, argv, url_decode);
        return;
    }
    sqlite3_result_error(context, "unknown encoding", -1);
}

int crypto_init(sqlite3* db) {
    static const int flags = SQLITE_UTF8 | SQLITE_INNOCUOUS | SQLITE_DETERMINISTIC;
    sqlite3_create_function(db, "md5", 1, flags, (void*)5, crypto_hash, 0, 0);
    sqlite3_create_function(db, "sha1", 1, flags, (void*)1, crypto_hash, 0, 0);
    sqlite3_create_function(db, "sha256", 1, flags, (void*)2256, crypto_hash, 0, 0);
    sqlite3_create_function(db, "sha384", 1, flags, (void*)2384, crypto_hash, 0, 0);
    sqlite3_create_function(db, "sha512", 1, flags, (void*)2512, crypto_hash, 0, 0);
    sqlite3_create_function(db, "encode", 2, flags, 0, crypto_encode, 0, 0);
    sqlite3_create_function(db, "decode", 2, flags, 0, crypto_decode, 0, 0);
    return SQLITE_OK;
}

// ---------------------------------
// src/crypto/hex.c
// ---------------------------------
// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Hex encoding/decoding

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

uint8_t* hex_encode(const uint8_t* src, size_t len, size_t* out_len) {
    *out_len = len * 2;
    uint8_t* encoded = malloc(*out_len + 1);
    if (encoded == NULL) {
        *out_len = 0;
        return NULL;
    }
    for (size_t i = 0; i < len; i++) {
        sprintf((char*)encoded + (i * 2), "%02x", src[i]);
    }
    encoded[*out_len] = '\0';
    *out_len = len * 2;
    return encoded;
}

uint8_t* hex_decode(const uint8_t* src, size_t len, size_t* out_len) {
    if (len % 2 != 0) {
        // input length must be even
        return NULL;
    }

    size_t decoded_len = len / 2;
    uint8_t* decoded = malloc(decoded_len);
    if (decoded == NULL) {
        *out_len = 0;
        return NULL;
    }

    for (size_t i = 0; i < decoded_len; i++) {
        uint8_t hi = src[i * 2];
        uint8_t lo = src[i * 2 + 1];

        if (hi >= '0' && hi <= '9') {
            hi -= '0';
        } else if (hi >= 'A' && hi <= 'F') {
            hi -= 'A' - 10;
        } else if (hi >= 'a' && hi <= 'f') {
            hi -= 'a' - 10;
        } else {
            // invalid character
            free(decoded);
            return NULL;
        }

        if (lo >= '0' && lo <= '9') {
            lo -= '0';
        } else if (lo >= 'A' && lo <= 'F') {
            lo -= 'A' - 10;
        } else if (lo >= 'a' && lo <= 'f') {
            lo -= 'a' - 10;
        } else {
            // invalid character
            free(decoded);
            return NULL;
        }

        decoded[i] = (hi << 4) | lo;
    }

    *out_len = decoded_len;
    return decoded;
}

// ---------------------------------
// src/crypto/md5.c
// ---------------------------------
/*********************************************************************
 * Filename:   md5.c
 * Author:     Brad Conte (brad AT bradconte.com)
 * Source:     https://github.com/B-Con/crypto-algorithms
 * License:    Public Domain
 * Details:    Implementation of the MD5 hashing algorithm.
 * Algorithm specification can be found here:
 * http://tools.ietf.org/html/rfc1321
 * This implementation uses little endian byte order.
 *********************************************************************/

/*************************** HEADER FILES ***************************/
#include <memory.h>
#include <stdlib.h>

/****************************** MACROS ******************************/
#define ROTLEFT(a, b) ((a << b) | (a >> (32 - b)))

#define F(x, y, z) ((x & y) | (~x & z))
#define G(x, y, z) ((x & z) | (y & ~z))
#define H(x, y, z) (x ^ y ^ z)
#define I(x, y, z) (y ^ (x | ~z))

#define FF(a, b, c, d, m, s, t)  \
    {                            \
        a += F(b, c, d) + m + t; \
        a = b + ROTLEFT(a, s);   \
    }
#define GG(a, b, c, d, m, s, t)  \
    {                            \
        a += G(b, c, d) + m + t; \
        a = b + ROTLEFT(a, s);   \
    }
#define HH(a, b, c, d, m, s, t)  \
    {                            \
        a += H(b, c, d) + m + t; \
        a = b + ROTLEFT(a, s);   \
    }
#define II(a, b, c, d, m, s, t)  \
    {                            \
        a += I(b, c, d) + m + t; \
        a = b + ROTLEFT(a, s);   \
    }

/*********************** FUNCTION DEFINITIONS ***********************/
static void md5_transform(MD5_CTX* ctx, const BYTE data[]) {
    WORD a, b, c, d, m[16], i, j;

    // MD5 specifies big endian byte order, but this implementation assumes a little
    // endian byte order CPU. Reverse all the bytes upon input, and re-reverse them
    // on output (in md5_final()).
    for (i = 0, j = 0; i < 16; ++i, j += 4)
        m[i] = (data[j]) + (data[j + 1] << 8) + (data[j + 2] << 16) + ((WORD)data[j + 3] << 24);

    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];

    FF(a, b, c, d, m[0], 7, 0xd76aa478);
    FF(d, a, b, c, m[1], 12, 0xe8c7b756);
    FF(c, d, a, b, m[2], 17, 0x242070db);
    FF(b, c, d, a, m[3], 22, 0xc1bdceee);
    FF(a, b, c, d, m[4], 7, 0xf57c0faf);
    FF(d, a, b, c, m[5], 12, 0x4787c62a);
    FF(c, d, a, b, m[6], 17, 0xa8304613);
    FF(b, c, d, a, m[7], 22, 0xfd469501);
    FF(a, b, c, d, m[8], 7, 0x698098d8);
    FF(d, a, b, c, m[9], 12, 0x8b44f7af);
    FF(c, d, a, b, m[10], 17, 0xffff5bb1);
    FF(b, c, d, a, m[11], 22, 0x895cd7be);
    FF(a, b, c, d, m[12], 7, 0x6b901122);
    FF(d, a, b, c, m[13], 12, 0xfd987193);
    FF(c, d, a, b, m[14], 17, 0xa679438e);
    FF(b, c, d, a, m[15], 22, 0x49b40821);

    GG(a, b, c, d, m[1], 5, 0xf61e2562);
    GG(d, a, b, c, m[6], 9, 0xc040b340);
    GG(c, d, a, b, m[11], 14, 0x265e5a51);
    GG(b, c, d, a, m[0], 20, 0xe9b6c7aa);
    GG(a, b, c, d, m[5], 5, 0xd62f105d);
    GG(d, a, b, c, m[10], 9, 0x02441453);
    GG(c, d, a, b, m[15], 14, 0xd8a1e681);
    GG(b, c, d, a, m[4], 20, 0xe7d3fbc8);
    GG(a, b, c, d, m[9], 5, 0x21e1cde6);
    GG(d, a, b, c, m[14], 9, 0xc33707d6);
    GG(c, d, a, b, m[3], 14, 0xf4d50d87);
    GG(b, c, d, a, m[8], 20, 0x455a14ed);
    GG(a, b, c, d, m[13], 5, 0xa9e3e905);
    GG(d, a, b, c, m[2], 9, 0xfcefa3f8);
    GG(c, d, a, b, m[7], 14, 0x676f02d9);
    GG(b, c, d, a, m[12], 20, 0x8d2a4c8a);

    HH(a, b, c, d, m[5], 4, 0xfffa3942);
    HH(d, a, b, c, m[8], 11, 0x8771f681);
    HH(c, d, a, b, m[11], 16, 0x6d9d6122);
    HH(b, c, d, a, m[14], 23, 0xfde5380c);
    HH(a, b, c, d, m[1], 4, 0xa4beea44);
    HH(d, a, b, c, m[4], 11, 0x4bdecfa9);
    HH(c, d, a, b, m[7], 16, 0xf6bb4b60);
    HH(b, c, d, a, m[10], 23, 0xbebfbc70);
    HH(a, b, c, d, m[13], 4, 0x289b7ec6);
    HH(d, a, b, c, m[0], 11, 0xeaa127fa);
    HH(c, d, a, b, m[3], 16, 0xd4ef3085);
    HH(b, c, d, a, m[6], 23, 0x04881d05);
    HH(a, b, c, d, m[9], 4, 0xd9d4d039);
    HH(d, a, b, c, m[12], 11, 0xe6db99e5);
    HH(c, d, a, b, m[15], 16, 0x1fa27cf8);
    HH(b, c, d, a, m[2], 23, 0xc4ac5665);

    II(a, b, c, d, m[0], 6, 0xf4292244);
    II(d, a, b, c, m[7], 10, 0x432aff97);
    II(c, d, a, b, m[14], 15, 0xab9423a7);
    II(b, c, d, a, m[5], 21, 0xfc93a039);
    II(a, b, c, d, m[12], 6, 0x655b59c3);
    II(d, a, b, c, m[3], 10, 0x8f0ccc92);
    II(c, d, a, b, m[10], 15, 0xffeff47d);
    II(b, c, d, a, m[1], 21, 0x85845dd1);
    II(a, b, c, d, m[8], 6, 0x6fa87e4f);
    II(d, a, b, c, m[15], 10, 0xfe2ce6e0);
    II(c, d, a, b, m[6], 15, 0xa3014314);
    II(b, c, d, a, m[13], 21, 0x4e0811a1);
    II(a, b, c, d, m[4], 6, 0xf7537e82);
    II(d, a, b, c, m[11], 10, 0xbd3af235);
    II(c, d, a, b, m[2], 15, 0x2ad7d2bb);
    II(b, c, d, a, m[9], 21, 0xeb86d391);

    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
}

void* md5_init() {
    MD5_CTX* ctx;
    ctx = malloc(sizeof(MD5_CTX));
    ctx->datalen = 0;
    ctx->bitlen = 0;
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xEFCDAB89;
    ctx->state[2] = 0x98BADCFE;
    ctx->state[3] = 0x10325476;
    return ctx;
}

void md5_update(MD5_CTX* ctx, const BYTE data[], size_t len) {
    size_t i;

    for (i = 0; i < len; ++i) {
        ctx->data[ctx->datalen] = data[i];
        ctx->datalen++;
        if (ctx->datalen == 64) {
            md5_transform(ctx, ctx->data);
            ctx->bitlen += 512;
            ctx->datalen = 0;
        }
    }
}

int md5_final(MD5_CTX* ctx, BYTE hash[]) {
    size_t i;

    i = ctx->datalen;

    // Pad whatever data is left in the buffer.
    if (ctx->datalen < 56) {
        ctx->data[i++] = 0x80;
        while (i < 56)
            ctx->data[i++] = 0x00;
    } else if (ctx->datalen >= 56) {
        ctx->data[i++] = 0x80;
        while (i < 64)
            ctx->data[i++] = 0x00;
        md5_transform(ctx, ctx->data);
        memset(ctx->data, 0, 56);
    }

    // Append to the padding the total message's length in bits and transform.
    ctx->bitlen += ctx->datalen * 8;
    ctx->data[56] = ctx->bitlen;
    ctx->data[57] = ctx->bitlen >> 8;
    ctx->data[58] = ctx->bitlen >> 16;
    ctx->data[59] = ctx->bitlen >> 24;
    ctx->data[60] = ctx->bitlen >> 32;
    ctx->data[61] = ctx->bitlen >> 40;
    ctx->data[62] = ctx->bitlen >> 48;
    ctx->data[63] = ctx->bitlen >> 56;
    md5_transform(ctx, ctx->data);

    // Since this implementation uses little endian byte ordering and MD uses big endian,
    // reverse all the bytes when copying the final state to the output hash.
    for (i = 0; i < 4; ++i) {
        hash[i] = (ctx->state[0] >> (i * 8)) & 0x000000ff;
        hash[i + 4] = (ctx->state[1] >> (i * 8)) & 0x000000ff;
        hash[i + 8] = (ctx->state[2] >> (i * 8)) & 0x000000ff;
        hash[i + 12] = (ctx->state[3] >> (i * 8)) & 0x000000ff;
    }
    free(ctx);
    return MD5_BLOCK_SIZE;
}

// ---------------------------------
// src/crypto/sha1.c
// ---------------------------------
// Originally from the sha1 SQLite exension, Public Domain
// https://sqlite.org/src/file/ext/misc/sha1.c
// Modified by Anton Zhiyanov, https://github.com/nalgeon/sqlean/, MIT License

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>


#define SHA_ROT(x, l, r) ((x) << (l) | (x) >> (r))
#define rol(x, k) SHA_ROT(x, k, 32 - (k))
#define ror(x, k) SHA_ROT(x, 32 - (k), k)

#define blk0le(i) (block[i] = (ror(block[i], 8) & 0xFF00FF00) | (rol(block[i], 8) & 0x00FF00FF))
#define blk0be(i) block[i]
#define blk(i)       \
    (block[i & 15] = \
         rol(block[(i + 13) & 15] ^ block[(i + 8) & 15] ^ block[(i + 2) & 15] ^ block[i & 15], 1))

/*
 * (R0+R1), R2, R3, R4 are the different operations (rounds) used in SHA1
 *
 * Rl0() for little-endian and Rb0() for big-endian.  Endianness is
 * determined at run-time.
 */
#define Rl0(v, w, x, y, z, i)                                      \
    z += ((w & (x ^ y)) ^ y) + blk0le(i) + 0x5A827999 + rol(v, 5); \
    w = ror(w, 2);
#define Rb0(v, w, x, y, z, i)                                      \
    z += ((w & (x ^ y)) ^ y) + blk0be(i) + 0x5A827999 + rol(v, 5); \
    w = ror(w, 2);
#define R1(v, w, x, y, z, i)                                    \
    z += ((w & (x ^ y)) ^ y) + blk(i) + 0x5A827999 + rol(v, 5); \
    w = ror(w, 2);
#define R2(v, w, x, y, z, i)                            \
    z += (w ^ x ^ y) + blk(i) + 0x6ED9EBA1 + rol(v, 5); \
    w = ror(w, 2);
#define R3(v, w, x, y, z, i)                                          \
    z += (((w | x) & y) | (w & x)) + blk(i) + 0x8F1BBCDC + rol(v, 5); \
    w = ror(w, 2);
#define R4(v, w, x, y, z, i)                            \
    z += (w ^ x ^ y) + blk(i) + 0xCA62C1D6 + rol(v, 5); \
    w = ror(w, 2);

/*
 * Hash a single 512-bit block. This is the core of the algorithm.
 */
void SHA1Transform(unsigned int state[5], const unsigned char buffer[64]) {
    unsigned int qq[5]; /* a, b, c, d, e; */
    static int one = 1;
    unsigned int block[16];
    memcpy(block, buffer, 64);
    memcpy(qq, state, 5 * sizeof(unsigned int));

#define a qq[0]
#define b qq[1]
#define c qq[2]
#define d qq[3]
#define e qq[4]

    /* Copy ctx->state[] to working vars */
    /*
  a = state[0];
  b = state[1];
  c = state[2];
  d = state[3];
  e = state[4];
  */

    /* 4 rounds of 20 operations each. Loop unrolled. */
    if (1 == *(unsigned char*)&one) {
        Rl0(a, b, c, d, e, 0);
        Rl0(e, a, b, c, d, 1);
        Rl0(d, e, a, b, c, 2);
        Rl0(c, d, e, a, b, 3);
        Rl0(b, c, d, e, a, 4);
        Rl0(a, b, c, d, e, 5);
        Rl0(e, a, b, c, d, 6);
        Rl0(d, e, a, b, c, 7);
        Rl0(c, d, e, a, b, 8);
        Rl0(b, c, d, e, a, 9);
        Rl0(a, b, c, d, e, 10);
        Rl0(e, a, b, c, d, 11);
        Rl0(d, e, a, b, c, 12);
        Rl0(c, d, e, a, b, 13);
        Rl0(b, c, d, e, a, 14);
        Rl0(a, b, c, d, e, 15);
    } else {
        Rb0(a, b, c, d, e, 0);
        Rb0(e, a, b, c, d, 1);
        Rb0(d, e, a, b, c, 2);
        Rb0(c, d, e, a, b, 3);
        Rb0(b, c, d, e, a, 4);
        Rb0(a, b, c, d, e, 5);
        Rb0(e, a, b, c, d, 6);
        Rb0(d, e, a, b, c, 7);
        Rb0(c, d, e, a, b, 8);
        Rb0(b, c, d, e, a, 9);
        Rb0(a, b, c, d, e, 10);
        Rb0(e, a, b, c, d, 11);
        Rb0(d, e, a, b, c, 12);
        Rb0(c, d, e, a, b, 13);
        Rb0(b, c, d, e, a, 14);
        Rb0(a, b, c, d, e, 15);
    }
    R1(e, a, b, c, d, 16);
    R1(d, e, a, b, c, 17);
    R1(c, d, e, a, b, 18);
    R1(b, c, d, e, a, 19);
    R2(a, b, c, d, e, 20);
    R2(e, a, b, c, d, 21);
    R2(d, e, a, b, c, 22);
    R2(c, d, e, a, b, 23);
    R2(b, c, d, e, a, 24);
    R2(a, b, c, d, e, 25);
    R2(e, a, b, c, d, 26);
    R2(d, e, a, b, c, 27);
    R2(c, d, e, a, b, 28);
    R2(b, c, d, e, a, 29);
    R2(a, b, c, d, e, 30);
    R2(e, a, b, c, d, 31);
    R2(d, e, a, b, c, 32);
    R2(c, d, e, a, b, 33);
    R2(b, c, d, e, a, 34);
    R2(a, b, c, d, e, 35);
    R2(e, a, b, c, d, 36);
    R2(d, e, a, b, c, 37);
    R2(c, d, e, a, b, 38);
    R2(b, c, d, e, a, 39);
    R3(a, b, c, d, e, 40);
    R3(e, a, b, c, d, 41);
    R3(d, e, a, b, c, 42);
    R3(c, d, e, a, b, 43);
    R3(b, c, d, e, a, 44);
    R3(a, b, c, d, e, 45);
    R3(e, a, b, c, d, 46);
    R3(d, e, a, b, c, 47);
    R3(c, d, e, a, b, 48);
    R3(b, c, d, e, a, 49);
    R3(a, b, c, d, e, 50);
    R3(e, a, b, c, d, 51);
    R3(d, e, a, b, c, 52);
    R3(c, d, e, a, b, 53);
    R3(b, c, d, e, a, 54);
    R3(a, b, c, d, e, 55);
    R3(e, a, b, c, d, 56);
    R3(d, e, a, b, c, 57);
    R3(c, d, e, a, b, 58);
    R3(b, c, d, e, a, 59);
    R4(a, b, c, d, e, 60);
    R4(e, a, b, c, d, 61);
    R4(d, e, a, b, c, 62);
    R4(c, d, e, a, b, 63);
    R4(b, c, d, e, a, 64);
    R4(a, b, c, d, e, 65);
    R4(e, a, b, c, d, 66);
    R4(d, e, a, b, c, 67);
    R4(c, d, e, a, b, 68);
    R4(b, c, d, e, a, 69);
    R4(a, b, c, d, e, 70);
    R4(e, a, b, c, d, 71);
    R4(d, e, a, b, c, 72);
    R4(c, d, e, a, b, 73);
    R4(b, c, d, e, a, 74);
    R4(a, b, c, d, e, 75);
    R4(e, a, b, c, d, 76);
    R4(d, e, a, b, c, 77);
    R4(c, d, e, a, b, 78);
    R4(b, c, d, e, a, 79);

    /* Add the working vars back into context.state[] */
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;

#undef a
#undef b
#undef c
#undef d
#undef e
}

/* Initialize a SHA1 context */
void* sha1_init() {
    /* SHA1 initialization constants */
    SHA1Context* ctx;
    ctx = malloc(sizeof(SHA1Context));
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xEFCDAB89;
    ctx->state[2] = 0x98BADCFE;
    ctx->state[3] = 0x10325476;
    ctx->state[4] = 0xC3D2E1F0;
    ctx->count[0] = ctx->count[1] = 0;
    return ctx;
}

/* Add new content to the SHA1 hash */
void sha1_update(SHA1Context* ctx, const unsigned char* data, size_t len) {
    unsigned int i, j;

    j = ctx->count[0];
    if ((ctx->count[0] += len << 3) < j) {
        ctx->count[1] += (len >> 29) + 1;
    }
    j = (j >> 3) & 63;
    if ((j + len) > 63) {
        (void)memcpy(&ctx->buffer[j], data, (i = 64 - j));
        SHA1Transform(ctx->state, ctx->buffer);
        for (; i + 63 < len; i += 64) {
            SHA1Transform(ctx->state, &data[i]);
        }
        j = 0;
    } else {
        i = 0;
    }
    (void)memcpy(&ctx->buffer[j], &data[i], len - i);
}

int sha1_final(SHA1Context* ctx, unsigned char hash[]) {
    unsigned int i;
    unsigned char finalcount[8];

    for (i = 0; i < 8; i++) {
        finalcount[i] = (unsigned char)((ctx->count[(i >= 4 ? 0 : 1)] >> ((3 - (i & 3)) * 8)) &
                                        255); /* Endian independent */
    }
    sha1_update(ctx, (const unsigned char*)"\200", 1);
    while ((ctx->count[0] & 504) != 448) {
        sha1_update(ctx, (const unsigned char*)"\0", 1);
    }
    sha1_update(ctx, finalcount, 8); /* Should cause a SHA1Transform() */
    for (i = 0; i < 20; i++) {
        hash[i] = (unsigned char)((ctx->state[i >> 2] >> ((3 - (i & 3)) * 8)) & 255);
    }
    free(ctx);
    return SHA1_BLOCK_SIZE;
}

// ---------------------------------
// src/crypto/sha2.c
// ---------------------------------
/*
 * FILE:    sha2.c
 * AUTHOR:  Aaron D. Gifford - http://www.aarongifford.com/
 *
 * Copyright (c) 2000-2001, Aaron D. Gifford
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTOR(S) ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTOR(S) BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id: sha2.c,v 1.1 2001/11/08 00:01:51 adg Exp adg $
 */

#include <assert.h> /* assert() */
#include <stdlib.h>
#include <string.h> /* memcpy()/memset() or bcopy()/bzero() */


/*
 * ASSERT NOTE:
 * Some sanity checking code is included using assert().  On my FreeBSD
 * system, this additional code can be removed by compiling with NDEBUG
 * defined.  Check your own systems manpage on assert() to see how to
 * compile WITHOUT the sanity checking code on your system.
 *
 * UNROLLED TRANSFORM LOOP NOTE:
 * You can define SHA2_UNROLL_TRANSFORM to use the unrolled transform
 * loop version for the hash transform rounds (defined using macros
 * later in this file).  Either define on the command line, for example:
 *
 *   cc -DSHA2_UNROLL_TRANSFORM -o sha2 sha2.c sha2prog.c
 *
 * or define below:
 *
 *   #define SHA2_UNROLL_TRANSFORM
 *
 */

/*** SHA-256/384/512 Machine Architecture Definitions *****************/
/*
 * BYTE_ORDER NOTE:
 *
 * Please make sure that your system defines BYTE_ORDER.  If your
 * architecture is little-endian, make sure it also defines
 * LITTLE_ENDIAN and that the two (BYTE_ORDER and LITTLE_ENDIAN) are
 * equivilent.
 *
 * If your system does not define the above, then you can do so by
 * hand like this:
 *
 *   #define LITTLE_ENDIAN 1234
 *   #define BIG_ENDIAN    4321
 *
 * And for little-endian machines, add:
 *
 *   #define BYTE_ORDER LITTLE_ENDIAN
 *
 * Or for big-endian machines:
 *
 *   #define BYTE_ORDER BIG_ENDIAN
 *
 * The FreeBSD machine this was written on defines BYTE_ORDER
 * appropriately by including <sys/types.h> (which in turn includes
 * <machine/endian.h> where the appropriate definitions are actually
 * made).
 */

#ifdef __BYTE_ORDER__
#ifndef BYTE_ORDER
#define BYTE_ORDER __BYTE_ORDER__
#endif
#ifndef BIG_ENDIAN
#define BIG_ENDIAN __ORDER_BIG_ENDIAN__
#endif
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN __ORDER_LITTLE_ENDIAN__
#endif
#endif

#ifndef BYTE_ORDER
#if defined(i386) || defined(__i386__) || defined(_M_IX86) || defined(__x86_64) ||    \
    defined(__x86_64__) || defined(_M_X64) || defined(_M_AMD64) || defined(_M_ARM) || \
    defined(__x86) || defined(__arm__)
#define BYTE_ORDER 1234
#elif defined(sparc) || defined(__ppc__)
#define BYTE_ORDER 4321
#else
#define BYTE_ORDER 0
#endif
#endif

#if !defined(BYTE_ORDER) || (BYTE_ORDER != LITTLE_ENDIAN && BYTE_ORDER != BIG_ENDIAN)
#error Define BYTE_ORDER to be equal to either LITTLE_ENDIAN or BIG_ENDIAN
#endif

/*
 * Define the followingsha2_* types to types of the correct length on
 * the native archtecture.   Most BSD systems and Linux define u_intXX_t
 * types.  Machines with very recent ANSI C headers, can use the
 * uintXX_t definintions from inttypes.h by defining SHA2_USE_INTTYPES_H
 * during compile or in the sha.h header file.
 *
 * Machines that support neither u_intXX_t nor inttypes.h's uintXX_t
 * will need to define these three typedefs below (and the appropriate
 * ones in sha.h too) by hand according to their system architecture.
 *
 * Thank you, Jun-ichiro itojun Hagino, for suggesting using u_intXX_t
 * types and pointing out recent ANSI C support for uintXX_t in inttypes.h.
 */
#ifdef SHA2_USE_INTTYPES_H

typedef uint8_t sha2_byte;    /* Exactly 1 byte */
typedef uint32_t sha2_word32; /* Exactly 4 bytes */
typedef uint64_t sha2_word64; /* Exactly 8 bytes */

#else /* SHA2_USE_INTTYPES_H */

typedef u_int8_t sha2_byte;    /* Exactly 1 byte */
typedef u_int32_t sha2_word32; /* Exactly 4 bytes */
typedef u_int64_t sha2_word64; /* Exactly 8 bytes */

#endif /* SHA2_USE_INTTYPES_H */

/*** SHA-256/384/512 Various Length Definitions ***********************/
/* NOTE: Most of these are in sha2.h */
#define SHA256_SHORT_BLOCK_LENGTH (SHA256_BLOCK_LENGTH - 8)
#define SHA384_SHORT_BLOCK_LENGTH (SHA384_BLOCK_LENGTH - 16)
#define SHA512_SHORT_BLOCK_LENGTH (SHA512_BLOCK_LENGTH - 16)

/*** ENDIAN REVERSAL MACROS *******************************************/
#if BYTE_ORDER == LITTLE_ENDIAN
#define REVERSE32(w, x)                                                  \
    {                                                                    \
        sha2_word32 tmp = (w);                                           \
        tmp = (tmp >> 16) | (tmp << 16);                                 \
        (x) = ((tmp & 0xff00ff00UL) >> 8) | ((tmp & 0x00ff00ffUL) << 8); \
    }
#define REVERSE64(w, x)                                                                      \
    {                                                                                        \
        sha2_word64 tmp = (w);                                                               \
        tmp = (tmp >> 32) | (tmp << 32);                                                     \
        tmp = ((tmp & 0xff00ff00ff00ff00ULL) >> 8) | ((tmp & 0x00ff00ff00ff00ffULL) << 8);   \
        (x) = ((tmp & 0xffff0000ffff0000ULL) >> 16) | ((tmp & 0x0000ffff0000ffffULL) << 16); \
    }
#endif /* BYTE_ORDER == LITTLE_ENDIAN */

/*
 * Macro for incrementally adding the unsigned 64-bit integer n to the
 * unsigned 128-bit integer (represented using a two-element array of
 * 64-bit words):
 */
#define ADDINC128(w, n)             \
    {                               \
        (w)[0] += (sha2_word64)(n); \
        if ((w)[0] < (n)) {         \
            (w)[1]++;               \
        }                           \
    }

/*
 * Macros for copying blocks of memory and for zeroing out ranges
 * of memory.  Using these macros makes it easy to switch from
 * using memset()/memcpy() and using bzero()/bcopy().
 *
 * Please define either SHA2_USE_MEMSET_MEMCPY or define
 * SHA2_USE_BZERO_BCOPY depending on which function set you
 * choose to use:
 */
#if !defined(SHA2_USE_MEMSET_MEMCPY) && !defined(SHA2_USE_BZERO_BCOPY)
/* Default to memset()/memcpy() if no option is specified */
#define SHA2_USE_MEMSET_MEMCPY 1
#endif
#if defined(SHA2_USE_MEMSET_MEMCPY) && defined(SHA2_USE_BZERO_BCOPY)
/* Abort with an error if BOTH options are defined */
#error Define either SHA2_USE_MEMSET_MEMCPY or SHA2_USE_BZERO_BCOPY, not both!
#endif

#ifdef SHA2_USE_MEMSET_MEMCPY
#define MEMSET_BZERO(p, l) memset((p), 0, (l))
#define MEMCPY_BCOPY(d, s, l) memcpy((d), (s), (l))
#endif
#ifdef SHA2_USE_BZERO_BCOPY
#define MEMSET_BZERO(p, l) bzero((p), (l))
#define MEMCPY_BCOPY(d, s, l) bcopy((s), (d), (l))
#endif

/*** THE SIX LOGICAL FUNCTIONS ****************************************/
/*
 * Bit shifting and rotation (used by the six SHA-XYZ logical functions:
 *
 *   NOTE:  The naming of R and S appears backwards here (R is a SHIFT and
 *   S is a ROTATION) because the SHA-256/384/512 description document
 *   (see http://csrc.nist.gov/cryptval/shs/sha256-384-512.pdf) uses this
 *   same "backwards" definition.
 */
/* Shift-right (used in SHA-256, SHA-384, and SHA-512): */
#define R(b, x) ((x) >> (b))
/* 32-bit Rotate-right (used in SHA-256): */
#define S32(b, x) (((x) >> (b)) | ((x) << (32 - (b))))
/* 64-bit Rotate-right (used in SHA-384 and SHA-512): */
#define S64(b, x) (((x) >> (b)) | ((x) << (64 - (b))))

/* Two of six logical functions used in SHA-256, SHA-384, and SHA-512: */
#define Ch(x, y, z) (((x) & (y)) ^ ((~(x)) & (z)))
#define Maj(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

/* Four of six logical functions used in SHA-256: */
#define Sigma0_256(x) (S32(2, (x)) ^ S32(13, (x)) ^ S32(22, (x)))
#define Sigma1_256(x) (S32(6, (x)) ^ S32(11, (x)) ^ S32(25, (x)))
#define sigma0_256(x) (S32(7, (x)) ^ S32(18, (x)) ^ R(3, (x)))
#define sigma1_256(x) (S32(17, (x)) ^ S32(19, (x)) ^ R(10, (x)))

/* Four of six logical functions used in SHA-384 and SHA-512: */
#define Sigma0_512(x) (S64(28, (x)) ^ S64(34, (x)) ^ S64(39, (x)))
#define Sigma1_512(x) (S64(14, (x)) ^ S64(18, (x)) ^ S64(41, (x)))
#define sigma0_512(x) (S64(1, (x)) ^ S64(8, (x)) ^ R(7, (x)))
#define sigma1_512(x) (S64(19, (x)) ^ S64(61, (x)) ^ R(6, (x)))

/*** INTERNAL FUNCTION PROTOTYPES *************************************/
/* NOTE: These should not be accessed directly from outside this
 * library -- they are intended for private internal visibility/use
 * only.
 */
// void SHA512_Last(SHA512_CTX*);
// void SHA256_Transform(SHA256_CTX*, const sha2_word32*);
// void SHA512_Transform(SHA512_CTX*, const sha2_word64*);

/*** SHA-XYZ INITIAL HASH VALUES AND CONSTANTS ************************/
/* Hash constant words K for SHA-256: */
const static sha2_word32 K256[64] = {
    0x428a2f98UL, 0x71374491UL, 0xb5c0fbcfUL, 0xe9b5dba5UL, 0x3956c25bUL, 0x59f111f1UL,
    0x923f82a4UL, 0xab1c5ed5UL, 0xd807aa98UL, 0x12835b01UL, 0x243185beUL, 0x550c7dc3UL,
    0x72be5d74UL, 0x80deb1feUL, 0x9bdc06a7UL, 0xc19bf174UL, 0xe49b69c1UL, 0xefbe4786UL,
    0x0fc19dc6UL, 0x240ca1ccUL, 0x2de92c6fUL, 0x4a7484aaUL, 0x5cb0a9dcUL, 0x76f988daUL,
    0x983e5152UL, 0xa831c66dUL, 0xb00327c8UL, 0xbf597fc7UL, 0xc6e00bf3UL, 0xd5a79147UL,
    0x06ca6351UL, 0x14292967UL, 0x27b70a85UL, 0x2e1b2138UL, 0x4d2c6dfcUL, 0x53380d13UL,
    0x650a7354UL, 0x766a0abbUL, 0x81c2c92eUL, 0x92722c85UL, 0xa2bfe8a1UL, 0xa81a664bUL,
    0xc24b8b70UL, 0xc76c51a3UL, 0xd192e819UL, 0xd6990624UL, 0xf40e3585UL, 0x106aa070UL,
    0x19a4c116UL, 0x1e376c08UL, 0x2748774cUL, 0x34b0bcb5UL, 0x391c0cb3UL, 0x4ed8aa4aUL,
    0x5b9cca4fUL, 0x682e6ff3UL, 0x748f82eeUL, 0x78a5636fUL, 0x84c87814UL, 0x8cc70208UL,
    0x90befffaUL, 0xa4506cebUL, 0xbef9a3f7UL, 0xc67178f2UL};

/* Initial hash value H for SHA-256: */
const static sha2_word32 sha256_initial_hash_value[8] = {0x6a09e667UL, 0xbb67ae85UL, 0x3c6ef372UL,
                                                         0xa54ff53aUL, 0x510e527fUL, 0x9b05688cUL,
                                                         0x1f83d9abUL, 0x5be0cd19UL};

/* Hash constant words K for SHA-384 and SHA-512: */
const static sha2_word64 K512[80] = {
    0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL, 0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL,
    0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL, 0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL,
    0xd807aa98a3030242ULL, 0x12835b0145706fbeULL, 0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
    0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL, 0x9bdc06a725c71235ULL, 0xc19bf174cf692694ULL,
    0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL, 0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
    0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL, 0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
    0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL, 0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL,
    0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL, 0x06ca6351e003826fULL, 0x142929670a0e6e70ULL,
    0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL, 0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
    0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL, 0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
    0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL, 0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL,
    0xd192e819d6ef5218ULL, 0xd69906245565a910ULL, 0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
    0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL, 0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL,
    0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL, 0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL,
    0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL, 0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
    0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL, 0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL,
    0xca273eceea26619cULL, 0xd186b8c721c0c207ULL, 0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL,
    0x06f067aa72176fbaULL, 0x0a637dc5a2c898a6ULL, 0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
    0x28db77f523047d84ULL, 0x32caab7b40c72493ULL, 0x3c9ebe0a15c9bebcULL, 0x431d67c49c100d4cULL,
    0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL, 0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL};

/* Initial hash value H for SHA-384 */
const static sha2_word64 sha384_initial_hash_value[8] = {
    0xcbbb9d5dc1059ed8ULL, 0x629a292a367cd507ULL, 0x9159015a3070dd17ULL, 0x152fecd8f70e5939ULL,
    0x67332667ffc00b31ULL, 0x8eb44a8768581511ULL, 0xdb0c2e0d64f98fa7ULL, 0x47b5481dbefa4fa4ULL};

/* Initial hash value H for SHA-512 */
const static sha2_word64 sha512_initial_hash_value[8] = {
    0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL, 0x3c6ef372fe94f82bULL, 0xa54ff53a5f1d36f1ULL,
    0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL, 0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL};

/*** SHA-256: *********************************************************/
void* sha256_init() {
    SHA256_CTX* context;
    context = malloc(sizeof(SHA256_CTX));
    if (!context)
        return NULL;
    MEMCPY_BCOPY(context->state, sha256_initial_hash_value, SHA256_DIGEST_LENGTH);
    MEMSET_BZERO(context->buffer, SHA256_BLOCK_LENGTH);
    context->bitcount = 0;
    return context;
}

#ifdef SHA2_UNROLL_TRANSFORM

/* Unrolled SHA-256 round macros: */

#if BYTE_ORDER == LITTLE_ENDIAN

#define ROUND256_0_TO_15(a, b, c, d, e, f, g, h)                      \
    REVERSE32(*data++, W256[j]);                                      \
    T1 = (h) + Sigma1_256(e) + Ch((e), (f), (g)) + K256[j] + W256[j]; \
    (d) += T1;                                                        \
    (h) = T1 + Sigma0_256(a) + Maj((a), (b), (c));                    \
    j++

#else /* BYTE_ORDER == LITTLE_ENDIAN */

#define ROUND256_0_TO_15(a, b, c, d, e, f, g, h)                                  \
    T1 = (h) + Sigma1_256(e) + Ch((e), (f), (g)) + K256[j] + (W256[j] = *data++); \
    (d) += T1;                                                                    \
    (h) = T1 + Sigma0_256(a) + Maj((a), (b), (c));                                \
    j++

#endif /* BYTE_ORDER == LITTLE_ENDIAN */

#define ROUND256(a, b, c, d, e, f, g, h)                     \
    s0 = W256[(j + 1) & 0x0f];                               \
    s0 = sigma0_256(s0);                                     \
    s1 = W256[(j + 14) & 0x0f];                              \
    s1 = sigma1_256(s1);                                     \
    T1 = (h) + Sigma1_256(e) + Ch((e), (f), (g)) + K256[j] + \
         (W256[j & 0x0f] += s1 + W256[(j + 9) & 0x0f] + s0); \
    (d) += T1;                                               \
    (h) = T1 + Sigma0_256(a) + Maj((a), (b), (c));           \
    j++

static void SHA256_Transform(SHA256_CTX* context, const sha2_word32* data) {
    sha2_word32 a, b, c, d, e, f, g, h, s0, s1;
    sha2_word32 T1, *W256;
    int j;

    W256 = (sha2_word32*)context->buffer;

    /* Initialize registers with the prev. intermediate value */
    a = context->state[0];
    b = context->state[1];
    c = context->state[2];
    d = context->state[3];
    e = context->state[4];
    f = context->state[5];
    g = context->state[6];
    h = context->state[7];

    j = 0;
    do {
        /* Rounds 0 to 15 (unrolled): */
        ROUND256_0_TO_15(a, b, c, d, e, f, g, h);
        ROUND256_0_TO_15(h, a, b, c, d, e, f, g);
        ROUND256_0_TO_15(g, h, a, b, c, d, e, f);
        ROUND256_0_TO_15(f, g, h, a, b, c, d, e);
        ROUND256_0_TO_15(e, f, g, h, a, b, c, d);
        ROUND256_0_TO_15(d, e, f, g, h, a, b, c);
        ROUND256_0_TO_15(c, d, e, f, g, h, a, b);
        ROUND256_0_TO_15(b, c, d, e, f, g, h, a);
    } while (j < 16);

    /* Now for the remaining rounds to 64: */
    do {
        ROUND256(a, b, c, d, e, f, g, h);
        ROUND256(h, a, b, c, d, e, f, g);
        ROUND256(g, h, a, b, c, d, e, f);
        ROUND256(f, g, h, a, b, c, d, e);
        ROUND256(e, f, g, h, a, b, c, d);
        ROUND256(d, e, f, g, h, a, b, c);
        ROUND256(c, d, e, f, g, h, a, b);
        ROUND256(b, c, d, e, f, g, h, a);
    } while (j < 64);

    /* Compute the current intermediate hash value */
    context->state[0] += a;
    context->state[1] += b;
    context->state[2] += c;
    context->state[3] += d;
    context->state[4] += e;
    context->state[5] += f;
    context->state[6] += g;
    context->state[7] += h;

    /* Clean up */
    a = b = c = d = e = f = g = h = T1 = 0;
}

#else /* SHA2_UNROLL_TRANSFORM */

static void SHA256_Transform(SHA256_CTX* context, const sha2_word32* data) {
    sha2_word32 a, b, c, d, e, f, g, h, s0, s1;
    sha2_word32 T1, T2, *W256;
    int j;

    W256 = (sha2_word32*)context->buffer;

    /* Initialize registers with the prev. intermediate value */
    a = context->state[0];
    b = context->state[1];
    c = context->state[2];
    d = context->state[3];
    e = context->state[4];
    f = context->state[5];
    g = context->state[6];
    h = context->state[7];

    j = 0;
    do {
#if BYTE_ORDER == LITTLE_ENDIAN
        /* Copy data while converting to host byte order */
        REVERSE32(*data++, W256[j]);
        /* Apply the SHA-256 compression function to update a..h */
        T1 = h + Sigma1_256(e) + Ch(e, f, g) + K256[j] + W256[j];
#else  /* BYTE_ORDER == LITTLE_ENDIAN */
        /* Apply the SHA-256 compression function to update a..h with copy */
        T1 = h + Sigma1_256(e) + Ch(e, f, g) + K256[j] + (W256[j] = *data++);
#endif /* BYTE_ORDER == LITTLE_ENDIAN */
        T2 = Sigma0_256(a) + Maj(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + T1;
        d = c;
        c = b;
        b = a;
        a = T1 + T2;

        j++;
    } while (j < 16);

    do {
        /* Part of the message block expansion: */
        s0 = W256[(j + 1) & 0x0f];
        s0 = sigma0_256(s0);
        s1 = W256[(j + 14) & 0x0f];
        s1 = sigma1_256(s1);

        /* Apply the SHA-256 compression function to update a..h */
        T1 = h + Sigma1_256(e) + Ch(e, f, g) + K256[j] +
             (W256[j & 0x0f] += s1 + W256[(j + 9) & 0x0f] + s0);
        T2 = Sigma0_256(a) + Maj(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + T1;
        d = c;
        c = b;
        b = a;
        a = T1 + T2;

        j++;
    } while (j < 64);

    /* Compute the current intermediate hash value */
    context->state[0] += a;
    context->state[1] += b;
    context->state[2] += c;
    context->state[3] += d;
    context->state[4] += e;
    context->state[5] += f;
    context->state[6] += g;
    context->state[7] += h;

    /* Clean up */
    a = b = c = d = e = f = g = h = T1 = T2 = 0;
}

#endif /* SHA2_UNROLL_TRANSFORM */

void sha256_update(SHA256_CTX* context, const sha2_byte* data, size_t len) {
    unsigned int freespace, usedspace;

    if (len == 0) {
        /* Calling with no data is valid - we do nothing */
        return;
    }

    /* Sanity check: */
    assert(context != (SHA256_CTX*)0 && data != (sha2_byte*)0);

    usedspace = (context->bitcount >> 3) % SHA256_BLOCK_LENGTH;
    if (usedspace > 0) {
        /* Calculate how much free space is available in the buffer */
        freespace = SHA256_BLOCK_LENGTH - usedspace;

        if (len >= freespace) {
            /* Fill the buffer completely and process it */
            MEMCPY_BCOPY(&context->buffer[usedspace], data, freespace);
            context->bitcount += freespace << 3;
            len -= freespace;
            data += freespace;
            SHA256_Transform(context, (sha2_word32*)context->buffer);
        } else {
            /* The buffer is not yet full */
            MEMCPY_BCOPY(&context->buffer[usedspace], data, len);
            context->bitcount += len << 3;
            /* Clean up: */
            usedspace = freespace = 0;
            return;
        }
    }
    while (len >= SHA256_BLOCK_LENGTH) {
        /* Process as many complete blocks as we can */
        SHA256_Transform(context, (sha2_word32*)data);
        context->bitcount += SHA256_BLOCK_LENGTH << 3;
        len -= SHA256_BLOCK_LENGTH;
        data += SHA256_BLOCK_LENGTH;
    }
    if (len > 0) {
        /* There's left-overs, so save 'em */
        MEMCPY_BCOPY(context->buffer, data, len);
        context->bitcount += len << 3;
    }
    /* Clean up: */
    usedspace = freespace = 0;
}

int sha256_final(SHA256_CTX* context, sha2_byte digest[SHA256_DIGEST_LENGTH]) {
    sha2_word32* d = (sha2_word32*)digest;
    unsigned int usedspace;

    /* Sanity check: */
    assert(context != (SHA256_CTX*)0);

    /* If no digest buffer is passed, we don't bother doing this: */
    if (digest != (sha2_byte*)0) {
        usedspace = (context->bitcount >> 3) % SHA256_BLOCK_LENGTH;
#if BYTE_ORDER == LITTLE_ENDIAN
        /* Convert FROM host byte order */
        REVERSE64(context->bitcount, context->bitcount);
#endif
        if (usedspace > 0) {
            /* Begin padding with a 1 bit: */
            context->buffer[usedspace++] = 0x80;

            if (usedspace <= SHA256_SHORT_BLOCK_LENGTH) {
                /* Set-up for the last transform: */
                MEMSET_BZERO(&context->buffer[usedspace], SHA256_SHORT_BLOCK_LENGTH - usedspace);
            } else {
                if (usedspace < SHA256_BLOCK_LENGTH) {
                    MEMSET_BZERO(&context->buffer[usedspace], SHA256_BLOCK_LENGTH - usedspace);
                }
                /* Do second-to-last transform: */
                SHA256_Transform(context, (sha2_word32*)context->buffer);

                /* And set-up for the last transform: */
                MEMSET_BZERO(context->buffer, SHA256_SHORT_BLOCK_LENGTH);
            }
        } else {
            /* Set-up for the last transform: */
            MEMSET_BZERO(context->buffer, SHA256_SHORT_BLOCK_LENGTH);

            /* Begin padding with a 1 bit: */
            *context->buffer = 0x80;
        }
        /* Set the bit count: */
        *(sha2_word64*)&context->buffer[SHA256_SHORT_BLOCK_LENGTH] = context->bitcount;

        /* Final transform: */
        SHA256_Transform(context, (sha2_word32*)context->buffer);

#if BYTE_ORDER == LITTLE_ENDIAN
        {
            /* Convert TO host byte order */
            int j;
            for (j = 0; j < 8; j++) {
                REVERSE32(context->state[j], context->state[j]);
                *d++ = context->state[j];
            }
        }
#else
        MEMCPY_BCOPY(d, context->state, SHA256_DIGEST_LENGTH);
#endif
    }

    /* Clean up state data: */
    free(context);
    usedspace = 0;
    return SHA256_DIGEST_LENGTH;
}

/*** SHA-512: *********************************************************/
void* sha512_init() {
    SHA512_CTX* context;
    context = malloc(sizeof(SHA512_CTX));
    if (!context)
        return NULL;
    MEMCPY_BCOPY(context->state, sha512_initial_hash_value, SHA512_DIGEST_LENGTH);
    MEMSET_BZERO(context->buffer, SHA512_BLOCK_LENGTH);
    context->bitcount[0] = context->bitcount[1] = 0;
    return context;
}

#ifdef SHA2_UNROLL_TRANSFORM

/* Unrolled SHA-512 round macros: */
#if BYTE_ORDER == LITTLE_ENDIAN

#define ROUND512_0_TO_15(a, b, c, d, e, f, g, h)                      \
    REVERSE64(*data++, W512[j]);                                      \
    T1 = (h) + Sigma1_512(e) + Ch((e), (f), (g)) + K512[j] + W512[j]; \
    (d) += T1, (h) = T1 + Sigma0_512(a) + Maj((a), (b), (c)), j++

#else /* BYTE_ORDER == LITTLE_ENDIAN */

#define ROUND512_0_TO_15(a, b, c, d, e, f, g, h)                                  \
    T1 = (h) + Sigma1_512(e) + Ch((e), (f), (g)) + K512[j] + (W512[j] = *data++); \
    (d) += T1;                                                                    \
    (h) = T1 + Sigma0_512(a) + Maj((a), (b), (c));                                \
    j++

#endif /* BYTE_ORDER == LITTLE_ENDIAN */

#define ROUND512(a, b, c, d, e, f, g, h)                     \
    s0 = W512[(j + 1) & 0x0f];                               \
    s0 = sigma0_512(s0);                                     \
    s1 = W512[(j + 14) & 0x0f];                              \
    s1 = sigma1_512(s1);                                     \
    T1 = (h) + Sigma1_512(e) + Ch((e), (f), (g)) + K512[j] + \
         (W512[j & 0x0f] += s1 + W512[(j + 9) & 0x0f] + s0); \
    (d) += T1;                                               \
    (h) = T1 + Sigma0_512(a) + Maj((a), (b), (c));           \
    j++

static void SHA512_Transform(SHA512_CTX* context, const sha2_word64* data) {
    sha2_word64 a, b, c, d, e, f, g, h, s0, s1;
    sha2_word64 T1, *W512 = (sha2_word64*)context->buffer;
    int j;

    /* Initialize registers with the prev. intermediate value */
    a = context->state[0];
    b = context->state[1];
    c = context->state[2];
    d = context->state[3];
    e = context->state[4];
    f = context->state[5];
    g = context->state[6];
    h = context->state[7];

    j = 0;
    do {
        ROUND512_0_TO_15(a, b, c, d, e, f, g, h);
        ROUND512_0_TO_15(h, a, b, c, d, e, f, g);
        ROUND512_0_TO_15(g, h, a, b, c, d, e, f);
        ROUND512_0_TO_15(f, g, h, a, b, c, d, e);
        ROUND512_0_TO_15(e, f, g, h, a, b, c, d);
        ROUND512_0_TO_15(d, e, f, g, h, a, b, c);
        ROUND512_0_TO_15(c, d, e, f, g, h, a, b);
        ROUND512_0_TO_15(b, c, d, e, f, g, h, a);
    } while (j < 16);

    /* Now for the remaining rounds up to 79: */
    do {
        ROUND512(a, b, c, d, e, f, g, h);
        ROUND512(h, a, b, c, d, e, f, g);
        ROUND512(g, h, a, b, c, d, e, f);
        ROUND512(f, g, h, a, b, c, d, e);
        ROUND512(e, f, g, h, a, b, c, d);
        ROUND512(d, e, f, g, h, a, b, c);
        ROUND512(c, d, e, f, g, h, a, b);
        ROUND512(b, c, d, e, f, g, h, a);
    } while (j < 80);

    /* Compute the current intermediate hash value */
    context->state[0] += a;
    context->state[1] += b;
    context->state[2] += c;
    context->state[3] += d;
    context->state[4] += e;
    context->state[5] += f;
    context->state[6] += g;
    context->state[7] += h;

    /* Clean up */
    a = b = c = d = e = f = g = h = T1 = 0;
}

#else /* SHA2_UNROLL_TRANSFORM */

static void SHA512_Transform(SHA512_CTX* context, const sha2_word64* data) {
    sha2_word64 a, b, c, d, e, f, g, h, s0, s1;
    sha2_word64 T1, T2, *W512 = (sha2_word64*)context->buffer;
    int j;

    /* Initialize registers with the prev. intermediate value */
    a = context->state[0];
    b = context->state[1];
    c = context->state[2];
    d = context->state[3];
    e = context->state[4];
    f = context->state[5];
    g = context->state[6];
    h = context->state[7];

    j = 0;
    do {
#if BYTE_ORDER == LITTLE_ENDIAN
        /* Convert TO host byte order */
        REVERSE64(*data++, W512[j]);
        /* Apply the SHA-512 compression function to update a..h */
        T1 = h + Sigma1_512(e) + Ch(e, f, g) + K512[j] + W512[j];
#else  /* BYTE_ORDER == LITTLE_ENDIAN */
        /* Apply the SHA-512 compression function to update a..h with copy */
        T1 = h + Sigma1_512(e) + Ch(e, f, g) + K512[j] + (W512[j] = *data++);
#endif /* BYTE_ORDER == LITTLE_ENDIAN */
        T2 = Sigma0_512(a) + Maj(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + T1;
        d = c;
        c = b;
        b = a;
        a = T1 + T2;

        j++;
    } while (j < 16);

    do {
        /* Part of the message block expansion: */
        s0 = W512[(j + 1) & 0x0f];
        s0 = sigma0_512(s0);
        s1 = W512[(j + 14) & 0x0f];
        s1 = sigma1_512(s1);

        /* Apply the SHA-512 compression function to update a..h */
        T1 = h + Sigma1_512(e) + Ch(e, f, g) + K512[j] +
             (W512[j & 0x0f] += s1 + W512[(j + 9) & 0x0f] + s0);
        T2 = Sigma0_512(a) + Maj(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + T1;
        d = c;
        c = b;
        b = a;
        a = T1 + T2;

        j++;
    } while (j < 80);

    /* Compute the current intermediate hash value */
    context->state[0] += a;
    context->state[1] += b;
    context->state[2] += c;
    context->state[3] += d;
    context->state[4] += e;
    context->state[5] += f;
    context->state[6] += g;
    context->state[7] += h;

    /* Clean up */
    a = b = c = d = e = f = g = h = T1 = T2 = 0;
}

#endif /* SHA2_UNROLL_TRANSFORM */

void sha512_update(SHA512_CTX* context, const sha2_byte* data, size_t len) {
    unsigned int freespace, usedspace;

    if (len == 0) {
        /* Calling with no data is valid - we do nothing */
        return;
    }

    /* Sanity check: */
    assert(context != (SHA512_CTX*)0 && data != (sha2_byte*)0);

    usedspace = (context->bitcount[0] >> 3) % SHA512_BLOCK_LENGTH;
    if (usedspace > 0) {
        /* Calculate how much free space is available in the buffer */
        freespace = SHA512_BLOCK_LENGTH - usedspace;

        if (len >= freespace) {
            /* Fill the buffer completely and process it */
            MEMCPY_BCOPY(&context->buffer[usedspace], data, freespace);
            ADDINC128(context->bitcount, freespace << 3);
            len -= freespace;
            data += freespace;
            SHA512_Transform(context, (sha2_word64*)context->buffer);
        } else {
            /* The buffer is not yet full */
            MEMCPY_BCOPY(&context->buffer[usedspace], data, len);
            ADDINC128(context->bitcount, len << 3);
            /* Clean up: */
            usedspace = freespace = 0;
            return;
        }
    }
    while (len >= SHA512_BLOCK_LENGTH) {
        /* Process as many complete blocks as we can */
        SHA512_Transform(context, (sha2_word64*)data);
        ADDINC128(context->bitcount, SHA512_BLOCK_LENGTH << 3);
        len -= SHA512_BLOCK_LENGTH;
        data += SHA512_BLOCK_LENGTH;
    }
    if (len > 0) {
        /* There's left-overs, so save 'em */
        MEMCPY_BCOPY(context->buffer, data, len);
        ADDINC128(context->bitcount, len << 3);
    }
    /* Clean up: */
    usedspace = freespace = 0;
}

static void SHA512_Last(SHA512_CTX* context) {
    unsigned int usedspace;

    usedspace = (context->bitcount[0] >> 3) % SHA512_BLOCK_LENGTH;
#if BYTE_ORDER == LITTLE_ENDIAN
    /* Convert FROM host byte order */
    REVERSE64(context->bitcount[0], context->bitcount[0]);
    REVERSE64(context->bitcount[1], context->bitcount[1]);
#endif
    if (usedspace > 0) {
        /* Begin padding with a 1 bit: */
        context->buffer[usedspace++] = 0x80;

        if (usedspace <= SHA512_SHORT_BLOCK_LENGTH) {
            /* Set-up for the last transform: */
            MEMSET_BZERO(&context->buffer[usedspace], SHA512_SHORT_BLOCK_LENGTH - usedspace);
        } else {
            if (usedspace < SHA512_BLOCK_LENGTH) {
                MEMSET_BZERO(&context->buffer[usedspace], SHA512_BLOCK_LENGTH - usedspace);
            }
            /* Do second-to-last transform: */
            SHA512_Transform(context, (sha2_word64*)context->buffer);

            /* And set-up for the last transform: */
            MEMSET_BZERO(context->buffer, SHA512_BLOCK_LENGTH - 2);
        }
    } else {
        /* Prepare for final transform: */
        MEMSET_BZERO(context->buffer, SHA512_SHORT_BLOCK_LENGTH);

        /* Begin padding with a 1 bit: */
        *context->buffer = 0x80;
    }
    /* Store the length of input data (in bits): */
    *(sha2_word64*)&context->buffer[SHA512_SHORT_BLOCK_LENGTH] = context->bitcount[1];
    *(sha2_word64*)&context->buffer[SHA512_SHORT_BLOCK_LENGTH + 8] = context->bitcount[0];

    /* Final transform: */
    SHA512_Transform(context, (sha2_word64*)context->buffer);
}

int sha512_final(SHA512_CTX* context, sha2_byte digest[SHA512_DIGEST_LENGTH]) {
    sha2_word64* d = (sha2_word64*)digest;

    /* Sanity check: */
    assert(context != (SHA512_CTX*)0);

    /* If no digest buffer is passed, we don't bother doing this: */
    if (digest != (sha2_byte*)0) {
        SHA512_Last(context);

        /* Save the hash data for output: */
#if BYTE_ORDER == LITTLE_ENDIAN
        {
            /* Convert TO host byte order */
            int j;
            for (j = 0; j < 8; j++) {
                REVERSE64(context->state[j], context->state[j]);
                *d++ = context->state[j];
            }
        }
#else
        MEMCPY_BCOPY(d, context->state, SHA512_DIGEST_LENGTH);
#endif
    }

    /* Zero out state data */
    free(context);
    return SHA512_DIGEST_LENGTH;
}

/*** SHA-384: *********************************************************/
void* sha384_init() {
    SHA384_CTX* context;
    context = malloc(sizeof(SHA384_CTX));
    if (!context)
        return NULL;
    MEMCPY_BCOPY(context->state, sha384_initial_hash_value, SHA512_DIGEST_LENGTH);
    MEMSET_BZERO(context->buffer, SHA384_BLOCK_LENGTH);
    context->bitcount[0] = context->bitcount[1] = 0;
    return context;
}

void sha384_update(SHA384_CTX* context, const sha2_byte* data, size_t len) {
    sha512_update((SHA512_CTX*)context, data, len);
}

int sha384_final(SHA384_CTX* context, sha2_byte digest[SHA384_DIGEST_LENGTH]) {
    sha2_word64* d = (sha2_word64*)digest;

    /* Sanity check: */
    assert(context != (SHA384_CTX*)0);

    /* If no digest buffer is passed, we don't bother doing this: */
    if (digest != (sha2_byte*)0) {
        SHA512_Last((SHA512_CTX*)context);

        /* Save the hash data for output: */
#if BYTE_ORDER == LITTLE_ENDIAN
        {
            /* Convert TO host byte order */
            int j;
            for (j = 0; j < 6; j++) {
                REVERSE64(context->state[j], context->state[j]);
                *d++ = context->state[j];
            }
        }
#else
        MEMCPY_BCOPY(d, context->state, SHA384_DIGEST_LENGTH);
#endif
    }

    /* Zero out state data */
    free(context);
    return SHA384_DIGEST_LENGTH;
}

// ---------------------------------
// src/crypto/url.c
// ---------------------------------
// Originally by Fränz Friederes, MIT License
// https://github.com/cryptii/cryptii/blob/main/src/Encoder/URL.js

// Modified by Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean/

// URL-escape encoding/decoding

#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

const char* url_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.~";

uint8_t hex_to_ascii(char c) {
    if (isdigit(c)) {
        return c - '0';
    } else {
        return tolower(c) - 'a' + 10;
    }
}

uint8_t* url_encode(const uint8_t* src, size_t len, size_t* out_len) {
    size_t encoded_len = 0;
    for (size_t i = 0; i < len; i++) {
        if (strchr(url_chars, src[i]) == NULL) {
            encoded_len += 3;
        } else {
            encoded_len += 1;
        }
    }

    uint8_t* encoded = malloc(encoded_len + 1);
    if (encoded == NULL) {
        *out_len = 0;
        return NULL;
    }

    size_t pos = 0;
    for (size_t i = 0; i < len; i++) {
        if (strchr(url_chars, src[i]) == NULL) {
            encoded[pos++] = '%';
            encoded[pos++] = "0123456789ABCDEF"[src[i] >> 4];
            encoded[pos++] = "0123456789ABCDEF"[src[i] & 0x0F];
        } else {
            encoded[pos++] = src[i];
        }
    }
    encoded[pos] = '\0';

    *out_len = pos;
    return encoded;
}

uint8_t* url_decode(const uint8_t* src, size_t len, size_t* out_len) {
    uint8_t* decoded = malloc(len);
    if (decoded == NULL) {
        *out_len = 0;
        return NULL;
    }

    size_t pos = 0;
    for (size_t i = 0; i < len; i++) {
        if (src[i] == '%') {
            if (i + 2 >= len || !isxdigit(src[i + 1]) || !isxdigit(src[i + 2])) {
                free(decoded);
                return NULL;
            }
            decoded[pos++] = (hex_to_ascii(src[i + 1]) << 4) | hex_to_ascii(src[i + 2]);
            i += 2;
        } else if (src[i] == '+') {
            decoded[pos++] = ' ';
        } else {
            decoded[pos++] = src[i];
        }
    }

    *out_len = pos;
    return decoded;
}

#endif // SQLEAN_ENABLE_CRYPTO
#ifdef SQLEAN_ENABLE_FILEIO
// ---------------------------------
// src/fileio/extension.c
// ---------------------------------
// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Read and write files in SQLite.

SQLITE_EXTENSION_INIT3


int fileio_init(sqlite3* db) {
    fileio_scalar_init(db);
    fileio_ls_init(db);
    fileio_scan_init(db);
    return SQLITE_OK;
}

// ---------------------------------
// src/fileio/legacy.c
// ---------------------------------
// Originally by D. Richard Hipp, Public Domain
// https://www.sqlite.org/src/file/ext/misc/fileio.c

// Modified by Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean/

/*
 * This SQLite extension implements SQL functions
 * for reading, writing and listing files and folders.
 *
 *
 * Notes on building the extension for Windows:
 * Unless linked statically with the SQLite library, a preprocessor
 * symbol, FILEIO_WIN32_DLL, must be #define'd to create a stand-alone
 * DLL form of this extension for WIN32. See its use below for details.
 */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#if !defined(_WIN32) && !defined(WIN32)
#include <dirent.h>
#include <sys/time.h>
#include <unistd.h>
#include <utime.h>

#else

#if !defined(_MSC_VER)
#define _MSC_VER 1929
#endif
#define FILEIO_WIN32_DLL
#include <direct.h>
#include <io.h>
#define dirent DIRENT

#ifndef chmod
#define chmod _chmod
#endif

#ifndef stat
#define stat _stat
#endif
#define mkdir(path, mode) _mkdir(path)
#define lstat(path, buf) stat(path, buf)

#endif

#include <errno.h>
#include <time.h>

SQLITE_EXTENSION_INIT3

/*
** Structure of the fsdir() table-valued function
*/
/*    0    1    2     3    4           5             */
#define FSDIR_SCHEMA "(name,mode,mtime,size,path HIDDEN,dir HIDDEN)"
#define FSDIR_COLUMN_NAME 0  /* Name of the file */
#define FSDIR_COLUMN_MODE 1  /* Access mode */
#define FSDIR_COLUMN_MTIME 2 /* Last modification time */
#define FSDIR_COLUMN_SIZE 3  /* File size */
#define FSDIR_COLUMN_PATH 4  /* Path to top of search */
#define FSDIR_COLUMN_REC 5   /* Recursive flag */

/*
** Set the result stored by context ctx to a blob containing the
** contents of file zName.  Or, leave the result unchanged (NULL)
** if the file does not exist or is unreadable.
**
** If the file exceeds the SQLite blob size limit, through an
** SQLITE_TOOBIG error.
**
** Throw an SQLITE_IOERR if there are difficulties pulling the file
** off of disk.
*/
static void readFileContents(sqlite3_context* ctx,
                             const char* zName,
                             const int nOffset,
                             const int nLimit) {
    FILE* in;
    sqlite3_int64 nIn;
    void* pBuf;
    sqlite3* db;
    int mxBlob;

    assert(nOffset >= 0);
    assert(nLimit >= 0);

    in = fopen(zName, "rb");
    if (in == 0) {
        /* File does not exist or is unreadable. Leave the result set to NULL. */
        return;
    }
    fseek(in, 0, SEEK_END);
    nIn = ftell(in);
    rewind(in);

    if (nOffset > nIn) {
        /* offset is greater than the size of the file */
        sqlite3_result_zeroblob(ctx, 0);
        fclose(in);
        return;
    }
    if (nOffset > 0) {
        fseek(in, nOffset, SEEK_SET);
        nIn -= nOffset;
    }

    if (nLimit > 0 && nLimit < nIn) {
        nIn = nLimit;
    }

    db = sqlite3_context_db_handle(ctx);
    mxBlob = sqlite3_limit(db, SQLITE_LIMIT_LENGTH, -1);
    if (nIn > mxBlob) {
        sqlite3_result_error_code(ctx, SQLITE_TOOBIG);
        fclose(in);
        return;
    }
    pBuf = sqlite3_malloc64(nIn ? nIn : 1);
    if (pBuf == 0) {
        sqlite3_result_error_nomem(ctx);
        fclose(in);
        return;
    }
    if (nIn == (sqlite3_int64)fread(pBuf, 1, (size_t)nIn, in)) {
        sqlite3_result_blob64(ctx, pBuf, nIn, sqlite3_free);
    } else {
        sqlite3_result_error_code(ctx, SQLITE_IOERR);
        sqlite3_free(pBuf);
    }
    fclose(in);
}

/*
** Implementation of the "readfile(X)" SQL function.  The entire content
** of the file named X is read and returned as a BLOB.  NULL is returned
** if the file does not exist or is unreadable.
*/
static void fileio_readfile(sqlite3_context* context, int argc, sqlite3_value** argv) {
    const char* zName = (const char*)sqlite3_value_text(argv[0]);
    if (zName == 0) {
        return;
    }

    int nOffset = 0;
    if (argc >= 2 && sqlite3_value_type(argv[1]) != SQLITE_NULL) {
        nOffset = sqlite3_value_int(argv[1]);
        if (nOffset < 0) {
            sqlite3_result_error(context, "offset must be >= 0", -1);
            return;
        }
    }

    int nLimit = 0;
    if (argc == 3 && sqlite3_value_type(argv[2]) != SQLITE_NULL) {
        nLimit = sqlite3_value_int(argv[2]);
        if (nLimit < 0) {
            sqlite3_result_error(context, "limit must be >= 0", -1);
            return;
        }
    }

    readFileContents(context, zName, nOffset, nLimit);
}

/*
** Set the error message contained in context ctx to the results of
** vprintf(zFmt, ...).
*/
static void ctxErrorMsg(sqlite3_context* ctx, const char* zFmt, ...) {
    char* zMsg = 0;
    va_list ap;
    va_start(ap, zFmt);
    zMsg = sqlite3_vmprintf(zFmt, ap);
    sqlite3_result_error(ctx, zMsg, -1);
    sqlite3_free(zMsg);
    va_end(ap);
}

#if defined(_WIN32)
/*
** This function is designed to convert a Win32 FILETIME structure into the
** number of seconds since the Unix Epoch (1970-01-01 00:00:00 UTC).
*/
static sqlite3_uint64 fileTimeToUnixTime(LPFILETIME pFileTime) {
    SYSTEMTIME epochSystemTime;
    ULARGE_INTEGER epochIntervals;
    FILETIME epochFileTime;
    ULARGE_INTEGER fileIntervals;

    memset(&epochSystemTime, 0, sizeof(SYSTEMTIME));
    epochSystemTime.wYear = 1970;
    epochSystemTime.wMonth = 1;
    epochSystemTime.wDay = 1;
    SystemTimeToFileTime(&epochSystemTime, &epochFileTime);
    epochIntervals.LowPart = epochFileTime.dwLowDateTime;
    epochIntervals.HighPart = epochFileTime.dwHighDateTime;

    fileIntervals.LowPart = pFileTime->dwLowDateTime;
    fileIntervals.HighPart = pFileTime->dwHighDateTime;

    return (fileIntervals.QuadPart - epochIntervals.QuadPart) / 10000000;
}

#if defined(FILEIO_WIN32_DLL) && (defined(_WIN32) || defined(WIN32))
#/* To allow a standalone DLL, use this next replacement function: */
#undef sqlite3_win32_utf8_to_unicode
#define sqlite3_win32_utf8_to_unicode utf8_to_utf16
#
LPWSTR utf8_to_utf16(const char* z) {
    int nAllot = MultiByteToWideChar(CP_UTF8, 0, z, -1, NULL, 0);
    LPWSTR rv = sqlite3_malloc(nAllot * sizeof(WCHAR));
    if (rv != 0 && 0 < MultiByteToWideChar(CP_UTF8, 0, z, -1, rv, nAllot))
        return rv;
    sqlite3_free(rv);
    return 0;
}
#endif

/*
** This function attempts to normalize the time values found in the stat()
** buffer to UTC.  This is necessary on Win32, where the runtime library
** appears to return these values as local times.
*/
static void statTimesToUtc(const char* zPath, struct stat* pStatBuf) {
    HANDLE hFindFile;
    WIN32_FIND_DATAW fd;
    LPWSTR zUnicodeName;
    extern LPWSTR sqlite3_win32_utf8_to_unicode(const char*);
    zUnicodeName = sqlite3_win32_utf8_to_unicode(zPath);
    if (zUnicodeName) {
        memset(&fd, 0, sizeof(WIN32_FIND_DATAW));
        hFindFile = FindFirstFileW(zUnicodeName, &fd);
        if (hFindFile != NULL) {
            pStatBuf->st_ctime = (time_t)fileTimeToUnixTime(&fd.ftCreationTime);
            pStatBuf->st_atime = (time_t)fileTimeToUnixTime(&fd.ftLastAccessTime);
            pStatBuf->st_mtime = (time_t)fileTimeToUnixTime(&fd.ftLastWriteTime);
            FindClose(hFindFile);
        }
        sqlite3_free(zUnicodeName);
    }
}
#endif

/*
** This function is used in place of stat().  On Windows, special handling
** is required in order for the included time to be returned as UTC.  On all
** other systems, this function simply calls stat().
*/
static int fileStat(const char* zPath, struct stat* pStatBuf) {
#if defined(_WIN32)
    int rc = stat(zPath, pStatBuf);
    if (rc == 0)
        statTimesToUtc(zPath, pStatBuf);
    return rc;
#else
    return stat(zPath, pStatBuf);
#endif
}

/*
** This function is used in place of lstat().  On Windows, special handling
** is required in order for the included time to be returned as UTC.  On all
** other systems, this function simply calls lstat().
*/
static int fileLinkStat(const char* zPath, struct stat* pStatBuf) {
#if defined(_WIN32)
    int rc = lstat(zPath, pStatBuf);
    if (rc == 0)
        statTimesToUtc(zPath, pStatBuf);
    return rc;
#else
    return lstat(zPath, pStatBuf);
#endif
}

/*
** Argument zFile is the name of a file that will be created and/or written
** by SQL function writefile(). This function ensures that the directory
** zFile will be written to exists, creating it if required. The permissions
** for any path components created by this function are set in accordance
** with the current umask.
**
** If an OOM condition is encountered, SQLITE_NOMEM is returned. Otherwise,
** SQLITE_OK is returned if the directory is successfully created, or
** SQLITE_ERROR otherwise.
*/
static int makeParentDirectory(const char* zFile) {
    char* zCopy = sqlite3_mprintf("%s", zFile);
    int rc = SQLITE_OK;

    if (zCopy == 0) {
        rc = SQLITE_NOMEM;
    } else {
        int nCopy = (int)strlen(zCopy);
        int i = 1;

        while (rc == SQLITE_OK) {
            struct stat sStat;
            int rc2;

            for (; zCopy[i] != '/' && i < nCopy; i++)
                ;
            if (i == nCopy)
                break;
            zCopy[i] = '\0';

            rc2 = fileStat(zCopy, &sStat);
            if (rc2 != 0) {
                if (mkdir(zCopy, 0777))
                    rc = SQLITE_ERROR;
            } else {
                if (!S_ISDIR(sStat.st_mode))
                    rc = SQLITE_ERROR;
            }
            zCopy[i] = '/';
            i++;
        }

        sqlite3_free(zCopy);
    }

    return rc;
}

/*
 * Creates a directory named `path` with permission bits `mode`.
 */
static int makeDirectory(sqlite3_context* ctx, const char* path, mode_t mode) {
    int res = mkdir(path, mode);
    if (res != 0) {
        /* The mkdir() call to create the directory failed. This might not
        ** be an error though - if there is already a directory at the same
        ** path and either the permissions already match or can be changed
        ** to do so using chmod(), it is not an error.  */
        struct stat sStat;
        if (errno != EEXIST || 0 != fileStat(path, &sStat) || !S_ISDIR(sStat.st_mode) ||
            ((sStat.st_mode & 0777) != (mode & 0777) && 0 != chmod(path, mode & 0777))) {
            return 1;
        }
    }
    return 0;
}

/*
 * Creates a symbolic link named `dst`, pointing to `src`.
 */
static int createSymlink(sqlite3_context* ctx, const char* src, const char* dst) {
#ifdef _WIN32
    return 0;
#else
    int res = symlink(src, dst) < 0;
    if (res < 0) {
        return 1;
    }
    return 0;
#endif
}

/*
 * Writes blob `pData` to a file specified by `zFile`,
 * with permission bits `mode` and modification time `mtime` (-1 to not set).
 * Returns the number of written bytes.
 */
static int writeFile(sqlite3_context* pCtx,
                     const char* zFile,
                     sqlite3_value* pData,
                     mode_t mode,
                     sqlite3_int64 mtime) {
    sqlite3_int64 nWrite = 0;
    const char* z;
    int rc = 0;
    FILE* out = fopen(zFile, "wb");
    if (out == 0)
        return 1;
    z = (const char*)sqlite3_value_blob(pData);
    if (z) {
        sqlite3_int64 n = fwrite(z, 1, sqlite3_value_bytes(pData), out);
        nWrite = sqlite3_value_bytes(pData);
        if (nWrite != n) {
            rc = 1;
        }
    }
    fclose(out);
    if (rc == 0 && mode && chmod(zFile, mode)) {
        rc = 1;
    }
    if (rc)
        return 2;
    sqlite3_result_int64(pCtx, nWrite);

    if (mtime >= 0) {
#if defined(_WIN32)
#if !SQLITE_OS_WINRT
        /* Windows */
        FILETIME lastAccess;
        FILETIME lastWrite;
        SYSTEMTIME currentTime;
        LONGLONG intervals;
        HANDLE hFile;
        LPWSTR zUnicodeName;
        extern LPWSTR sqlite3_win32_utf8_to_unicode(const char*);

        GetSystemTime(&currentTime);
        SystemTimeToFileTime(&currentTime, &lastAccess);
        intervals = Int32x32To64(mtime, 10000000) + 116444736000000000;
        lastWrite.dwLowDateTime = (DWORD)intervals;
        lastWrite.dwHighDateTime = intervals >> 32;
        zUnicodeName = sqlite3_win32_utf8_to_unicode(zFile);
        if (zUnicodeName == 0) {
            return 1;
        }
        hFile = CreateFileW(zUnicodeName, FILE_WRITE_ATTRIBUTES, 0, NULL, OPEN_EXISTING,
                            FILE_FLAG_BACKUP_SEMANTICS, NULL);
        sqlite3_free(zUnicodeName);
        if (hFile != INVALID_HANDLE_VALUE) {
            BOOL bResult = SetFileTime(hFile, NULL, &lastAccess, &lastWrite);
            CloseHandle(hFile);
            return !bResult;
        } else {
            return 1;
        }
#endif
#elif defined(AT_FDCWD) && 0 /* utimensat() is not universally available */
        /* Recent unix */
        struct timespec times[2];
        times[0].tv_nsec = times[1].tv_nsec = 0;
        times[0].tv_sec = time(0);
        times[1].tv_sec = mtime;
        if (utimensat(AT_FDCWD, zFile, times, AT_SYMLINK_NOFOLLOW)) {
            return 1;
        }
#else
        /* Legacy unix */
        struct timeval times[2];
        times[0].tv_usec = times[1].tv_usec = 0;
        times[0].tv_sec = time(0);
        times[1].tv_sec = mtime;
        if (utimes(zFile, times)) {
            return 1;
        }
#endif
    }

    return 0;
}

// Writes data to a file.
// writefile(path, data[, perm[, mtime]])
static void fileio_writefile(sqlite3_context* context, int argc, sqlite3_value** argv) {
    sqlite3_int64 mtime = -1;

    if (argc < 2 || argc > 4) {
        sqlite3_result_error(context, "wrong number of arguments to function writefile()", -1);
        return;
    }

    const char* zFile = (const char*)sqlite3_value_text(argv[0]);
    if (zFile == 0) {
        return;
    }

    mode_t perm = 0666;
    if (argc >= 3) {
        perm = (mode_t)sqlite3_value_int(argv[2]);
    }

    if (argc == 4) {
        mtime = sqlite3_value_int64(argv[3]);
    }

    int res = writeFile(context, zFile, argv[1], perm, mtime);
    if (res == 1 && errno == ENOENT) {
        if (makeParentDirectory(zFile) == SQLITE_OK) {
            res = writeFile(context, zFile, argv[1], perm, mtime);
        }
    }

    if (argc > 2 && res != 0) {
        ctxErrorMsg(context, "failed to write file: %s", zFile);
    }
}

// Appends string to a file specified by path.
// fileio_append(path, str)
static void fileio_append(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    bool is_new_file = false;
    FILE* file = sqlite3_get_auxdata(ctx, 0);
    if (file == NULL) {
        const char* path = (const char*)sqlite3_value_text(argv[0]);
        file = fopen(path, "a");
        if (file == NULL && errno == ENOENT) {
            // parent directory does not exist, let's create it
            if (makeParentDirectory(path) == SQLITE_OK) {
                file = fopen(path, "a");
            }
        }
        if (file == NULL) {
            sqlite3_result_error(ctx, "failed to open file", -1);
            return;
        }
        is_new_file = true;
    }

    const char* str = (const char*)sqlite3_value_text(argv[1]);
    int rc = fputs(str, file);
    if (rc < 0) {
        if (is_new_file) {
            fclose(file);
        }
        sqlite3_result_error(ctx, "failed to append string to file", -1);
        return;
    }

    size_t n = strlen(str);
    sqlite3_result_int(ctx, n);

    if (is_new_file) {
        sqlite3_set_auxdata(ctx, 0, file, (void (*)(void*))fclose);
    }
}

// Creates a symlink.
// symlink(src, dst)
static void fileio_symlink(sqlite3_context* context, int argc, sqlite3_value** argv) {
    if (argc != 2) {
        sqlite3_result_error(context, "wrong number of arguments to function symlink()", -1);
        return;
    }

    const char* src = (const char*)sqlite3_value_text(argv[0]);
    if (src == 0) {
        return;
    }
    const char* dst = (const char*)sqlite3_value_text(argv[1]);

    int res = createSymlink(context, src, dst);
    if (res != 0) {
        ctxErrorMsg(context, "failed to create symlink to: %s", src);
    }
}

// Creates a directory.
// mkdir(path, perm)
static void fileio_mkdir(sqlite3_context* context, int argc, sqlite3_value** argv) {
    if (argc != 1 && argc != 2) {
        sqlite3_result_error(context, "wrong number of arguments to function mkdir()", -1);
        return;
    }

    const char* path = (const char*)sqlite3_value_text(argv[0]);
    if (path == 0) {
        return;
    }

    mode_t perm = 0777;
    if (argc == 2) {
        perm = (mode_t)sqlite3_value_int(argv[1]);
    }

    int res = makeDirectory(context, path, perm);

    if (res != 0) {
        ctxErrorMsg(context, "failed to create directory: %s", path);
    }
}

// Given a numberic st_mode from stat(), convert it into a human-readable
// text string in the style of "ls -l".
// lsmode(mode)
static void fileio_lsmode(sqlite3_context* context, int argc, sqlite3_value** argv) {
    int i;
    int iMode = sqlite3_value_int(argv[0]);
    char z[16];
    (void)argc;
    if (S_ISLNK(iMode)) {
        z[0] = 'l';
    } else if (S_ISREG(iMode)) {
        z[0] = '-';
    } else if (S_ISDIR(iMode)) {
        z[0] = 'd';
    } else {
        z[0] = '?';
    }
    for (i = 0; i < 3; i++) {
        int m = (iMode >> ((2 - i) * 3));
        char* a = &z[1 + i * 3];
        a[0] = (m & 0x4) ? 'r' : '-';
        a[1] = (m & 0x2) ? 'w' : '-';
        a[2] = (m & 0x1) ? 'x' : '-';
    }
    z[10] = '\0';
    sqlite3_result_text(context, z, -1, SQLITE_TRANSIENT);
}

/*
** Cursor type for recursively iterating through a directory structure.
*/
typedef struct fsdir_cursor fsdir_cursor;
typedef struct FsdirLevel FsdirLevel;

struct FsdirLevel {
    DIR* pDir;  /* From opendir() */
    char* zDir; /* Name of directory (nul-terminated) */
};

struct fsdir_cursor {
    sqlite3_vtab_cursor base; /* Base class - must be first */

    bool recursive; /* true to traverse dirs recursively, false otherwise */

    int nLvl;         /* Number of entries in aLvl[] array */
    int iLvl;         /* Index of current entry */
    FsdirLevel* aLvl; /* Hierarchy of directories being traversed */

    struct stat sStat;    /* Current lstat() results */
    char* zPath;          /* Path to current entry */
    sqlite3_int64 iRowid; /* Current rowid */
};

typedef struct fsdir_tab fsdir_tab;
struct fsdir_tab {
    sqlite3_vtab base; /* Base class - must be first */
};

/*
** Construct a new fsdir virtual table object.
*/
static int fsdirConnect(sqlite3* db,
                        void* pAux,
                        int argc,
                        const char* const* argv,
                        sqlite3_vtab** ppVtab,
                        char** pzErr) {
    fsdir_tab* pNew = 0;
    int rc;
    (void)pAux;
    (void)argc;
    (void)argv;
    (void)pzErr;
    rc = sqlite3_declare_vtab(db, "CREATE TABLE x" FSDIR_SCHEMA);
    if (rc == SQLITE_OK) {
        pNew = (fsdir_tab*)sqlite3_malloc(sizeof(*pNew));
        if (pNew == 0)
            return SQLITE_NOMEM;
        memset(pNew, 0, sizeof(*pNew));
        sqlite3_vtab_config(db, SQLITE_VTAB_DIRECTONLY);
    }
    *ppVtab = (sqlite3_vtab*)pNew;
    return rc;
}

/*
** This method is the destructor for fsdir vtab objects.
*/
static int fsdirDisconnect(sqlite3_vtab* pVtab) {
    sqlite3_free(pVtab);
    return SQLITE_OK;
}

/*
** Constructor for a new fsdir_cursor object.
*/
static int fsdirOpen(sqlite3_vtab* p, sqlite3_vtab_cursor** ppCursor) {
    fsdir_cursor* pCur;
    (void)p;
    pCur = sqlite3_malloc(sizeof(*pCur));
    if (pCur == 0)
        return SQLITE_NOMEM;
    memset(pCur, 0, sizeof(*pCur));
    pCur->iLvl = -1;
    *ppCursor = &pCur->base;
    return SQLITE_OK;
}

/*
** Reset a cursor back to the state it was in when first returned
** by fsdirOpen().
*/
static void fsdirResetCursor(fsdir_cursor* pCur) {
    int i;
    for (i = 0; i <= pCur->iLvl; i++) {
        FsdirLevel* pLvl = &pCur->aLvl[i];
        if (pLvl->pDir)
            closedir(pLvl->pDir);
        sqlite3_free(pLvl->zDir);
    }
    sqlite3_free(pCur->zPath);
    sqlite3_free(pCur->aLvl);
    pCur->aLvl = 0;
    pCur->zPath = 0;
    pCur->nLvl = 0;
    pCur->iLvl = -1;
    pCur->iRowid = 1;
}

/*
** Destructor for an fsdir_cursor.
*/
static int fsdirClose(sqlite3_vtab_cursor* cur) {
    fsdir_cursor* pCur = (fsdir_cursor*)cur;

    fsdirResetCursor(pCur);
    sqlite3_free(pCur);
    return SQLITE_OK;
}

/*
** Set the error message for the virtual table associated with cursor
** pCur to the results of vprintf(zFmt, ...).
*/
static void fsdirSetErrmsg(fsdir_cursor* pCur, const char* zFmt, ...) {
    va_list ap;
    va_start(ap, zFmt);
    pCur->base.pVtab->zErrMsg = sqlite3_vmprintf(zFmt, ap);
    va_end(ap);
}

/*
** Advance an fsdir_cursor to its next row of output.
*/
static int fsdirNext(sqlite3_vtab_cursor* cur) {
    fsdir_cursor* pCur = (fsdir_cursor*)cur;
    mode_t m = pCur->sStat.st_mode;

    pCur->iRowid++;
    if (S_ISDIR(m) && (pCur->iLvl == -1 || pCur->recursive)) {
        /* Descend into this directory */
        int iNew = pCur->iLvl + 1;
        FsdirLevel* pLvl;
        if (iNew >= pCur->nLvl) {
            int nNew = iNew + 1;
            sqlite3_int64 nByte = nNew * sizeof(FsdirLevel);
            FsdirLevel* aNew = (FsdirLevel*)sqlite3_realloc64(pCur->aLvl, nByte);
            if (aNew == 0)
                return SQLITE_NOMEM;
            memset(&aNew[pCur->nLvl], 0, sizeof(FsdirLevel) * (nNew - pCur->nLvl));
            pCur->aLvl = aNew;
            pCur->nLvl = nNew;
        }
        pCur->iLvl = iNew;
        pLvl = &pCur->aLvl[iNew];

        pLvl->zDir = pCur->zPath;
        pCur->zPath = 0;
        pLvl->pDir = opendir(pLvl->zDir);
        if (pLvl->pDir == 0) {
            fsdirSetErrmsg(pCur, "cannot read directory: %s", pCur->zPath);
            return SQLITE_ERROR;
        }
    }

    while (pCur->iLvl >= 0) {
        FsdirLevel* pLvl = &pCur->aLvl[pCur->iLvl];
        struct dirent* pEntry = readdir(pLvl->pDir);
        if (pEntry) {
            if (pEntry->d_name[0] == '.') {
                if (pEntry->d_name[1] == '.' && pEntry->d_name[2] == '\0')
                    continue;
                if (pEntry->d_name[1] == '\0')
                    continue;
            }
            sqlite3_free(pCur->zPath);
            pCur->zPath = sqlite3_mprintf("%s/%s", pLvl->zDir, pEntry->d_name);
            if (pCur->zPath == 0)
                return SQLITE_NOMEM;
            if (fileLinkStat(pCur->zPath, &pCur->sStat)) {
                fsdirSetErrmsg(pCur, "cannot stat file: %s", pCur->zPath);
                return SQLITE_ERROR;
            }
            return SQLITE_OK;
        }
        closedir(pLvl->pDir);
        sqlite3_free(pLvl->zDir);
        pLvl->pDir = 0;
        pLvl->zDir = 0;
        pCur->iLvl--;
    }

    /* EOF */
    sqlite3_free(pCur->zPath);
    pCur->zPath = 0;
    return SQLITE_OK;
}

/*
** Return values of columns for the row at which the series_cursor
** is currently pointing.
*/
static int fsdirColumn(sqlite3_vtab_cursor* cur, /* The cursor */
                       sqlite3_context* ctx,     /* First argument to sqlite3_result_...() */
                       int i                     /* Which column to return */
) {
    fsdir_cursor* pCur = (fsdir_cursor*)cur;
    switch (i) {
        case FSDIR_COLUMN_NAME: {
            sqlite3_result_text(ctx, pCur->zPath, -1, SQLITE_TRANSIENT);
            break;
        }

        case FSDIR_COLUMN_MODE:
            sqlite3_result_int64(ctx, pCur->sStat.st_mode);
            break;

        case FSDIR_COLUMN_MTIME:
            sqlite3_result_int64(ctx, pCur->sStat.st_mtime);
            break;

        case FSDIR_COLUMN_SIZE: {
            sqlite3_result_int64(ctx, pCur->sStat.st_size);
            break;
        }
        case FSDIR_COLUMN_PATH:
        default: {
            /* The FSDIR_COLUMN_PATH and FSDIR_COLUMN_REC are input parameters.
            ** always return their values as NULL */
            break;
        }
    }
    return SQLITE_OK;
}

/*
** Return the rowid for the current row. In this implementation, the
** first row returned is assigned rowid value 1, and each subsequent
** row a value 1 more than that of the previous.
*/
static int fsdirRowid(sqlite3_vtab_cursor* cur, sqlite_int64* pRowid) {
    fsdir_cursor* pCur = (fsdir_cursor*)cur;
    *pRowid = pCur->iRowid;
    return SQLITE_OK;
}

/*
** Return TRUE if the cursor has been moved off of the last
** row of output.
*/
static int fsdirEof(sqlite3_vtab_cursor* cur) {
    fsdir_cursor* pCur = (fsdir_cursor*)cur;
    return (pCur->zPath == 0);
}

/*
** xFilter callback.
**
** idxNum==0   PATH was not supplied (invalid function call)
** idxNum==1   PATH was supplied
*/
static int fsdirFilter(sqlite3_vtab_cursor* cur,
                       int idxNum,
                       const char* idxStr,
                       int argc,
                       sqlite3_value** argv) {
    fsdir_cursor* pCur = (fsdir_cursor*)cur;
    (void)idxStr;
    fsdirResetCursor(pCur);

    if (idxNum == 0) {
        fsdirSetErrmsg(pCur, "table function lsdir requires an argument");
        return SQLITE_ERROR;
    }

    assert(idxNum == 1 && (argc == 1 || argc == 2));
    const char* zPath = (const char*)sqlite3_value_text(argv[0]);
    if (zPath == 0) {
        fsdirSetErrmsg(pCur, "table function lsdir requires a non-NULL argument");
        return SQLITE_ERROR;
    }
    pCur->zPath = sqlite3_mprintf("%s", zPath);

    bool recursive = false;
    if (argc == 2) {
        recursive = (bool)sqlite3_value_int(argv[1]);
    }
    pCur->recursive = recursive;

    if (pCur->zPath == 0) {
        return SQLITE_NOMEM;
    }
    if (fileLinkStat(pCur->zPath, &pCur->sStat)) {
        // file does not exist, terminate via subsequent call to fsdirEof
        pCur->zPath = 0;
    }

    return SQLITE_OK;
}

/*
** SQLite will invoke this method one or more times while planning a query
** that uses the generate_series virtual table.  This routine needs to create
** a query plan for each invocation and compute an estimated cost for that
** plan.
**
** In this implementation idxNum is used to represent the
** query plan.  idxStr is unused.
**
** The query plan is represented by values of idxNum:
**
**  (1)  The path value is supplied by argv[0]
*/
static int fsdirBestIndex(sqlite3_vtab* tab, sqlite3_index_info* pIdxInfo) {
    int i;            /* Loop over constraints */
    int idxPath = -1; /* Index in pIdxInfo->aConstraint of PATH= */
    int idxRec = -1;  /* Index in pIdxInfo->aConstraint of REC= */
    int seenPath = 0; /* True if an unusable PATH= constraint is seen */
    int seenRec = 0;  /* True if an unusable REC= constraint is seen */
    const struct sqlite3_index_constraint* pConstraint;

    (void)tab;
    pConstraint = pIdxInfo->aConstraint;
    for (i = 0; i < pIdxInfo->nConstraint; i++, pConstraint++) {
        if (pConstraint->op != SQLITE_INDEX_CONSTRAINT_EQ)
            continue;
        switch (pConstraint->iColumn) {
            case FSDIR_COLUMN_PATH: {
                if (pConstraint->usable) {
                    idxPath = i;
                    seenPath = 0;
                } else if (idxPath < 0) {
                    seenPath = 1;
                }
                break;
            }
            case FSDIR_COLUMN_REC: {
                if (pConstraint->usable) {
                    idxRec = i;
                    seenRec = 0;
                } else if (idxRec < 0) {
                    seenRec = 1;
                }
                break;
            }
        }
    }
    if (seenPath || seenRec) {
        /* If input parameters are unusable, disallow this plan */
        return SQLITE_CONSTRAINT;
    }

    if (idxPath < 0) {
        pIdxInfo->idxNum = 0;
        /* The pIdxInfo->estimatedCost should have been initialized to a huge
        ** number.  Leave it unchanged. */
        pIdxInfo->estimatedRows = 0x7fffffff;
    } else {
        pIdxInfo->aConstraintUsage[idxPath].omit = 1;
        pIdxInfo->aConstraintUsage[idxPath].argvIndex = 1;
        if (idxRec >= 0) {
            pIdxInfo->aConstraintUsage[idxRec].omit = 1;
            pIdxInfo->aConstraintUsage[idxRec].argvIndex = 2;
        }
        pIdxInfo->idxNum = 1;
        pIdxInfo->estimatedCost = 100.0;
    }

    return SQLITE_OK;
}

static sqlite3_module ls_module = {
    .xConnect = fsdirConnect,
    .xBestIndex = fsdirBestIndex,
    .xDisconnect = fsdirDisconnect,
    .xOpen = fsdirOpen,
    .xClose = fsdirClose,
    .xFilter = fsdirFilter,
    .xNext = fsdirNext,
    .xEof = fsdirEof,
    .xColumn = fsdirColumn,
    .xRowid = fsdirRowid,
};

int fileio_ls_init(sqlite3* db) {
    sqlite3_create_module(db, "fileio_ls", &ls_module, 0);
    sqlite3_create_module(db, "lsdir", &ls_module, 0);
    return SQLITE_OK;
}

int fileio_scalar_init(sqlite3* db) {
    static const int flags = SQLITE_UTF8 | SQLITE_DIRECTONLY;
    sqlite3_create_function(db, "fileio_mode", 1, SQLITE_UTF8, 0, fileio_lsmode, 0, 0);
    sqlite3_create_function(db, "lsmode", 1, SQLITE_UTF8, 0, fileio_lsmode, 0, 0);

    sqlite3_create_function(db, "fileio_mkdir", -1, flags, 0, fileio_mkdir, 0, 0);
    sqlite3_create_function(db, "mkdir", -1, flags, 0, fileio_mkdir, 0, 0);

    sqlite3_create_function(db, "fileio_read", -1, flags, 0, fileio_readfile, 0, 0);
    sqlite3_create_function(db, "readfile", -1, flags, 0, fileio_readfile, 0, 0);

    sqlite3_create_function(db, "fileio_symlink", 2, flags, 0, fileio_symlink, 0, 0);
    sqlite3_create_function(db, "symlink", 2, flags, 0, fileio_symlink, 0, 0);

    sqlite3_create_function(db, "fileio_write", -1, flags, 0, fileio_writefile, 0, 0);
    sqlite3_create_function(db, "writefile", -1, flags, 0, fileio_writefile, 0, 0);

    sqlite3_create_function(db, "fileio_append", 2, flags, 0, fileio_append, 0, 0);
    return SQLITE_OK;
}

#if defined(FILEIO_WIN32_DLL) && (defined(_WIN32) || defined(WIN32))
/* To allow a standalone DLL, make test_windirent.c use the same
 * redefined SQLite API calls as the above extension code does.
 * Just pull in this .c to accomplish this. As a beneficial side
 * effect, this extension becomes a single translation unit. */

/*
** Implementation of the POSIX getenv() function using the Win32 API.
** This function is not thread-safe.
*/
const char* windirent_getenv(const char* name) {
    static char value[32768];                    /* Maximum length, per MSDN */
    DWORD dwSize = sizeof(value) / sizeof(char); /* Size in chars */
    DWORD dwRet;                                 /* Value returned by GetEnvironmentVariableA() */

    memset(value, 0, sizeof(value));
    dwRet = GetEnvironmentVariableA(name, value, dwSize);
    if (dwRet == 0 || dwRet > dwSize) {
        /*
        ** The function call to GetEnvironmentVariableA() failed -OR-
        ** the buffer is not large enough.  Either way, return NULL.
        */
        return 0;
    } else {
        /*
        ** The function call to GetEnvironmentVariableA() succeeded
        ** -AND- the buffer contains the entire value.
        */
        return value;
    }
}

/*
** Implementation of the POSIX opendir() function using the MSVCRT.
*/
LPDIR opendir(const char* dirname) {
    struct _finddata_t data;
    LPDIR dirp = (LPDIR)sqlite3_malloc(sizeof(DIR));
    SIZE_T namesize = sizeof(data.name) / sizeof(data.name[0]);

    if (dirp == NULL)
        return NULL;
    memset(dirp, 0, sizeof(DIR));

    /* TODO: Remove this if Unix-style root paths are not used. */
    if (sqlite3_stricmp(dirname, "/") == 0) {
        dirname = windirent_getenv("SystemDrive");
    }

    memset(&data, 0, sizeof(struct _finddata_t));
    _snprintf(data.name, namesize, "%s\\*", dirname);
    dirp->d_handle = _findfirst(data.name, &data);

    if (dirp->d_handle == BAD_INTPTR_T) {
        closedir(dirp);
        return NULL;
    }

    /* TODO: Remove this block to allow hidden and/or system files. */
    if (is_filtered(data)) {
    next:

        memset(&data, 0, sizeof(struct _finddata_t));
        if (_findnext(dirp->d_handle, &data) == -1) {
            closedir(dirp);
            return NULL;
        }

        /* TODO: Remove this block to allow hidden and/or system files. */
        if (is_filtered(data))
            goto next;
    }

    dirp->d_first.d_attributes = data.attrib;
    strncpy(dirp->d_first.d_name, data.name, NAME_MAX);
    dirp->d_first.d_name[NAME_MAX] = '\0';

    return dirp;
}

/*
** Implementation of the POSIX readdir() function using the MSVCRT.
*/
LPDIRENT readdir(LPDIR dirp) {
    struct _finddata_t data;

    if (dirp == NULL)
        return NULL;

    if (dirp->d_first.d_ino == 0) {
        dirp->d_first.d_ino++;
        dirp->d_next.d_ino++;

        return &dirp->d_first;
    }

next:

    memset(&data, 0, sizeof(struct _finddata_t));
    if (_findnext(dirp->d_handle, &data) == -1)
        return NULL;

    /* TODO: Remove this block to allow hidden and/or system files. */
    if (is_filtered(data))
        goto next;

    dirp->d_next.d_ino++;
    dirp->d_next.d_attributes = data.attrib;
    strncpy(dirp->d_next.d_name, data.name, NAME_MAX);
    dirp->d_next.d_name[NAME_MAX] = '\0';

    return &dirp->d_next;
}

/*
** Implementation of the POSIX readdir_r() function using the MSVCRT.
*/
INT readdir_r(LPDIR dirp, LPDIRENT entry, LPDIRENT* result) {
    struct _finddata_t data;

    if (dirp == NULL)
        return EBADF;

    if (dirp->d_first.d_ino == 0) {
        dirp->d_first.d_ino++;
        dirp->d_next.d_ino++;

        entry->d_ino = dirp->d_first.d_ino;
        entry->d_attributes = dirp->d_first.d_attributes;
        strncpy(entry->d_name, dirp->d_first.d_name, NAME_MAX);
        entry->d_name[NAME_MAX] = '\0';

        *result = entry;
        return 0;
    }

next:

    memset(&data, 0, sizeof(struct _finddata_t));
    if (_findnext(dirp->d_handle, &data) == -1) {
        *result = NULL;
        return ENOENT;
    }

    /* TODO: Remove this block to allow hidden and/or system files. */
    if (is_filtered(data))
        goto next;

    entry->d_ino = (ino_t)-1; /* not available */
    entry->d_attributes = data.attrib;
    strncpy(entry->d_name, data.name, NAME_MAX);
    entry->d_name[NAME_MAX] = '\0';

    *result = entry;
    return 0;
}

/*
** Implementation of the POSIX closedir() function using the MSVCRT.
*/
INT closedir(LPDIR dirp) {
    INT result = 0;

    if (dirp == NULL)
        return EINVAL;

    if (dirp->d_handle != NULL_INTPTR_T && dirp->d_handle != BAD_INTPTR_T) {
        result = _findclose(dirp->d_handle);
    }

    sqlite3_free(dirp);
    return result;
}
#endif

// ---------------------------------
// src/fileio/scan.c
// ---------------------------------
// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// scanfile(name)
// Reads a file with the specified name line by line.
// Implemented as a table-valued function.

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#else
#include <sys/types.h>
#endif

SQLITE_EXTENSION_INIT3

/*
 * readline reads chars from the input `stream` until it encounters \n char.
 * Returns the number or characters read.
 *
 * `lineptr` points to the first character read.
 * `n` equals the current buffer size.
 */
static ssize_t readline(char** lineptr, size_t* n, FILE* stream) {
    char* bufptr = NULL;
    char* p = bufptr;
    size_t size;
    int c;

    if (lineptr == NULL) {
        return -1;
    }
    if (stream == NULL) {
        return -1;
    }
    if (n == NULL) {
        return -1;
    }
    bufptr = *lineptr;
    size = *n;

    c = fgetc(stream);
    if (c == EOF) {
        return -1;
    }
    if (bufptr == NULL) {
        bufptr = malloc(128);
        if (bufptr == NULL) {
            return -1;
        }
        size = 128;
    }
    p = bufptr;
    while (c != EOF) {
        if ((ssize_t)(p - bufptr) > (ssize_t)(size - 1)) {
            size = size + 128;
            bufptr = realloc(bufptr, size);
            if (bufptr == NULL) {
                return -1;
            }
        }
        *p++ = c;
        if (c == '\n') {
            break;
        }
        c = fgetc(stream);
    }

    *p++ = '\0';
    *lineptr = bufptr;
    *n = size;

    return p - bufptr - 1;
}

typedef struct {
    sqlite3_vtab base;
} Table;

typedef struct {
    sqlite3_vtab_cursor base;
    const char* name;
    FILE* in;
    bool eof;
    char* line;
    sqlite3_int64 rowid;
} Cursor;

#define COLUMN_ROWID -1
#define COLUMN_VALUE 0
#define COLUMN_NAME 1

// xconnect creates the virtual table.
static int xconnect(sqlite3* db,
                    void* aux,
                    int argc,
                    const char* const* argv,
                    sqlite3_vtab** vtabptr,
                    char** errptr) {
    (void)aux;
    (void)argc;
    (void)argv;
    (void)errptr;

    int rc = sqlite3_declare_vtab(db, "CREATE TABLE x(value text, name hidden)");
    if (rc != SQLITE_OK) {
        return rc;
    }

    Table* table = sqlite3_malloc(sizeof(*table));
    *vtabptr = (sqlite3_vtab*)table;
    if (table == NULL) {
        return SQLITE_NOMEM;
    }
    memset(table, 0, sizeof(*table));
    sqlite3_vtab_config(db, SQLITE_VTAB_DIRECTONLY);
    return SQLITE_OK;
}

// xdisconnect destroys the virtual table.
static int xdisconnect(sqlite3_vtab* vtable) {
    Table* table = (Table*)vtable;
    sqlite3_free(table);
    return SQLITE_OK;
}

// xopen creates a new cursor.
static int xopen(sqlite3_vtab* vtable, sqlite3_vtab_cursor** curptr) {
    (void)vtable;
    Cursor* cursor = sqlite3_malloc(sizeof(*cursor));
    if (cursor == NULL) {
        return SQLITE_NOMEM;
    }
    memset(cursor, 0, sizeof(*cursor));
    *curptr = &cursor->base;
    return SQLITE_OK;
}

// xclose destroys the cursor.
static int xclose(sqlite3_vtab_cursor* cur) {
    Cursor* cursor = (Cursor*)cur;
    if (cursor->in != NULL) {
        fclose(cursor->in);
    }
    if (cursor->line != NULL) {
        free(cursor->line);
    }
    sqlite3_free(cur);
    return SQLITE_OK;
}

// xnext advances the cursor to its next row of output.
static int xnext(sqlite3_vtab_cursor* cur) {
    Cursor* cursor = (Cursor*)cur;
    cursor->rowid++;
    size_t bufsize = 0;
    ssize_t len = readline(&cursor->line, &bufsize, cursor->in);
    if (len == -1) {
        cursor->eof = true;
    }
    if (len >= 1 && cursor->line[len - 1] == '\n') {
        cursor->line[len - 1] = '\0';
    }
    if (len >= 2 && cursor->line[len - 2] == '\r') {
        cursor->line[len - 2] = '\0';
    }
    return SQLITE_OK;
}

// xcolumn returns the current cursor value.
static int xcolumn(sqlite3_vtab_cursor* cur, sqlite3_context* ctx, int col_idx) {
    (void)col_idx;
    Cursor* cursor = (Cursor*)cur;
    switch (col_idx) {
        case COLUMN_VALUE:
            sqlite3_result_text(ctx, (const char*)cursor->line, -1, SQLITE_TRANSIENT);
            break;

        case COLUMN_NAME:
            sqlite3_result_text(ctx, cursor->name, -1, SQLITE_TRANSIENT);
            break;

        default:
            break;
    }
    return SQLITE_OK;
}

// xrowid returns the rowid for the current row.
static int xrowid(sqlite3_vtab_cursor* cur, sqlite_int64* rowid_ptr) {
    Cursor* cursor = (Cursor*)cur;
    *rowid_ptr = cursor->rowid;
    return SQLITE_OK;
}

// xeof returns TRUE if the cursor has been moved off of the last row of output.
static int xeof(sqlite3_vtab_cursor* cur) {
    Cursor* cursor = (Cursor*)cur;
    return cursor->eof;
}

// xfilter rewinds the cursor back to the first row of output.
static int xfilter(sqlite3_vtab_cursor* cur,
                   int idx_num,
                   const char* idx_str,
                   int argc,
                   sqlite3_value** argv) {
    (void)idx_num;
    (void)idx_str;

    if (argc != 1) {
        return SQLITE_ERROR;
    }
    const char* name = (const char*)sqlite3_value_text(argv[0]);

    Cursor* cursor = (Cursor*)cur;
    sqlite3_vtab* vtable = (cursor->base).pVtab;

    // free resources from the previous file, if any
    if (cursor->in != NULL) {
        fclose(cursor->in);
    }
    if (cursor->line != NULL) {
        free(cursor->line);
    }

    // reset the cursor
    cursor->name = name;
    cursor->eof = false;
    cursor->line = NULL;
    cursor->rowid = 0;

    cursor->in = fopen(cursor->name, "r");
    if (cursor->in == NULL) {
        vtable->zErrMsg = sqlite3_mprintf("cannot open '%s' for reading", cursor->name);
        return SQLITE_ERROR;
    }

    return xnext(cur);
}

// xbest_index instructs SQLite to pass certain arguments to xFilter.
static int xbest_index(sqlite3_vtab* vtable, sqlite3_index_info* index_info) {
    // for (size_t i = 0; i < index_info->nConstraint; i++) {
    //     const struct sqlite3_index_constraint* constraint = index_info->aConstraint + i;
    //     printf("i=%zu iColumn=%d, op=%d, usable=%d\n", i, constraint->iColumn, constraint->op,
    //            constraint->usable);
    // }

    // only the name argument is supported
    if (index_info->nConstraint != 1) {
        vtable->zErrMsg = sqlite3_mprintf("scanfile() expects a single constraint (name)");
        return SQLITE_ERROR;
    }

    const struct sqlite3_index_constraint* constraint = index_info->aConstraint;
    if (constraint->iColumn != COLUMN_NAME) {
        vtable->zErrMsg = sqlite3_mprintf("scanfile() expects a name constraint)");
        return SQLITE_ERROR;
    }

    if (constraint->usable == 0) {
        // unusable contraint
        return SQLITE_CONSTRAINT;
    }

    // pass the name argument to xFilter
    index_info->aConstraintUsage[0].argvIndex = COLUMN_NAME;
    index_info->aConstraintUsage[0].omit = 1;
    index_info->estimatedCost = (double)1000;
    index_info->estimatedRows = 1000;
    return SQLITE_OK;
}

static sqlite3_module scan_module = {
    .xConnect = xconnect,
    .xBestIndex = xbest_index,
    .xDisconnect = xdisconnect,
    .xOpen = xopen,
    .xClose = xclose,
    .xFilter = xfilter,
    .xNext = xnext,
    .xEof = xeof,
    .xColumn = xcolumn,
    .xRowid = xrowid,
};

int fileio_scan_init(sqlite3* db) {
    sqlite3_create_module(db, "fileio_scan", &scan_module, 0);
    sqlite3_create_module(db, "scanfile", &scan_module, 0);
    return SQLITE_OK;
}

#endif // SQLEAN_ENABLE_FILEIO
#ifdef SQLEAN_ENABLE_IPADDR
// ---------------------------------
// src/ipaddr/extension.c
// ---------------------------------
// Copyright (c) 2021 Vincent Bernat, MIT License
// https://github.com/nalgeon/sqlean

// IP address manipulation in SQLite.

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __FreeBSD__
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#endif

SQLITE_EXTENSION_INIT3

struct ipaddress {
    int af;
    union {
        struct in6_addr ipv6;
        struct in_addr ipv4;
    };
    unsigned masklen;
};

static struct ipaddress* parse_ipaddress(const char* address) {
    struct ipaddress* ip = NULL;
    unsigned char buf[sizeof(struct in6_addr)];
    char* sep = strchr(address, '/');
    unsigned long masklen;
    if (sep) {
        char* end;
        errno = 0;
        masklen = strtoul(sep + 1, &end, 10);
        if (errno != 0 || sep + 1 == end || *end != '\0')
            return NULL;
        *sep = '\0';
    }
    if (inet_pton(AF_INET, address, buf)) {
        if (sep && masklen > 32)
            goto end;

        ip = sqlite3_malloc(sizeof(struct ipaddress));
        memcpy(&ip->ipv4, buf, sizeof(struct in_addr));
        ip->af = AF_INET;
        ip->masklen = sep ? masklen : 32;
    } else if (inet_pton(AF_INET6, address, buf)) {
        if (sep && masklen > 128)
            goto end;

        ip = sqlite3_malloc(sizeof(struct ipaddress));
        memcpy(&ip->ipv6, buf, sizeof(struct in6_addr));
        ip->af = AF_INET6;
        ip->masklen = sep ? masklen : 128;
    }
end:
    if (sep)
        *sep = '/';
    return ip;
}

static void ipaddr_ipfamily(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
        sqlite3_result_null(context);
        return;
    }
    const char* address = (char*)sqlite3_value_text(argv[0]);
    struct ipaddress* ip = parse_ipaddress(address);
    if (ip == NULL) {
        sqlite3_result_null(context);
        return;
    }
    sqlite3_result_int(context, ip->af == AF_INET ? 4 : 6);
    sqlite3_free(ip);
}

static void ipaddr_iphost(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
        sqlite3_result_null(context);
        return;
    }
    const char* address = (char*)sqlite3_value_text(argv[0]);
    struct ipaddress* ip = parse_ipaddress(address);
    if (ip == NULL) {
        sqlite3_result_null(context);
        return;
    }
    if (ip->af == AF_INET) {
        char* result = sqlite3_malloc(INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &ip->ipv4, result, INET_ADDRSTRLEN);
        sqlite3_result_text(context, result, -1, sqlite3_free);
    } else if (ip->af == AF_INET6) {
        char* result = sqlite3_malloc(INET6_ADDRSTRLEN);
        inet_ntop(AF_INET6, &ip->ipv6, result, INET6_ADDRSTRLEN);
        sqlite3_result_text(context, result, -1, sqlite3_free);
    }
    sqlite3_free(ip);
}

static void ipaddr_ipmasklen(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
        sqlite3_result_null(context);
        return;
    }
    const char* address = (char*)sqlite3_value_text(argv[0]);
    struct ipaddress* ip = parse_ipaddress(address);
    if (ip == NULL) {
        sqlite3_result_null(context);
        return;
    }
    sqlite3_result_int(context, ip->masklen);
    return;
}

static void ipaddr_ipnetwork(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
        sqlite3_result_null(context);
        return;
    }
    const char* address = (char*)sqlite3_value_text(argv[0]);
    struct ipaddress* ip = parse_ipaddress(address);
    if (ip == NULL) {
        sqlite3_result_null(context);
        return;
    }
    if (ip->af == AF_INET) {
        char buf[INET_ADDRSTRLEN];
        ip->ipv4.s_addr =
            htonl(ntohl(ip->ipv4.s_addr) & ~(uint32_t)((1ULL << (32 - ip->masklen)) - 1));
        inet_ntop(AF_INET, &ip->ipv4, buf, INET_ADDRSTRLEN);
        char* result = sqlite3_malloc(INET_ADDRSTRLEN + 3);
        sprintf(result, "%s/%u", buf, ip->masklen);
        sqlite3_result_text(context, result, -1, sqlite3_free);
    } else if (ip->af == AF_INET6) {
        char buf[INET6_ADDRSTRLEN];
        for (unsigned i = 0; i < 16; i++) {
            if (ip->masklen / 8 < i)
                ip->ipv6.s6_addr[i] = 0;
            else if (ip->masklen / 8 == i)
                ip->ipv6.s6_addr[i] &= ~(ip->masklen % 8);
        }
        inet_ntop(AF_INET6, &ip->ipv6, buf, INET6_ADDRSTRLEN);
        char* result = sqlite3_malloc(INET6_ADDRSTRLEN + 4);
        sprintf(result, "%s/%u", buf, ip->masklen);
        sqlite3_result_text(context, result, -1, sqlite3_free);
    }
    sqlite3_free(ip);
}

static void ipaddr_ipcontains(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL || sqlite3_value_type(argv[1]) == SQLITE_NULL) {
        sqlite3_result_null(context);
        return;
    }

    const char* address1 = (char*)sqlite3_value_text(argv[0]);
    struct ipaddress* ip1 = parse_ipaddress(address1);
    const char* address2 = (char*)sqlite3_value_text(argv[1]);
    struct ipaddress* ip2 = parse_ipaddress(address2);
    if (ip1 == NULL || ip2 == NULL) {
        sqlite3_result_null(context);
        goto end;
    }
    if (ip1->af != ip2->af || ip1->masklen > ip2->masklen) {
        sqlite3_result_int(context, 0);
        goto end;
    }

    if (ip1->af == AF_INET) {
        ip1->ipv4.s_addr =
            htonl(ntohl(ip1->ipv4.s_addr) & ~(uint32_t)((1ULL << (32 - ip1->masklen)) - 1));
        ip2->ipv4.s_addr =
            htonl(ntohl(ip2->ipv4.s_addr) & ~(uint32_t)((1ULL << (32 - ip1->masklen)) - 1));
        sqlite3_result_int(context, ip1->ipv4.s_addr == ip2->ipv4.s_addr);
        goto end;
    }
    if (ip1->af == AF_INET6) {
        for (unsigned i = 0; i < 16; i++) {
            if (ip1->masklen / 8 < i) {
                ip1->ipv6.s6_addr[i] = 0;
                ip2->ipv6.s6_addr[i] = 0;
            } else if (ip1->masklen / 8 == i) {
                ip1->ipv6.s6_addr[i] &= ~(ip1->masklen % 8);
                ip2->ipv6.s6_addr[i] &= ~(ip1->masklen % 8);
            }
            if (ip1->ipv6.s6_addr[i] != ip2->ipv6.s6_addr[i]) {
                sqlite3_result_int(context, 0);
                goto end;
            }
        }
        sqlite3_result_int(context, 1);
    }
end:
    sqlite3_free(ip1);
    sqlite3_free(ip2);
}

int ipaddr_init(sqlite3* db) {
    static const int flags = SQLITE_UTF8 | SQLITE_INNOCUOUS | SQLITE_DETERMINISTIC;
    sqlite3_create_function(db, "ipfamily", 1, flags, 0, ipaddr_ipfamily, 0, 0);
    sqlite3_create_function(db, "iphost", 1, flags, 0, ipaddr_iphost, 0, 0);
    sqlite3_create_function(db, "ipmasklen", 1, flags, 0, ipaddr_ipmasklen, 0, 0);
    sqlite3_create_function(db, "ipnetwork", 1, flags, 0, ipaddr_ipnetwork, 0, 0);
    sqlite3_create_function(db, "ipcontains", 2, flags, 0, ipaddr_ipcontains, 0, 0);
    return SQLITE_OK;
}

#endif // SQLEAN_ENABLE_IPADDR
#ifdef SQLEAN_ENABLE_VSV
// ---------------------------------
// src/vsv/extension.c
// ---------------------------------
// vsv extension by Keith Medcalf, Public Domain
// http://www.dessus.com/files/vsv.c

// Modified by Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean/

// CSV files as virtual tables in SQLite

/*
** 2020-02-08 modified by Keith Medcalf who also disclaims all copyright
** on the modifications and hereby places this code in the public domain
**
** This file contains the implementation of an SQLite virtual table for
** reading VSV (Variably Separated Values), which are like CSV files,
** but subtly different.  VSV supports a number of extensions to the
** CSV format as well as more processing options.
**
**
** Usage:
**
**  create virtual table temp.vsv using vsv(...);
**  select * from vsv;
**
** The parameters to the vsv module (the vsv(...) part) are as follows:
**
**  filename=STRING     the filename, passed to the Operating System
**  data=STRING         alternative data
**  schema=STRING       Alternate Schema to use
**  columns=N           columns parsed from the VSV file
**  header=BOOL         whether or not a header row is present
**  skip=N              number of leading data rows to skip
**  rsep=STRING         record separator
**  fsep=STRING         field separator
**  dsep=STRING         decimal separator
**  validatetext=BOOL   validate UTF-8 encoding of text fields
**  affinity=AFFINITY   affinity to apply to each returned value
**  nulls=BOOL          empty fields are returned as NULL
**
**
** Defaults:
**
**  filename / data     nothing.  You must provide one or the other
**                      it is an error to provide both or neither
**  schema              nothing.  If not provided then one will be
**                      generated for you from the header, or if no
**                      header is available then autogenerated using
**                      field names manufactured as cX where X is the
**                      column number
**  columns             nothing.  If not specified then the number of
**                      columns is determined by counting the fields
**                      in the first record of the VSV file (which
**                      will be the header row if header is specified),
**                      the number of columns is not parsed from the
**                      schema even if one is provided
**  header=no           no header row in the VSV file
**  skip=0              do not skip any data rows in the VSV file
**  fsep=','            default field separator is a comma
**  rsep='\n'           default record separator is a newline
**  dsep='.'            default decimal separator is a point
**  validatetext=no     do not validate text field encoding
**  affinity=none       do not apply affinity to each returned value
**  nulls=off           empty fields returned as zero-length
**
**
** Parameter types:
**
**  STRING              means a quoted string
**  N                   means a whole number not containing a sign
**  BOOL                means something that evaluates as true or false
**                          it is case insensitive
**                          yes, no, true, false, 1, 0
**  AFFINITY            means an SQLite3 type specification
**                          it is case insensitive
**                          none, blob, text, integer, real, numeric
**
** STRING means a quoted string.  The quote character may be either
** a single quote or a double quote.  Two quote characters in a row
** will be replaced with a single quote character.  STRINGS do not
** need to be quoted if it is obvious where they begin and end
** (that is, they do not contain a comma).  Leading and trailing
** spaces will be trimmed from unquoted strings.
**
**    filename =./this/filename.here, ...
**    filename =./this/filename.here , ...
**    filename = ./this/filename.here, ...
**    filename = ./this/filename.here , ...
**    filename = './this/filename.here', ...
**    filename = "./this/filename.here", ...
**
**  are all equivalent.
**
** BOOL defaults to true so the following specifications are all the
** same:
**
**  header = true
**  header = yes
**  header = 1
**  header
**
**
** Specific Parameters:
**
** The platform/compiler/OS fopen call is responsible for interpreting
** the filename.  It may contain anything recognized by the OS.
**
** The separator string containing exactly one character, or a valid
** escape sequence.  Recognized escape sequences are:
**
**  \t                  horizontal tab, ascii character 9 (0x09)
**  \n                  linefeed, ascii character 10 (0x0a)
**  \v                  vertical tab, ascii character 11 (0x0b)
**  \f                  form feed, ascii character 12 (0x0c)
**  \xhh                specific byte where hh is hexadecimal
**
** The validatetext setting will cause the validity of the field
** encoding (not its contents) to be verified.  It effects how
** fields that are supposed to contain text will be returned to
** the SQLite3 library in order to prevent invalid utf8 data from
** being stored or processed as if it were valid utf8 text.
**
** The nulls option will cause fields that do not contain anything
** to return NULL rather than an empty result.  Two separators
** side-by-each with no intervening characters at all will be
** returned as NULL if nulls is true and if nulls is false or
** the contents are explicity empty ("") then a 0 length blob
** (if affinity=blob) or 0 length text string.
**
** For the affinity setting, the following processing is applied to
** each value returned by the VSV virtual table:
**
**  none                no affinity is applied, all fields will be
**                      returned as text just like in the original
**                      csv module, embedded nulls will terminate
**                      the text.  if validatetext is in effect then
**                      an error will be thrown if the field does
**                      not contain validly encoded text or contains
**                      embedded nulls
**
**  blob                all fields will be returned as blobs
**                      validatetext has no effect
**
**  text                all fields will be returned as text just
**                      like in the original csv module, embedded
**                      nulls will terminate the text.
**                      if validatetext is in effect then a blob
**                      will be returned if the field does not
**                      contain validly encoded text or the field
**                      contains embedded nulls
**
**  integer             if the field data looks like an integer,
**                      (regex "^ *(\+|-)?\d+ *$"),
**                      then an integer will be returned as
**                      provided by the compiler and platform
**                      runtime strtoll function
**                      otherwise the field will be processed as
**                      text as defined above
**
**  real                if the field data looks like a number,
**                      (regex "^ *(\+|-)?(\d+\.?\d*|\d*\.?\d+)([eE](\+|-)?\d+)? *$")
**                      then a double will be returned as
**                      provided by the compiler and platform
**                      runtime strtold function otherwise the
**                      field will be processed as text as
**                      defined above
**
**  numeric             if the field looks like an integer
**                      (see integer above) that integer will be
**                      returned
**                      if the field looks like a number
**                      (see real above) then the number will
**                      returned as an integer if it has no
**                      fractional part and
**                      (a) your platform/compiler supports
**                      long double and the number will fit in
**                      a 64-bit integer; or,
**                      (b) your platform/compiler does not
**                      support long double (treats it as a double)
**                      then a 64-bit integer will only be returned
**                      if the value would fit in a 6-byte varint,
**                      otherwise a double will be returned
**
** The nulls option will cause fields that do not contain anything
** to return NULL rather than an empty result.  Two separators
** side-by-each with no intervening characters at all will be
** returned as NULL if nulls is true and if nulls is false or
** the contents are explicity empty ("") then a 0 length blob
** (if affinity=blob) or 0 length text string will be returned.
**
*/
/*
** 2016-05-28
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
******************************************************************************
**
** This file contains the implementation of an SQLite virtual table for
** reading CSV files.
**
** Usage:
**
**    .load ./csv
**    CREATE VIRTUAL TABLE temp.csv USING csv(filename=FILENAME);
**    SELECT * FROM csv;
**
** The columns are named "c1", "c2", "c3", ... by default.  Or the
** application can define its own CREATE TABLE statement using the
** schema= parameter, like this:
**
**    CREATE VIRTUAL TABLE temp.csv2 USING csv(
**       filename = "../http.log",
**       schema = "CREATE TABLE x(date,ipaddr,url,referrer,userAgent)"
**    );
**
** Instead of specifying a file, the text of the CSV can be loaded using
** the data= parameter.
**
** If the columns=N parameter is supplied, then the CSV file is assumed to have
** N columns.  If both the columns= and schema= parameters are omitted, then
** the number and names of the columns is determined by the first line of
** the CSV input.
**
*/
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

SQLITE_EXTENSION_INIT3

/**A macro to hint to the compiler that a function should not be * *inlined.*/
#if defined(__GNUC__)
#define VSV_NOINLINE __attribute__((noinline))
#elif defined(_MSC_VER) && _MSC_VER >= 1310
#define VSV_NOINLINE __declspec(noinline)
#else
#define VSV_NOINLINE
#endif

/*
** Max size of the error message in a VsvReader
*/
#define VSV_MXERR 200

/*
** Size of the VsvReader input buffer
*/
#define VSV_INBUFSZ 1024

/*
** A context object used when read a VSV file.
*/
typedef struct VsvReader VsvReader;
struct VsvReader {
    FILE* in;             /* Read the VSV text from this input stream */
    char* z;              /* Accumulated text for a field */
    int n;                /* Number of bytes in z */
    int nAlloc;           /* Space allocated for z[] */
    int nLine;            /* Current line number */
    int bNotFirst;        /* True if prior text has been seen */
    int cTerm;            /* Character that terminated the most recent field */
    int fsep;             /* Field Seperator Character */
    int rsep;             /* Record Seperator Character */
    int dsep;             /* Decimal Seperator Character */
    int affinity;         /* Perform Affinity Conversions */
    int notNull;          /* Have we seen data for field */
    size_t iIn;           /* Next unread character in the input buffer */
    size_t nIn;           /* Number of characters in the input buffer */
    char* zIn;            /* The input buffer */
    char zErr[VSV_MXERR]; /* Error message */
};

/*
** Initialize a VsvReader object
*/
static void vsv_reader_init(VsvReader* p) {
    p->in = 0;
    p->z = 0;
    p->n = 0;
    p->nAlloc = 0;
    p->nLine = 0;
    p->bNotFirst = 0;
    p->nIn = 0;
    p->zIn = 0;
    p->notNull = 0;
    p->zErr[0] = 0;
}

/*
** Close and reset a VsvReader object
*/
static void vsv_reader_reset(VsvReader* p) {
    if (p->in) {
        fclose(p->in);
        sqlite3_free(p->zIn);
    }
    sqlite3_free(p->z);
    vsv_reader_init(p);
}

/*
** Report an error on a VsvReader
*/
static void vsv_errmsg(VsvReader* p, const char* zFormat, ...) {
    va_list ap;
    va_start(ap, zFormat);
    sqlite3_vsnprintf(VSV_MXERR, p->zErr, zFormat, ap);
    va_end(ap);
}

/*
** Open the file associated with a VsvReader
** Return the number of errors.
*/
static int vsv_reader_open(VsvReader* p,          /* The reader to open */
                           const char* zFilename, /* Read from this filename */
                           const char* zData      /*  ... or use this data */
) {
    if (zFilename) {
        p->zIn = sqlite3_malloc(VSV_INBUFSZ);
        if (p->zIn == 0) {
            vsv_errmsg(p, "out of memory");
            return 1;
        }
        p->in = fopen(zFilename, "rb");
        if (p->in == 0) {
            sqlite3_free(p->zIn);
            vsv_reader_reset(p);
            vsv_errmsg(p, "cannot open '%s' for reading", zFilename);
            return 1;
        }
    } else {
        assert(p->in == 0);
        p->zIn = (char*)zData;
        p->nIn = strlen(zData);
    }
    return 0;
}

/*
** The input buffer has overflowed.  Refill the input buffer, then
** return the next character
*/
static VSV_NOINLINE int vsv_getc_refill(VsvReader* p) {
    size_t got;

    assert(p->iIn >= p->nIn); /* Only called on an empty input buffer */
    assert(p->in != 0);       /* Only called if reading from a file */

    got = fread(p->zIn, 1, VSV_INBUFSZ, p->in);
    if (got == 0) {
        return EOF;
    }
    p->nIn = got;
    p->iIn = 1;
    return p->zIn[0];
}

/*
** Return the next character of input.  Return EOF at end of input.
*/
static int vsv_getc(VsvReader* p) {
    if (p->iIn >= p->nIn) {
        if (p->in != 0) {
            return vsv_getc_refill(p);
        }
        return EOF;
    }
    return ((unsigned char*)p->zIn)[p->iIn++];
}

/*
** Increase the size of p->z and append character c to the end.
** Return 0 on success and non-zero if there is an OOM error
*/
static VSV_NOINLINE int vsv_resize_and_append(VsvReader* p, char c) {
    char* zNew;
    int nNew = p->nAlloc * 2 + 100;
    zNew = sqlite3_realloc64(p->z, nNew);
    if (zNew) {
        p->z = zNew;
        p->nAlloc = nNew;
        p->z[p->n++] = c;
        return 0;
    } else {
        vsv_errmsg(p, "out of memory");
        return 1;
    }
}

/*
** Append a single character to the VsvReader.z[] array.
** Return 0 on success and non-zero if there is an OOM error
*/
static int vsv_append(VsvReader* p, char c) {
    if (p->n >= p->nAlloc - 1) {
        return vsv_resize_and_append(p, c);
    }
    p->z[p->n++] = c;
    return 0;
}

/*
** Read a single field of VSV text.  Compatible with rfc4180 and extended
** with the option of having a separator other than ",".
**
**   +  Input comes from p->in.
**   +  Store results in p->z of length p->n.  Space to hold p->z comes
**      from sqlite3_malloc64().
**   +  Keep track of the line number in p->nLine.
**   +  Store the character that terminates the field in p->cTerm.  Store
**      EOF on end-of-file.
**
** Return 0 at EOF or on OOM.  On EOF, the p->cTerm character will have
** been set to EOF.
*/
static char* vsv_read_one_field(VsvReader* p) {
    int c;
    p->notNull = 0;
    p->n = 0;
    c = vsv_getc(p);
    if (c == EOF) {
        p->cTerm = EOF;
        return 0;
    }
    if (c == '"') {
        int pc, ppc;
        int startLine = p->nLine;
        p->notNull = 1;
        pc = ppc = 0;
        while (1) {
            c = vsv_getc(p);
            if (c == '\n') {
                p->nLine++;
            }
            if (c == '"' && pc == '"') {
                pc = ppc;
                ppc = 0;
                continue;
            }
            if ((c == p->fsep && pc == '"') || (c == p->rsep && pc == '"') ||
                (p->rsep == '\n' && c == '\n' && pc == '\r' && ppc == '"') ||
                (c == EOF && pc == '"')) {
                do {
                    p->n--;
                } while (p->z[p->n] != '"');
                p->cTerm = (char)c;
                break;
            }
            if (pc == '"' && p->rsep == '\n' && c != '\r') {
                vsv_errmsg(p, "line %d: unescaped %c character", p->nLine, '"');
                break;
            }
            if (c == EOF) {
                vsv_errmsg(p, "line %d: unterminated %c-quoted field\n", startLine, '"');
                p->cTerm = (char)c;
                break;
            }
            if (vsv_append(p, (char)c)) {
                return 0;
            }
            ppc = pc;
            pc = c;
        }
    } else {
        /*
        ** If this is the first field being parsed and it begins with the
        ** UTF-8 BOM  (0xEF BB BF) then skip the BOM
        */
        if ((c & 0xff) == 0xef && p->bNotFirst == 0) {
            vsv_append(p, (char)c);
            c = vsv_getc(p);
            if ((c & 0xff) == 0xbb) {
                vsv_append(p, (char)c);
                c = vsv_getc(p);
                if ((c & 0xff) == 0xbf) {
                    p->bNotFirst = 1;
                    p->n = 0;
                    return vsv_read_one_field(p);
                }
            }
        }
        while (c != EOF && c != p->rsep && c != p->fsep) {
            if (c == '\n')
                p->nLine++;
            if (!p->notNull)
                p->notNull = 1;
            if (vsv_append(p, (char)c))
                return 0;
            c = vsv_getc(p);
        }
        if (c == '\n') {
            p->nLine++;
        }
        if (p->n > 0 && (p->rsep == '\n' || p->fsep == '\n') && p->z[p->n - 1] == '\r') {
            p->n--;
            if (p->n == 0) {
                p->notNull = 0;
            }
        }
        p->cTerm = (char)c;
    }
    if (p->z) {
        p->z[p->n] = 0;
    }
    p->bNotFirst = 1;
    return p->z;
}

/*
** Forward references to the various virtual table methods implemented
** in this file.
*/
static int vsvtabCreate(sqlite3*, void*, int, const char* const*, sqlite3_vtab**, char**);
static int vsvtabConnect(sqlite3*, void*, int, const char* const*, sqlite3_vtab**, char**);
static int vsvtabBestIndex(sqlite3_vtab*, sqlite3_index_info*);
static int vsvtabDisconnect(sqlite3_vtab*);
static int vsvtabOpen(sqlite3_vtab*, sqlite3_vtab_cursor**);
static int vsvtabClose(sqlite3_vtab_cursor*);
static int vsvtabFilter(sqlite3_vtab_cursor*,
                        int idxNum,
                        const char* idxStr,
                        int argc,
                        sqlite3_value** argv);
static int vsvtabNext(sqlite3_vtab_cursor*);
static int vsvtabEof(sqlite3_vtab_cursor*);
static int vsvtabColumn(sqlite3_vtab_cursor*, sqlite3_context*, int);
static int vsvtabRowid(sqlite3_vtab_cursor*, sqlite3_int64*);

/*
** An instance of the VSV virtual table
*/
typedef struct VsvTable {
    sqlite3_vtab base; /* Base class.  Must be first */
    char* zFilename;   /* Name of the VSV file */
    char* zData;       /* Raw VSV data in lieu of zFilename */
    long iStart;       /* Offset to start of data in zFilename */
    int nCol;          /* Number of columns in the VSV file */
    int fsep;          /* The field seperator for this VSV file */
    int rsep;          /* The record seperator for this VSV file */
    int dsep;          /* The record decimal for this VSV file */
    int affinity;      /* Perform affinity conversions */
    int nulls;         /* Process NULLs */
    int validateUTF8;  /* Validate UTF8 */
} VsvTable;

/*
** A cursor for the VSV virtual table
*/
typedef struct VsvCursor {
    sqlite3_vtab_cursor base; /* Base class.  Must be first */
    VsvReader rdr;            /* The VsvReader object */
    char** azVal;             /* Value of the current row */
    int* aLen;                /* Allocation Length of each entry */
    int* dLen;                /* Data Length of each entry */
    sqlite3_int64 iRowid;     /* The current rowid.  Negative for EOF */
} VsvCursor;

/*
** Transfer error message text from a reader into a VsvTable
*/
static void vsv_xfer_error(VsvTable* pTab, VsvReader* pRdr) {
    sqlite3_free(pTab->base.zErrMsg);
    pTab->base.zErrMsg = sqlite3_mprintf("%s", pRdr->zErr);
}

/*
** This method is the destructor for a VsvTable object.
*/
static int vsvtabDisconnect(sqlite3_vtab* pVtab) {
    VsvTable* p = (VsvTable*)pVtab;
    sqlite3_free(p->zFilename);
    sqlite3_free(p->zData);
    sqlite3_free(p);
    return SQLITE_OK;
}

/*
** Skip leading whitespace.  Return a pointer to the first non-whitespace
** character, or to the zero terminator if the string has only whitespace
*/
static const char* vsv_skip_whitespace(const char* z) {
    while (isspace((unsigned char)z[0])) {
        z++;
    }
    return z;
}

/*
** Remove trailing whitespace from the end of string z[]
*/
static void vsv_trim_whitespace(char* z) {
    size_t n = strlen(z);
    while (n > 0 && isspace((unsigned char)z[n])) {
        n--;
    }
    z[n] = 0;
}

/*
** Dequote the string
*/
static void vsv_dequote(char* z) {
    int j;
    char cQuote = z[0];
    size_t i, n;

    if (cQuote != '\'' && cQuote != '"') {
        return;
    }
    n = strlen(z);
    if (n < 2 || z[n - 1] != z[0]) {
        return;
    }
    for (i = 1, j = 0; i < n - 1; i++) {
        if (z[i] == cQuote && z[i + 1] == cQuote) {
            i++;
        }
        z[j++] = z[i];
    }
    z[j] = 0;
}

/*
** Check to see if the string is of the form:  "TAG = VALUE" with optional
** whitespace before and around tokens.  If it is, return a pointer to the
** first character of VALUE.  If it is not, return NULL.
*/
static const char* vsv_parameter(const char* zTag, int nTag, const char* z) {
    z = vsv_skip_whitespace(z);
    if (strncmp(zTag, z, nTag) != 0) {
        return 0;
    }
    z = vsv_skip_whitespace(z + nTag);
    if (z[0] != '=') {
        return 0;
    }
    return vsv_skip_whitespace(z + 1);
}

/*
** Decode a parameter that requires a dequoted string.
**
** Return 1 if the parameter is seen, or 0 if not.  1 is returned
** even if there is an error.  If an error occurs, then an error message
** is left in p->zErr.  If there are no errors, p->zErr[0]==0.
*/
static int vsv_string_parameter(VsvReader* p, /* Leave the error message here, if there is one */
                                const char* zParam, /* Parameter we are checking for */
                                const char* zArg,   /* Raw text of the virtual table argment */
                                char** pzVal        /* Write the dequoted string value here */
) {
    const char* zValue;
    zValue = vsv_parameter(zParam, (int)strlen(zParam), zArg);
    if (zValue == 0) {
        return 0;
    }
    p->zErr[0] = 0;
    if (*pzVal) {
        vsv_errmsg(p, "more than one '%s' parameter", zParam);
        return 1;
    }
    *pzVal = sqlite3_mprintf("%s", zValue);
    if (*pzVal == 0) {
        vsv_errmsg(p, "out of memory");
        return 1;
    }
    vsv_trim_whitespace(*pzVal);
    vsv_dequote(*pzVal);
    return 1;
}

/*
** Return 0 if the argument is false and 1 if it is true.  Return -1 if
** we cannot really tell.
*/
static int vsv_boolean(const char* z) {
    if (sqlite3_stricmp("yes", z) == 0 || sqlite3_stricmp("on", z) == 0 ||
        sqlite3_stricmp("true", z) == 0 || (z[0] == '1' && z[1] == 0)) {
        return 1;
    }
    if (sqlite3_stricmp("no", z) == 0 || sqlite3_stricmp("off", z) == 0 ||
        sqlite3_stricmp("false", z) == 0 || (z[0] == '0' && z[1] == 0)) {
        return 0;
    }
    return -1;
}

/*
** Check to see if the string is of the form:  "TAG = BOOLEAN" or just "TAG".
** If it is, set *pValue to be the value of the boolean ("true" if there is
** not "= BOOLEAN" component) and return non-zero.  If the input string
** does not begin with TAG, return zero.
*/
static int vsv_boolean_parameter(const char* zTag, /* Tag we are looking for */
                                 int nTag,         /* Size of the tag in bytes */
                                 const char* z,    /* Input parameter */
                                 int* pValue       /* Write boolean value here */
) {
    int b;
    z = vsv_skip_whitespace(z);
    if (strncmp(zTag, z, nTag) != 0) {
        return 0;
    }
    z = vsv_skip_whitespace(z + nTag);
    if (z[0] == 0) {
        *pValue = 1;
        return 1;
    }
    if (z[0] != '=') {
        return 0;
    }
    z = vsv_skip_whitespace(z + 1);
    b = vsv_boolean(z);
    if (b >= 0) {
        *pValue = b;
        return 1;
    }
    return 0;
}

/*
** Convert the seperator character specification into the character code
** Return 1 signifies error, 0 for no error
**
** Recognized inputs:
**      any single character
**      escaped characters \f \n \t \v
**      escaped hex byte   \x1e \x1f etc (RS and US respectively)
**
*/
static int vsv_parse_sep_char(char* in, int dflt, int* out) {
    if (!in) {
        *out = dflt;
        return 0;
    }
    switch (strlen(in)) {
        case 0: {
            *out = dflt;
            return 0;
        }
        case 1: {
            *out = in[0];
            return 0;
        }
        case 2: {
            if (in[0] != '\\') {
                return 1;
            }
            switch (in[1]) {
                case 'f': {
                    *out = 12;
                    return 0;
                }
                case 'n': {
                    *out = 10;
                    return 0;
                }
                case 't': {
                    *out = 9;
                    return 0;
                }
                case 'v': {
                    *out = 11;
                    return 0;
                }
            }
            return 1;
        }
        case 4: {
            if (sqlite3_strnicmp(in, "\\x", 2) != 0) {
                return 1;
            }
            if (!isxdigit(in[2]) || !isxdigit(in[3])) {
                return 1;
            }
            *out = ((in[2] > '9' ? (in[2] & 0x0f) + 9 : in[2] & 0x0f) << 4) +
                   (in[3] > '9' ? (in[3] & 0x0f) + 9 : in[3] & 0x0f);
            return 0;
        }
    }
    return 0;
}

/*
** Parameters:
**    filename=FILENAME          Name of file containing VSV content
**    data=TEXT                  Direct VSV content.
**    schema=SCHEMA              Alternative VSV schema.
**    header=YES|NO              First row of VSV defines the names of
**                               columns if "yes".  Default "no".
**    columns=N                  Assume the VSV file contains N columns.
**    fsep=FSET                  Field Seperator
**    rsep=RSEP                  Record Seperator
**    dsep=RSEP                  Decimal Seperator
**    skip=N                     skip N records of file (default 0)
**    affinity=AFF               affinity to apply to ALL columns
**                               default:  none
**                               none text integer real numeric
**
** If schema= is omitted, then the columns are named "c0", "c1", "c2",
** and so forth.  If columns=N is omitted, then the file is opened and
** the number of columns in the first row is counted to determine the
** column count.  If header=YES, then the first row is skipped.
*/
static int vsvtabConnect(sqlite3* db,
                         void* pAux,
                         int argc,
                         const char* const* argv,
                         sqlite3_vtab** ppVtab,
                         char** pzErr) {
    VsvTable* pNew = 0;    /* The VsvTable object to construct */
    int affinity = -1;     /* Affinity coercion */
    int bHeader = -1;      /* header= flags.  -1 means not seen yet */
    int validateUTF8 = -1; /* validateUTF8 flag */
    int rc = SQLITE_OK;    /* Result code from this routine */
    size_t i, j;           /* Loop counters */
    int b;                 /* Value of a boolean parameter */
    int nCol = -99;        /* Value of the columns= parameter */
    int nSkip = -1;        /* Value of the skip= parameter */
    int bNulls = -1;       /* Process Nulls flag */
    VsvReader sRdr;        /* A VSV file reader used to store an error
                            ** message and/or to count the number of columns */
    static const char* azParam[] = {"filename", "data", "schema", "fsep", "rsep", "dsep"};
    char* azPValue[6]; /* Parameter values */
#define VSV_FILENAME (azPValue[0])
#define VSV_DATA (azPValue[1])
#define VSV_SCHEMA (azPValue[2])
#define VSV_FSEP (azPValue[3])
#define VSV_RSEP (azPValue[4])
#define VSV_DSEP (azPValue[5])

    assert(sizeof(azPValue) == sizeof(azParam));
    memset(&sRdr, 0, sizeof(sRdr));
    memset(azPValue, 0, sizeof(azPValue));
    for (i = 3; i < (size_t)argc; i++) {
        const char* z = argv[i];
        const char* zValue;
        for (j = 0; j < sizeof(azParam) / sizeof(azParam[0]); j++) {
            if (vsv_string_parameter(&sRdr, azParam[j], z, &azPValue[j])) {
                break;
            }
        }
        if (j < sizeof(azParam) / sizeof(azParam[0])) {
            if (sRdr.zErr[0]) {
                goto vsvtab_connect_error;
            }
        } else if (vsv_boolean_parameter("header", 6, z, &b)) {
            if (bHeader >= 0) {
                vsv_errmsg(&sRdr, "more than one 'header' parameter");
                goto vsvtab_connect_error;
            }
            bHeader = b;
        } else if (vsv_boolean_parameter("validatetext", 12, z, &b)) {
            if (validateUTF8 >= 0) {
                vsv_errmsg(&sRdr, "more than one 'validatetext' parameter");
                goto vsvtab_connect_error;
            }
            validateUTF8 = b;
        } else if (vsv_boolean_parameter("nulls", 5, z, &b)) {
            if (bNulls >= 0) {
                vsv_errmsg(&sRdr, "more than one 'nulls' parameter");
                goto vsvtab_connect_error;
            }
            bNulls = b;
        } else if ((zValue = vsv_parameter("columns", 7, z)) != 0) {
            if (nCol > 0) {
                vsv_errmsg(&sRdr, "more than one 'columns' parameter");
                goto vsvtab_connect_error;
            }
            nCol = atoi(zValue);
            if (nCol <= 0) {
                vsv_errmsg(&sRdr, "column= value must be positive");
                goto vsvtab_connect_error;
            }
        } else if ((zValue = vsv_parameter("skip", 4, z)) != 0) {
            if (nSkip > 0) {
                vsv_errmsg(&sRdr, "more than one 'skip' parameter");
                goto vsvtab_connect_error;
            }
            nSkip = atoi(zValue);
            if (nSkip <= 0) {
                vsv_errmsg(&sRdr, "skip= value must be positive");
                goto vsvtab_connect_error;
            }
        } else if ((zValue = vsv_parameter("affinity", 8, z)) != 0) {
            if (affinity > -1) {
                vsv_errmsg(&sRdr, "more than one 'affinity' parameter");
                goto vsvtab_connect_error;
            }
            if (sqlite3_strnicmp(zValue, "none", 4) == 0)
                affinity = 0;
            else if (sqlite3_strnicmp(zValue, "blob", 4) == 0)
                affinity = 1;
            else if (sqlite3_strnicmp(zValue, "text", 4) == 0)
                affinity = 2;
            else if (sqlite3_strnicmp(zValue, "integer", 7) == 0)
                affinity = 3;
            else if (sqlite3_strnicmp(zValue, "real", 4) == 0)
                affinity = 4;
            else if (sqlite3_strnicmp(zValue, "numeric", 7) == 0)
                affinity = 5;
            else {
                vsv_errmsg(&sRdr, "unknown affinity: '%s'", zValue);
                goto vsvtab_connect_error;
            }
        } else {
            vsv_errmsg(&sRdr, "bad parameter: '%s'", z);
            goto vsvtab_connect_error;
        }
    }
    if (affinity == -1) {
        affinity = 0;
    }
    if (bNulls == -1) {
        bNulls = 0;
    }
    if (validateUTF8 == -1) {
        validateUTF8 = 0;
    }
    if ((VSV_FILENAME == 0) == (VSV_DATA == 0)) {
        vsv_errmsg(&sRdr, "must specify either filename= or data= but not both");
        goto vsvtab_connect_error;
    }
    if (vsv_parse_sep_char(VSV_FSEP, ',', &(sRdr.fsep))) {
        vsv_errmsg(&sRdr, "cannot parse fsep: '%s'", VSV_FSEP);
        goto vsvtab_connect_error;
    }
    if (vsv_parse_sep_char(VSV_RSEP, '\n', &(sRdr.rsep))) {
        vsv_errmsg(&sRdr, "cannot parse rsep: '%s'", VSV_RSEP);
        goto vsvtab_connect_error;
    }
    if (vsv_parse_sep_char(VSV_DSEP, '.', &(sRdr.dsep))) {
        vsv_errmsg(&sRdr, "cannot parse dsep: '%s'", VSV_DSEP);
        goto vsvtab_connect_error;
    }
    if ((nCol <= 0 || bHeader == 1) && vsv_reader_open(&sRdr, VSV_FILENAME, VSV_DATA)) {
        goto vsvtab_connect_error;
    }
    pNew = sqlite3_malloc(sizeof(*pNew));
    *ppVtab = (sqlite3_vtab*)pNew;
    if (pNew == 0) {
        goto vsvtab_connect_oom;
    }
    memset(pNew, 0, sizeof(*pNew));
    pNew->fsep = sRdr.fsep;
    pNew->rsep = sRdr.rsep;
    pNew->dsep = sRdr.dsep;
    pNew->affinity = affinity;
    pNew->validateUTF8 = validateUTF8;
    pNew->nulls = bNulls;
    if (VSV_SCHEMA == 0) {
        sqlite3_str* pStr = sqlite3_str_new(0);
        char* zSep = "";
        int iCol = 0;
        sqlite3_str_appendf(pStr, "CREATE TABLE x(");
        if (nCol < 0 && bHeader < 1) {
            nCol = 0;
            do {
                vsv_read_one_field(&sRdr);
                nCol++;
            } while (sRdr.cTerm == sRdr.fsep);
        }
        if (nCol > 0 && bHeader < 1) {
            for (iCol = 0; iCol < nCol; iCol++) {
                sqlite3_str_appendf(pStr, "%sc%d", zSep, iCol);
                zSep = ",";
            }
        } else {
            do {
                char* z = vsv_read_one_field(&sRdr);
                if ((nCol > 0 && iCol < nCol) || (nCol < 0 && bHeader)) {
                    sqlite3_str_appendf(pStr, "%s\"%w\"", zSep, z);
                    zSep = ",";
                    iCol++;
                }
            } while (sRdr.cTerm == sRdr.fsep);
            if (nCol < 0) {
                nCol = iCol;
            } else {
                while (iCol < nCol) {
                    sqlite3_str_appendf(pStr, "%sc%d", zSep, ++iCol);
                    zSep = ",";
                }
            }
        }
        sqlite3_str_appendf(pStr, ")");
        VSV_SCHEMA = sqlite3_str_finish(pStr);
        if (VSV_SCHEMA == 0) {
            goto vsvtab_connect_oom;
        }
    } else if (nCol < 0) {
        do {
            vsv_read_one_field(&sRdr);
            nCol++;
        } while (sRdr.cTerm == sRdr.fsep);
    } else if (nSkip < 1 && bHeader == 1) {
        do {
            vsv_read_one_field(&sRdr);
        } while (sRdr.cTerm == sRdr.fsep);
    }
    pNew->nCol = nCol;
    if (nSkip > 0) {
        int tskip = nSkip + (bHeader == 1);
        vsv_reader_reset(&sRdr);
        if (vsv_reader_open(&sRdr, VSV_FILENAME, VSV_DATA)) {
            goto vsvtab_connect_error;
        }
        do {
            do {
                if (!vsv_read_one_field(&sRdr))
                    goto vsvtab_connect_error;
            } while (sRdr.cTerm == sRdr.fsep);
            tskip--;
        } while (tskip > 0 && sRdr.cTerm == sRdr.rsep);
        if (tskip > 0) {
            vsv_errmsg(&sRdr, "premature end of file during skip");
            goto vsvtab_connect_error;
        }
    }
    pNew->zFilename = VSV_FILENAME;
    VSV_FILENAME = 0;
    pNew->zData = VSV_DATA;
    VSV_DATA = 0;
    if (bHeader != 1 && nSkip < 1) {
        pNew->iStart = 0;
    } else if (pNew->zData) {
        pNew->iStart = (int)sRdr.iIn;
    } else {
        pNew->iStart = (int)(ftell(sRdr.in) - sRdr.nIn + sRdr.iIn);
    }
    vsv_reader_reset(&sRdr);
    rc = sqlite3_declare_vtab(db, VSV_SCHEMA);
    if (rc) {
        vsv_errmsg(&sRdr, "bad schema: '%s' - %s", VSV_SCHEMA, sqlite3_errmsg(db));
        goto vsvtab_connect_error;
    }
    for (i = 0; i < sizeof(azPValue) / sizeof(azPValue[0]); i++) {
        sqlite3_free(azPValue[i]);
    }
    /*
    ** Rationale for DIRECTONLY:
    ** An attacker who controls a database schema could use this vtab
    ** to exfiltrate sensitive data from other files in the filesystem.
    ** And, recommended practice is to put all VSV virtual tables in the
    ** TEMP namespace, so they should still be usable from within TEMP
    ** views, so there shouldn't be a serious loss of functionality by
    ** prohibiting the use of this vtab from persistent triggers and views.
    */
    sqlite3_vtab_config(db, SQLITE_VTAB_DIRECTONLY);
    return SQLITE_OK;

vsvtab_connect_oom:
    rc = SQLITE_NOMEM;
    vsv_errmsg(&sRdr, "out of memory");

vsvtab_connect_error:
    if (pNew) {
        vsvtabDisconnect(&pNew->base);
    }
    for (i = 0; i < sizeof(azPValue) / sizeof(azPValue[0]); i++) {
        sqlite3_free(azPValue[i]);
    }
    if (sRdr.zErr[0]) {
        sqlite3_free(*pzErr);
        *pzErr = sqlite3_mprintf("%s", sRdr.zErr);
    }
    vsv_reader_reset(&sRdr);
    if (rc == SQLITE_OK) {
        rc = SQLITE_ERROR;
    }
    return rc;
}

/*
** Reset the current row content held by a VsvCursor.
*/
static void vsvtabCursorRowReset(VsvCursor* pCur) {
    VsvTable* pTab = (VsvTable*)pCur->base.pVtab;
    int i;
    for (i = 0; i < pTab->nCol; i++) {
        sqlite3_free(pCur->azVal[i]);
        pCur->azVal[i] = 0;
        pCur->aLen[i] = 0;
        pCur->dLen[i] = -1;
    }
}

/*
** The xConnect and xCreate methods do the same thing, but they must be
** different so that the virtual table is not an eponymous virtual table.
*/
static int vsvtabCreate(sqlite3* db,
                        void* pAux,
                        int argc,
                        const char* const* argv,
                        sqlite3_vtab** ppVtab,
                        char** pzErr) {
    return vsvtabConnect(db, pAux, argc, argv, ppVtab, pzErr);
}

/*
** Destructor for a VsvCursor.
*/
static int vsvtabClose(sqlite3_vtab_cursor* cur) {
    VsvCursor* pCur = (VsvCursor*)cur;
    vsvtabCursorRowReset(pCur);
    vsv_reader_reset(&pCur->rdr);
    sqlite3_free(cur);
    return SQLITE_OK;
}

/*
** Constructor for a new VsvTable cursor object.
*/
static int vsvtabOpen(sqlite3_vtab* p, sqlite3_vtab_cursor** ppCursor) {
    VsvTable* pTab = (VsvTable*)p;
    VsvCursor* pCur;
    size_t nByte;
    nByte = sizeof(*pCur) + (sizeof(char*) + (2 * sizeof(int))) * pTab->nCol;
    pCur = sqlite3_malloc64(nByte);
    if (pCur == 0)
        return SQLITE_NOMEM;
    memset(pCur, 0, nByte);
    pCur->azVal = (char**)&pCur[1];
    pCur->aLen = (int*)&pCur->azVal[pTab->nCol];
    pCur->dLen = (int*)&pCur->aLen[pTab->nCol];
    pCur->rdr.fsep = pTab->fsep;
    pCur->rdr.rsep = pTab->rsep;
    pCur->rdr.dsep = pTab->dsep;
    pCur->rdr.affinity = pTab->affinity;
    *ppCursor = &pCur->base;
    if (vsv_reader_open(&pCur->rdr, pTab->zFilename, pTab->zData)) {
        vsv_xfer_error(pTab, &pCur->rdr);
        return SQLITE_ERROR;
    }
    return SQLITE_OK;
}

/*
** Advance a VsvCursor to its next row of input.
** Set the EOF marker if we reach the end of input.
*/
static int vsvtabNext(sqlite3_vtab_cursor* cur) {
    VsvCursor* pCur = (VsvCursor*)cur;
    VsvTable* pTab = (VsvTable*)cur->pVtab;
    int i = 0;
    char* z;
    do {
        z = vsv_read_one_field(&pCur->rdr);
        if (z == 0) {
            if (i < pTab->nCol)
                pCur->dLen[i] = -1;
        } else if (i < pTab->nCol) {
            if (pCur->aLen[i] < pCur->rdr.n + 1) {
                char* zNew = sqlite3_realloc64(pCur->azVal[i], pCur->rdr.n + 1);
                if (zNew == 0) {
                    z = 0;
                    vsv_errmsg(&pCur->rdr, "out of memory");
                    vsv_xfer_error(pTab, &pCur->rdr);
                    break;
                }
                pCur->azVal[i] = zNew;
                pCur->aLen[i] = pCur->rdr.n + 1;
            }
            if (!pCur->rdr.notNull && pTab->nulls) {
                pCur->dLen[i] = -1;
            } else {
                pCur->dLen[i] = pCur->rdr.n;
                memcpy(pCur->azVal[i], z, pCur->rdr.n + 1);
            }
            i++;
        }
    } while (pCur->rdr.cTerm == pCur->rdr.fsep);
    if ((pCur->rdr.cTerm == EOF && i == 0)) {
        pCur->iRowid = -1;
    } else {
        pCur->iRowid++;
        while (i < pTab->nCol) {
            pCur->dLen[i] = -1;
            i++;
        }
    }
    return SQLITE_OK;
}

/*
**
** Determine affinity of field
**
** ignore leading space
** then may have + or -
** then may have digits or . (if . found then type=real)
** then may have digits (if another . then not number)
** then may have e (if found then type=real)
** then may have + or -
** then may have digits
** then may have trailing space
*/
static int vsv_isValidNumber(int dsep, char* arg) {
    char* start;
    char* stop;
    int isValid = 0;
    int hasDigit = 0;

    start = arg;
    stop = arg + strlen(arg) - 1;
    while (start <= stop && *start == ' ')  // strip spaces from begining
    {
        start++;
    }
    while (start <= stop && *stop == ' ')  // strip spaces from end
    {
        stop--;
    }
    if (start > stop) {
        goto vsv_end_isValidNumber;
    }
    if (start <= stop && (*start == '+' || *start == '-'))  // may have + or -
    {
        start++;
    }
    if (start <= stop && isdigit(*start))  // must have a digit to be valid
    {
        hasDigit = 1;
        isValid = 1;
    }
    while (start <= stop && isdigit(*start))  // bunch of digits
    {
        start++;
    }
    if (start <= stop && *start == dsep)  // may have decimal separator
    {
        isValid = 2;
        if (*start != '.') {
            *start = '.';
        }
        start++;
    }
    if (start <= stop && isdigit(*start)) {
        hasDigit = 1;
    }
    while (start <= stop && isdigit(*start))  // bunch of digits
    {
        start++;
    }
    if (!hasDigit)  // no digits then invalid
    {
        isValid = 0;
        goto vsv_end_isValidNumber;
    }
    if (start <= stop && (*start == 'e' || *start == 'E'))  // may have 'e' or 'E'
    {
        isValid = 3;
        start++;
    }
    if (start <= stop && isValid == 3 && (*start == '+' || *start == '-')) {
        start++;
    }
    if (start <= stop && isValid == 3 && isdigit(*start)) {
        isValid = 2;
    }
    while (start <= stop && isdigit(*start))  // bunch of digits
    {
        start++;
    }
    if (isValid == 3) {
        isValid = 0;
    }
vsv_end_isValidNumber:
    if (start <= stop) {
        isValid = 0;
    }
    return isValid;
}

/*
** Validate UTF-8
** Return -1 if invalid else length
*/
static long long vsv_utf8IsValid(char* string) {
    long long length = 0;
    unsigned char* start;
    int trailing = 0;
    unsigned char c;

    start = (unsigned char*)string;
    while ((c = *start)) {
        if (trailing) {
            if ((c & 0xC0) == 0x80) {
                trailing--;
                start++;
                length++;
                continue;
            } else {
                length = -1;
                break;
            }
        }
        if ((c & 0x80) == 0) {
            start++;
            length++;
            continue;
        }
        if ((c & 0xE0) == 0xC0) {
            trailing = 1;
            start++;
            length++;
            continue;
        }
        if ((c & 0xF0) == 0xE0) {
            trailing = 2;
            start++;
            length++;
            continue;
        }
        if ((c & 0xF8) == 0xF0) {
            trailing = 3;
            start++;
            length++;
            continue;
        }
        length = -1;
        break;
    }
    return length;
}

/*
** Return values of columns for the row at which the VsvCursor
** is currently pointing.
*/
static int vsvtabColumn(sqlite3_vtab_cursor* cur, /* The cursor */
                        sqlite3_context* ctx,     /* First argument to sqlite3_result_...() */
                        int i                     /* Which column to return */
) {
    VsvCursor* pCur = (VsvCursor*)cur;
    VsvTable* pTab = (VsvTable*)cur->pVtab;
    long long dLen = pCur->dLen[i];
    long long length = 0;

    if (i >= 0 && i < pTab->nCol && pCur->azVal[i] != 0 && dLen > -1) {
        switch (pTab->affinity) {
            case 0: {
                if (pTab->validateUTF8) {
                    length = vsv_utf8IsValid(pCur->azVal[i]);
                    if (length == dLen) {
                        sqlite3_result_text(ctx, pCur->azVal[i], dLen, SQLITE_TRANSIENT);
                    } else {
                        sqlite3_result_error(ctx, "Invalid UTF8 Data", -1);
                    }
                } else {
                    sqlite3_result_text(ctx, pCur->azVal[i], -1, SQLITE_TRANSIENT);
                }
                break;
            }
            case 1: {
                sqlite3_result_blob(ctx, pCur->azVal[i], dLen, SQLITE_TRANSIENT);
                break;
            }
            case 2: {
                if (pTab->validateUTF8) {
                    length = vsv_utf8IsValid(pCur->azVal[i]);
                    if (length < dLen) {
                        sqlite3_result_blob(ctx, pCur->azVal[i], dLen, SQLITE_TRANSIENT);
                    } else {
                        sqlite3_result_text(ctx, pCur->azVal[i], length, SQLITE_TRANSIENT);
                    }
                } else {
                    sqlite3_result_text(ctx, pCur->azVal[i], -1, SQLITE_TRANSIENT);
                }
                break;
            }
            case 3: {
                switch (vsv_isValidNumber(pCur->rdr.dsep, pCur->azVal[i])) {
                    case 1: {
                        sqlite3_result_int64(ctx, strtoll(pCur->azVal[i], 0, 10));
                        break;
                    }
                    default: {
                        if (pTab->validateUTF8) {
                            length = vsv_utf8IsValid(pCur->azVal[i]);
                            if (length < dLen) {
                                sqlite3_result_blob(ctx, pCur->azVal[i], dLen, SQLITE_TRANSIENT);
                            } else {
                                sqlite3_result_text(ctx, pCur->azVal[i], length, SQLITE_TRANSIENT);
                            }
                        } else {
                            sqlite3_result_text(ctx, pCur->azVal[i], -1, SQLITE_TRANSIENT);
                        }
                        break;
                    }
                }
                break;
            }
            case 4: {
                switch (vsv_isValidNumber(pCur->rdr.dsep, pCur->azVal[i])) {
                    case 1:
                    case 2: {
                        sqlite3_result_double(ctx, strtod(pCur->azVal[i], 0));
                        break;
                    }
                    default: {
                        if (pTab->validateUTF8) {
                            length = vsv_utf8IsValid(pCur->azVal[i]);
                            if (length < dLen) {
                                sqlite3_result_blob(ctx, pCur->azVal[i], dLen, SQLITE_TRANSIENT);
                            } else {
                                sqlite3_result_text(ctx, pCur->azVal[i], length, SQLITE_TRANSIENT);
                            }
                        } else {
                            sqlite3_result_text(ctx, pCur->azVal[i], -1, SQLITE_TRANSIENT);
                        }
                        break;
                    }
                }
                break;
            }
            case 5: {
                switch (vsv_isValidNumber(pCur->rdr.dsep, pCur->azVal[i])) {
                    case 1: {
                        sqlite3_result_int64(ctx, strtoll(pCur->azVal[i], 0, 10));
                        break;
                    }
                    case 2: {
                        long double dv, fp, ip;

                        dv = strtold(pCur->azVal[i], 0);
                        fp = modfl(dv, &ip);
                        if (sizeof(long double) > sizeof(double)) {
                            if (fp == 0.0L && dv >= -9223372036854775808.0L &&
                                dv <= 9223372036854775807.0L) {
                                sqlite3_result_int64(ctx, (long long)dv);
                            } else {
                                sqlite3_result_double(ctx, (double)dv);
                            }
                        } else {
                            // Only convert if it will fit in a 6-byte varint
                            if (fp == 0.0L && dv >= -140737488355328.0L &&
                                dv <= 140737488355328.0L) {
                                sqlite3_result_int64(ctx, (long long)dv);
                            } else {
                                sqlite3_result_double(ctx, (double)dv);
                            }
                        }
                        break;
                    }
                    default: {
                        if (pTab->validateUTF8) {
                            length = vsv_utf8IsValid(pCur->azVal[i]);
                            if (length < dLen) {
                                sqlite3_result_blob(ctx, pCur->azVal[i], dLen, SQLITE_TRANSIENT);
                            } else {
                                sqlite3_result_text(ctx, pCur->azVal[i], length, SQLITE_TRANSIENT);
                            }
                        } else {
                            sqlite3_result_text(ctx, pCur->azVal[i], -1, SQLITE_TRANSIENT);
                        }
                        break;
                    }
                }
            }
        }
    }
    return SQLITE_OK;
}

/*
** Return the rowid for the current row.
*/
static int vsvtabRowid(sqlite3_vtab_cursor* cur, sqlite_int64* pRowid) {
    VsvCursor* pCur = (VsvCursor*)cur;
    *pRowid = pCur->iRowid;
    return SQLITE_OK;
}

/*
** Return TRUE if the cursor has been moved off of the last
** row of output.
*/
static int vsvtabEof(sqlite3_vtab_cursor* cur) {
    VsvCursor* pCur = (VsvCursor*)cur;
    return pCur->iRowid < 0;
}

/*
** Only a full table scan is supported.  So xFilter simply rewinds to
** the beginning.
*/
static int vsvtabFilter(sqlite3_vtab_cursor* pVtabCursor,
                        int idxNum,
                        const char* idxStr,
                        int argc,
                        sqlite3_value** argv) {
    VsvCursor* pCur = (VsvCursor*)pVtabCursor;
    VsvTable* pTab = (VsvTable*)pVtabCursor->pVtab;
    pCur->iRowid = 0;
    if (pCur->rdr.in == 0) {
        assert(pCur->rdr.zIn == pTab->zData);
        assert(pTab->iStart >= 0);
        assert((size_t)pTab->iStart <= pCur->rdr.nIn);
        pCur->rdr.iIn = pTab->iStart;
    } else {
        fseek(pCur->rdr.in, pTab->iStart, SEEK_SET);
        pCur->rdr.iIn = 0;
        pCur->rdr.nIn = 0;
    }
    return vsvtabNext(pVtabCursor);
}

/*
** Only a forward full table scan is supported.  xBestIndex is mostly
** a no-op.
*/
static int vsvtabBestIndex(sqlite3_vtab* tab, sqlite3_index_info* pIdxInfo) {
    pIdxInfo->estimatedCost = 1000000;
    return SQLITE_OK;
}

static sqlite3_module vsv_module = {
    .xCreate = vsvtabCreate,
    .xConnect = vsvtabConnect,
    .xBestIndex = vsvtabBestIndex,
    .xDisconnect = vsvtabDisconnect,
    .xDestroy = vsvtabDisconnect,
    .xOpen = vsvtabOpen,
    .xClose = vsvtabClose,
    .xFilter = vsvtabFilter,
    .xNext = vsvtabNext,
    .xEof = vsvtabEof,
    .xColumn = vsvtabColumn,
    .xRowid = vsvtabRowid,
};

int vsv_init(sqlite3* db) {
    sqlite3_create_module(db, "vsv", &vsv_module, 0);
    return SQLITE_OK;
}

#endif // SQLEAN_ENABLE_VSV
#ifdef SQLEAN_ENABLE_DEFINE
// ---------------------------------
// src/define/eval.c
// ---------------------------------
// Created by by D. Richard Hipp, Public Domain
// https://www.sqlite.org/src/file/ext/misc/eval.c

// Modified by Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean/

// Evaluate dynamic SQL.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

SQLITE_EXTENSION_INIT3

/*
 * Structure used to accumulate the output
 */
struct EvalResult {
    char* z;              /* Accumulated output */
    const char* zSep;     /* Separator */
    int szSep;            /* Size of the separator string */
    sqlite3_int64 nAlloc; /* Number of bytes allocated for z[] */
    sqlite3_int64 nUsed;  /* Number of bytes of z[] actually used */
};

/*
 * Callback from sqlite_exec() for the eval() function.
 */
static int eval_callback(void* pCtx, int argc, char** argv, char** colnames) {
    struct EvalResult* p = (struct EvalResult*)pCtx;
    int i;
    if (argv == 0) {
        return SQLITE_OK;
    }
    for (i = 0; i < argc; i++) {
        const char* z = argv[i] ? argv[i] : "";
        size_t sz = strlen(z);
        if ((sqlite3_int64)sz + p->nUsed + p->szSep + 1 > p->nAlloc) {
            char* zNew;
            p->nAlloc = p->nAlloc * 2 + sz + p->szSep + 1;
            /* Using sqlite3_realloc64() would be better, but it is a recent
            ** addition and will cause a segfault if loaded by an older version
            ** of SQLite.  */
            zNew = p->nAlloc <= 0x7fffffff ? sqlite3_realloc64(p->z, p->nAlloc) : 0;
            if (zNew == 0) {
                sqlite3_free(p->z);
                memset(p, 0, sizeof(*p));
                return SQLITE_NOMEM;
            }
            p->z = zNew;
        }
        if (p->nUsed > 0) {
            memcpy(&p->z[p->nUsed], p->zSep, p->szSep);
            p->nUsed += p->szSep;
        }
        memcpy(&p->z[p->nUsed], z, sz);
        p->nUsed += sz;
    }
    return SQLITE_OK;
}

/*
 * Implementation of the eval(X) and eval(X,Y) SQL functions.
 *
 * Evaluate the SQL text in X. Return the results, using string
 * Y as the separator. If Y is omitted, use a single space character.
 */
static void define_eval(sqlite3_context* context, int argc, sqlite3_value** argv) {
    const char* zSql;
    sqlite3* db;
    char* zErr = 0;
    int rc;
    struct EvalResult x;

    memset(&x, 0, sizeof(x));
    x.zSep = " ";
    zSql = (const char*)sqlite3_value_text(argv[0]);
    if (zSql == 0) {
        return;
    }
    if (argc > 1) {
        x.zSep = (const char*)sqlite3_value_text(argv[1]);
        if (x.zSep == 0) {
            return;
        }
    }
    x.szSep = (int)strlen(x.zSep);
    db = sqlite3_context_db_handle(context);
    rc = sqlite3_exec(db, zSql, eval_callback, &x, &zErr);
    if (rc != SQLITE_OK) {
        sqlite3_result_error(context, zErr, -1);
        sqlite3_free(zErr);
    } else if (x.zSep == 0) {
        sqlite3_result_error_nomem(context);
        sqlite3_free(x.z);
    } else {
        sqlite3_result_text(context, x.z, (int)x.nUsed, sqlite3_free);
    }
}

int define_eval_init(sqlite3* db) {
    const int flags = SQLITE_UTF8 | SQLITE_DIRECTONLY;
    sqlite3_create_function(db, "eval", 1, flags, NULL, define_eval, NULL, NULL);
    sqlite3_create_function(db, "eval", 2, flags, NULL, define_eval, NULL, NULL);
    return SQLITE_OK;
}

// ---------------------------------
// src/define/extension.c
// ---------------------------------
// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// User-defined functions in SQLite.

SQLITE_EXTENSION_INIT3


int define_init(sqlite3* db) {
    int status = define_manage_init(db);
    define_eval_init(db);
    define_module_init(db);
    return status;
}

// ---------------------------------
// src/define/manage.c
// ---------------------------------
// Copyright (c) 2022 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Manage defined functions.

#include <stdio.h>
#include <stdlib.h>

SQLITE_EXTENSION_INIT3

#define DEFINE_CACHE 2

#pragma region statement cache

typedef struct cache_node {
    sqlite3_stmt* stmt;
    struct cache_node* next;
} cache_node;

static cache_node* cache_head = NULL;
static cache_node* cache_tail = NULL;

static int cache_add(sqlite3_stmt* stmt) {
    if (cache_head == NULL) {
        cache_head = (cache_node*)malloc(sizeof(cache_node));
        if (cache_head == NULL) {
            return SQLITE_ERROR;
        }
        cache_head->stmt = stmt;
        cache_head->next = NULL;
        cache_tail = cache_head;
        return SQLITE_OK;
    }
    cache_tail->next = (cache_node*)malloc(sizeof(cache_node));
    if (cache_tail->next == NULL) {
        return SQLITE_ERROR;
    }
    cache_tail = cache_tail->next;
    cache_tail->stmt = stmt;
    cache_tail->next = NULL;
    return SQLITE_OK;
}

static void cache_print() {
    if (cache_head == NULL) {
        printf("cache is empty");
        return;
    }
    cache_node* curr = cache_head;
    while (curr != NULL) {
        printf("%s\n", sqlite3_sql(curr->stmt));
        curr = curr->next;
    }
}

static void cache_free() {
    if (cache_head == NULL) {
        return;
    }
    cache_node* prev;
    cache_node* curr = cache_head;
    while (curr != NULL) {
        sqlite3_finalize(curr->stmt);
        prev = curr;
        curr = curr->next;
        free(prev);
    }
    cache_head = cache_tail = NULL;
}

/*
 * Prints prepared statements cache contents.
 */
static void define_cache(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    cache_print();
}

#pragma endregion

/*
 * Saves user-defined function into the database.
 */
int define_save_function(sqlite3* db, const char* name, const char* type, const char* body) {
    char* sql =
        "insert into sqlean_define(name, type, body) values (?, ?, ?) "
        "on conflict do nothing";
    sqlite3_stmt* stmt;
    int ret = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (ret != SQLITE_OK) {
        return ret;
    }
    sqlite3_bind_text(stmt, 1, name, -1, NULL);
    sqlite3_bind_text(stmt, 2, type, -1, NULL);
    sqlite3_bind_text(stmt, 3, body, -1, NULL);
    ret = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (ret != SQLITE_DONE) {
        return ret;
    }
    return SQLITE_OK;
}

// no cache at all
#if DEFINE_CACHE == 0

/*
 * Executes user-defined sql from the context.
 */
static void define_exec(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    int ret = SQLITE_OK;
    char* sql = sqlite3_user_data(ctx);
    sqlite3_stmt* stmt;
    // sqlite3_close requires all prepared statements to be closed before destroying functions, so
    // we have to re-create this every call
    if ((ret = sqlite3_prepare_v2(sqlite3_context_db_handle(ctx), sql, -1, &stmt, NULL)) !=
        SQLITE_OK) {
        sqlite3_result_error_code(ctx, ret);
        return;
    }
    for (int i = 0; i < argc; i++)
        if ((ret = sqlite3_bind_value(stmt, i + 1, argv[i])) != SQLITE_OK)
            goto end;
    if ((ret = sqlite3_step(stmt)) != SQLITE_ROW) {
        if (ret == SQLITE_DONE)
            ret = SQLITE_MISUSE;
        goto end;
    }
    sqlite3_result_value(ctx, sqlite3_column_value(stmt, 0));

end:
    sqlite3_finalize(stmt);
    if (ret != SQLITE_ROW)
        sqlite3_result_error_code(ctx, ret);
}

/*
 * Creates user-defined function without caching the prepared statement.
 */
static int define_create(sqlite3* db, const char* name, const char* body) {
    char* sql = sqlite3_mprintf("select %s", body);
    if (!sql) {
        return SQLITE_NOMEM;
    }

    sqlite3_stmt* stmt;
    int ret = sqlite3_prepare_v3(db, sql, -1, SQLITE_PREPARE_PERSISTENT, &stmt, NULL);
    if (ret != SQLITE_OK) {
        sqlite3_free(sql);
        return ret;
    }
    int nparams = sqlite3_bind_parameter_count(stmt);
    sqlite3_finalize(stmt);

    return sqlite3_create_function_v2(db, name, nparams, SQLITE_UTF8, sql, define_exec, NULL, NULL,
                                      sqlite3_free);
}

/*
 * Creates user-defined function and saves it to the database.
 */
static void define_function(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    sqlite3* db = sqlite3_context_db_handle(ctx);
    const char* name = (const char*)sqlite3_value_text(argv[0]);
    const char* body = (const char*)sqlite3_value_text(argv[1]);
    int ret;
    if ((ret = define_create(db, name, body)) != SQLITE_OK) {
        sqlite3_result_error_code(ctx, ret);
        return;
    }
    if ((ret = define_save_function(db, name, "scalar", body)) != SQLITE_OK) {
        sqlite3_result_error_code(ctx, ret);
        return;
    }
}

/*
 * No-op as nothing is cached.
 */
static void define_free(sqlite3_context* ctx, int argc, sqlite3_value** argv) {}

// custom cache
#elif DEFINE_CACHE == 2

/*
 * Executes compiled prepared statement from the context.
 */
static void define_exec(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    int ret = SQLITE_OK;
    sqlite3_stmt* stmt = sqlite3_user_data(ctx);
    for (int i = 0; i < argc; i++) {
        if ((ret = sqlite3_bind_value(stmt, i + 1, argv[i])) != SQLITE_OK) {
            sqlite3_reset(stmt);
            sqlite3_result_error_code(ctx, ret);
            return;
        }
    }
    if ((ret = sqlite3_step(stmt)) != SQLITE_ROW) {
        if (ret == SQLITE_DONE) {
            ret = SQLITE_MISUSE;
        }
        sqlite3_reset(stmt);
        sqlite3_result_error_code(ctx, ret);
        return;
    }
    sqlite3_result_value(ctx, sqlite3_column_value(stmt, 0));
    sqlite3_reset(stmt);
}

/*
 * Creates user-defined function and caches the prepared statement.
 */
static int define_create(sqlite3* db, const char* name, const char* body) {
    char* sql = sqlite3_mprintf("select %s", body);
    if (!sql) {
        return SQLITE_NOMEM;
    }

    sqlite3_stmt* stmt;
    int ret = sqlite3_prepare_v3(db, sql, -1, SQLITE_PREPARE_PERSISTENT, &stmt, NULL);
    sqlite3_free(sql);
    if (ret != SQLITE_OK) {
        return ret;
    }
    int nparams = sqlite3_bind_parameter_count(stmt);
    // We are going to cache the statement in the function constructor and retrieve it later
    // when executing the function, using sqlite3_user_data(). But relying on this internal cache
    // is not enough.
    //
    // SQLite requires all prepared statements to be closed before calling the function destructor
    // when closing the connection. So we can't close the statement in the function destructor.
    // We have to cache it in the external cache and ask the user to manually free it
    // before closing the connection.
    //
    // Alternatively, we can cache via the sqlite3_set_auxdata() with a negative slot,
    // but that seems rather hacky.
    if ((ret = cache_add(stmt)) != SQLITE_OK) {
        return ret;
    }

    return sqlite3_create_function(db, name, nparams, SQLITE_UTF8, stmt, define_exec, NULL, NULL);
}

/*
 * Creates compiled user-defined function and saves it to the database.
 */
static void define_function(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    sqlite3* db = sqlite3_context_db_handle(ctx);
    const char* name = (const char*)sqlite3_value_text(argv[0]);
    const char* body = (const char*)sqlite3_value_text(argv[1]);
    int ret;
    if ((ret = define_create(db, name, body)) != SQLITE_OK) {
        sqlite3_result_error_code(ctx, ret);
        return;
    }
    if ((ret = define_save_function(db, name, "scalar", body)) != SQLITE_OK) {
        sqlite3_result_error_code(ctx, ret);
        return;
    }
}

/*
 * Frees prepared statements compiled by user-defined functions.
 */
static void define_free(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    cache_free();
}

#endif  // DEFINE_CACHE

/*
 * Deletes user-defined function (scalar or table-valued)
 */
static void define_undefine(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    char* template =
        "delete from sqlean_define where name = '%s';"
        "drop table if exists \"%s\";";
    const char* name = (const char*)sqlite3_value_text(argv[0]);
    char* sql = sqlite3_mprintf(template, name, name);
    if (!sql) {
        sqlite3_result_error_code(ctx, SQLITE_NOMEM);
        return;
    }

    sqlite3* db = sqlite3_context_db_handle(ctx);
    int ret = sqlite3_exec(db, sql, NULL, NULL, NULL);
    if (ret != SQLITE_OK) {
        sqlite3_result_error_code(ctx, ret);
    }
    sqlite3_free(sql);
}

/*
 * Loads user-defined functions from the database.
 */
static int define_load(sqlite3* db) {
    char* sql =
        "create table if not exists sqlean_define"
        "(name text primary key, type text, body text)";
    int ret = sqlite3_exec(db, sql, NULL, NULL, NULL);
    if (ret != SQLITE_OK) {
        return ret;
    }

    sqlite3_stmt* stmt;
    sql = "select name, body from sqlean_define where type = 'scalar'";
    if ((ret = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL)) != SQLITE_OK) {
        return ret;
    }

    const char* name;
    const char* body;
    while (sqlite3_step(stmt) != SQLITE_DONE) {
        name = (const char*)sqlite3_column_text(stmt, 0);
        body = (const char*)sqlite3_column_text(stmt, 1);
        ret = define_create(db, name, body);
        if (ret != SQLITE_OK) {
            break;
        }
    }
    return sqlite3_finalize(stmt);
}

int define_manage_init(sqlite3* db) {
    const int flags = SQLITE_UTF8 | SQLITE_DIRECTONLY;
    sqlite3_create_function(db, "define", 2, flags, NULL, define_function, NULL, NULL);
    sqlite3_create_function(db, "define_free", 0, flags, NULL, define_free, NULL, NULL);
    sqlite3_create_function(db, "define_cache", 0, flags, NULL, define_cache, NULL, NULL);
    sqlite3_create_function(db, "undefine", 1, flags, NULL, define_undefine, NULL, NULL);
    return define_load(db);
}

// ---------------------------------
// src/define/module.c
// ---------------------------------
// Created by 0x09, Public Domain
// https://github.com/0x09/sqlite-statement-vtab/blob/master/statement_vtab.c

// Modified by Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean/

// Define table-valued functions.

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

SQLITE_EXTENSION_INIT3


struct define_vtab {
    sqlite3_vtab base;
    sqlite3* db;
    char* sql;
    size_t sql_len;
    int num_inputs;
    int num_outputs;
};

struct define_cursor {
    sqlite3_vtab_cursor base;
    sqlite3_stmt* stmt;
    int rowid;
    int param_argc;
    sqlite3_value** param_argv;
};

static char* build_create_statement(sqlite3_stmt* stmt) {
    sqlite3_str* sql = sqlite3_str_new(NULL);
    sqlite3_str_appendall(sql, "CREATE TABLE x( ");
    for (int i = 0, nout = sqlite3_column_count(stmt); i < nout; i++) {
        const char* name = sqlite3_column_name(stmt, i);
        if (!name) {
            sqlite3_free(sqlite3_str_finish(sql));
            return NULL;
        }
        const char* type = sqlite3_column_decltype(stmt, i);
        sqlite3_str_appendf(sql, "%Q %s,", name, (type ? type : ""));
    }
    for (int i = 0, nargs = sqlite3_bind_parameter_count(stmt); i < nargs; i++) {
        const char* name = sqlite3_bind_parameter_name(stmt, i + 1);
        if (name)
            sqlite3_str_appendf(sql, "%Q hidden,", name + 1);
        else
            sqlite3_str_appendf(sql, "'%d' hidden,", i + 1);
    }
    if (sqlite3_str_length(sql))
        sqlite3_str_value(sql)[sqlite3_str_length(sql) - 1] = ')';
    return sqlite3_str_finish(sql);
}

static int define_vtab_destroy(sqlite3_vtab* pVTab) {
    sqlite3_free(((struct define_vtab*)pVTab)->sql);
    sqlite3_free(pVTab);
    return SQLITE_OK;
}

static int define_vtab_create(sqlite3* db,
                              void* pAux,
                              int argc,
                              const char* const* argv,
                              sqlite3_vtab** ppVtab,
                              char** pzErr) {
    size_t len;
    if (argc < 4 || (len = strlen(argv[3])) < 3) {
        if (!(*pzErr = sqlite3_mprintf("no statement provided")))
            return SQLITE_NOMEM;
        return SQLITE_MISUSE;
    }
    if (argv[3][0] != '(' || argv[3][len - 1] != ')') {
        if (!(*pzErr = sqlite3_mprintf("statement must be parenthesized")))
            return SQLITE_NOMEM;
        return SQLITE_MISUSE;
    }

    int ret;
    sqlite3_stmt* stmt = NULL;
    char* create = NULL;

    struct define_vtab* vtab = sqlite3_malloc64(sizeof(*vtab));
    if (!vtab) {
        return SQLITE_NOMEM;
    }
    memset(vtab, 0, sizeof(*vtab));
    *ppVtab = &vtab->base;

    vtab->db = db;
    vtab->sql_len = len - 2;
    if (!(vtab->sql = sqlite3_mprintf("%.*s", vtab->sql_len, argv[3] + 1))) {
        ret = SQLITE_NOMEM;
        goto error;
    }

    ret = sqlite3_prepare_v2(db, vtab->sql, vtab->sql_len, &stmt, NULL);
    if (ret != SQLITE_OK) {
        goto sqlite_error;
    }

    if (!sqlite3_stmt_readonly(stmt)) {
        ret = SQLITE_ERROR;
        if (!(*pzErr = sqlite3_mprintf("Statement must be read only.")))
            ret = SQLITE_NOMEM;
        goto error;
    }

    vtab->num_inputs = sqlite3_bind_parameter_count(stmt);
    vtab->num_outputs = sqlite3_column_count(stmt);

    if (!(create = build_create_statement(stmt))) {
        ret = SQLITE_NOMEM;
        goto error;
    }

    if ((ret = sqlite3_declare_vtab(db, create)) != SQLITE_OK) {
        goto sqlite_error;
    }

    if ((ret = define_save_function(db, argv[2], "table", argv[3])) != SQLITE_OK) {
        goto error;
    }

    sqlite3_free(create);
    sqlite3_finalize(stmt);
    return SQLITE_OK;

sqlite_error:
    if (!(*pzErr = sqlite3_mprintf("%s", sqlite3_errmsg(db))))
        ret = SQLITE_NOMEM;
error:
    sqlite3_free(create);
    sqlite3_finalize(stmt);
    define_vtab_destroy(*ppVtab);
    *ppVtab = NULL;
    return ret;
}

// if these point to the literal same function sqlite makes define_vtab eponymous, which we don't
// want
static int define_vtab_connect(sqlite3* db,
                               void* pAux,
                               int argc,
                               const char* const* argv,
                               sqlite3_vtab** ppVtab,
                               char** pzErr) {
    return define_vtab_create(db, pAux, argc, argv, ppVtab, pzErr);
}

static int define_vtab_open(sqlite3_vtab* pVTab, sqlite3_vtab_cursor** ppCursor) {
    struct define_vtab* vtab = (struct define_vtab*)pVTab;
    struct define_cursor* cur = sqlite3_malloc64(sizeof(*cur));
    if (!cur)
        return SQLITE_NOMEM;

    *ppCursor = &cur->base;
    cur->param_argv = sqlite3_malloc(sizeof(*cur->param_argv) * vtab->num_inputs);
    return sqlite3_prepare_v2(vtab->db, vtab->sql, vtab->sql_len, &cur->stmt, NULL);
}

static int define_vtab_close(sqlite3_vtab_cursor* cur) {
    struct define_cursor* stmtcur = (struct define_cursor*)cur;
    sqlite3_finalize(stmtcur->stmt);
    sqlite3_free(stmtcur->param_argv);
    sqlite3_free(cur);
    return SQLITE_OK;
}

static int define_vtab_next(sqlite3_vtab_cursor* cur) {
    struct define_cursor* stmtcur = (struct define_cursor*)cur;
    int ret = sqlite3_step(stmtcur->stmt);
    if (ret == SQLITE_ROW) {
        stmtcur->rowid++;
        return SQLITE_OK;
    }
    return ret == SQLITE_DONE ? SQLITE_OK : ret;
}

static int define_vtab_rowid(sqlite3_vtab_cursor* cur, sqlite_int64* pRowid) {
    *pRowid = ((struct define_cursor*)cur)->rowid;
    return SQLITE_OK;
}

static int define_vtab_eof(sqlite3_vtab_cursor* cur) {
    return !sqlite3_stmt_busy(((struct define_cursor*)cur)->stmt);
}

static int define_vtab_column(sqlite3_vtab_cursor* cur, sqlite3_context* ctx, int i) {
    struct define_cursor* stmtcur = (struct define_cursor*)cur;
    int num_outputs = ((struct define_vtab*)cur->pVtab)->num_outputs;
    if (i < num_outputs)
        sqlite3_result_value(ctx, sqlite3_column_value(stmtcur->stmt, i));
    else if (i - num_outputs < stmtcur->param_argc)
        sqlite3_result_value(ctx, stmtcur->param_argv[i - num_outputs]);
    return SQLITE_OK;
}

// xBestIndex needs to communicate which columns are constrained by the where clause to xFilter;
// in terms of a statement table this translates to which parameters will be available to bind.
static int define_vtab_filter(sqlite3_vtab_cursor* cur,
                              int idxNum,
                              const char* idxStr,
                              int argc,
                              sqlite3_value** argv) {
    struct define_cursor* stmtcur = (struct define_cursor*)cur;
    stmtcur->rowid = 1;
    sqlite3_stmt* stmt = stmtcur->stmt;
    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);

    int ret;
    for (int i = 0; i < argc; i++)
        if ((ret = sqlite3_bind_value(stmt, idxStr ? ((int*)idxStr)[i] : i + 1, argv[i])) !=
            SQLITE_OK)
            return ret;
    ret = sqlite3_step(stmt);
    if (!(ret == SQLITE_ROW || ret == SQLITE_DONE))
        return ret;

    assert(((struct define_vtab*)cur->pVtab)->num_inputs >= argc);
    if ((stmtcur->param_argc = argc))  // these seem to persist for the remainder of the statement,
                                       // so just shallow copy
        memcpy(stmtcur->param_argv, argv, sizeof(*stmtcur->param_argv) * argc);

    return SQLITE_OK;
}

static int define_vtab_best_index(sqlite3_vtab* pVTab, sqlite3_index_info* index_info) {
    int num_outputs = ((struct define_vtab*)pVTab)->num_outputs;
    int out_constraints = 0;
    index_info->orderByConsumed = 0;
    index_info->estimatedCost = 1;
    index_info->estimatedRows = 1;
    int col_max = 0;
    sqlite3_uint64 used_cols = 0;
    for (int i = 0; i < index_info->nConstraint; i++) {
        // skip if this is a constraint on one of our output columns
        if (index_info->aConstraint[i].iColumn < num_outputs)
            continue;
        // a given query plan is only usable if all provided "input" columns are usable and have
        // equal constraints only is this redundant / an EQ constraint ever unusable?
        if (!index_info->aConstraint[i].usable ||
            index_info->aConstraint[i].op != SQLITE_INDEX_CONSTRAINT_EQ)
            return SQLITE_CONSTRAINT;

        int col_index = index_info->aConstraint[i].iColumn - num_outputs;
        index_info->aConstraintUsage[i].argvIndex = col_index + 1;
        index_info->aConstraintUsage[i].omit = 1;

        if (col_index + 1 > col_max)
            col_max = col_index + 1;
        if (col_index < 64)
            used_cols |= 1ull << col_index;

        out_constraints++;
    }

    // if the constrained columns are contiguous then we can just tell sqlite to order the arg
    // vector provided to xFilter in the same order as our column bindings, so there's no need to
    // map between these (this will always be the case when calling the vtab as a table-valued
    // function) only support this optimization for up to 64 constrained columns since checking for
    // continuity more generally would cost as much as just allocating the mapping
    sqlite_uint64 required_cols = (col_max < 64 ? 1ull << col_max : 0ull) - 1;
    if (!out_constraints || (col_max <= 64 && used_cols == required_cols))
        return SQLITE_OK;

    // otherwise map the constraint index as provided to xFilter to column index for bindings
    // if this is sparse e.g. where arg1 = x and arg3 = y then we store this separately in idxStr
    int* colmap = sqlite3_malloc64(sizeof(*colmap) * out_constraints);
    if (!colmap)
        return SQLITE_NOMEM;

    int argc = 0;
    int old_index;
    for (int i = 0; i < index_info->nConstraint; i++)
        if ((old_index = index_info->aConstraintUsage[i].argvIndex)) {
            colmap[argc] = old_index;
            index_info->aConstraintUsage[i].argvIndex = ++argc;
        }

    index_info->idxStr = (char*)colmap;
    index_info->needToFreeIdxStr = 1;

    return SQLITE_OK;
}

static sqlite3_module define_module = {
    .xCreate = define_vtab_create,
    .xConnect = define_vtab_connect,
    .xBestIndex = define_vtab_best_index,
    .xDisconnect = define_vtab_destroy,
    .xDestroy = define_vtab_destroy,
    .xOpen = define_vtab_open,
    .xClose = define_vtab_close,
    .xFilter = define_vtab_filter,
    .xNext = define_vtab_next,
    .xEof = define_vtab_eof,
    .xColumn = define_vtab_column,
    .xRowid = define_vtab_rowid,
};

int define_module_init(sqlite3* db) {
    sqlite3_create_module(db, "define", &define_module, NULL);
    return SQLITE_OK;
}

#endif // SQLEAN_ENABLE_DEFINE
#ifdef SQLEAN_ENABLE_MATH
// ---------------------------------
// src/math/extension.c
// ---------------------------------
// Originally from SQLite 3.42.0 source code (func.c), Public Domain

// Modified by Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean/

// SQLite math functions.

#include <assert.h>
#include <math.h>

SQLITE_EXTENSION_INIT3

#if defined(HAVE_STDINT_H) /* Use this case if we have ANSI headers */
#define SQLITE_INT_TO_PTR(X) ((void*)(intptr_t)(X))
#define SQLITE_PTR_TO_INT(X) ((int)(intptr_t)(X))
#elif defined(__PTRDIFF_TYPE__) /* This case should work for GCC */
#define SQLITE_INT_TO_PTR(X) ((void*)(__PTRDIFF_TYPE__)(X))
#define SQLITE_PTR_TO_INT(X) ((int)(__PTRDIFF_TYPE__)(X))
#elif !defined(__GNUC__) /* Works for compilers other than LLVM */
#define SQLITE_INT_TO_PTR(X) ((void*)&((char*)0)[X])
#define SQLITE_PTR_TO_INT(X) ((int)(((char*)X) - (char*)0))
#else /* Generates a warning - but it always works */
#define SQLITE_INT_TO_PTR(X) ((void*)(X))
#define SQLITE_PTR_TO_INT(X) ((int)(X))
#endif

/* Mathematical Constants */
#ifndef M_PI
#define M_PI 3.141592653589793238462643383279502884
#endif
#ifndef M_LN10
#define M_LN10 2.302585092994045684017991454684364208
#endif
#ifndef M_LN2
#define M_LN2 0.693147180559945309417232121458176568
#endif

/*
** Implementation SQL functions:
**
**   ceil(X)
**   ceiling(X)
**   floor(X)
**
** The sqlite3_user_data() pointer is a pointer to the libm implementation
** of the underlying C function.
*/
static void ceilingFunc(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    switch (sqlite3_value_numeric_type(argv[0])) {
        case SQLITE_INTEGER: {
            sqlite3_result_int64(context, sqlite3_value_int64(argv[0]));
            break;
        }
        case SQLITE_FLOAT: {
            double (*x)(double) = (double (*)(double))sqlite3_user_data(context);
            sqlite3_result_double(context, x(sqlite3_value_double(argv[0])));
            break;
        }
        default: {
            break;
        }
    }
}

/*
** On some systems, ceil() and floor() are intrinsic function.  You are
** unable to take a pointer to these functions.  Hence, we here wrap them
** in our own actual functions.
*/
static double xCeil(double x) {
    return ceil(x);
}
static double xFloor(double x) {
    return floor(x);
}

/*
** Some systems do not have log2() and log10() in their standard math
** libraries.
*/
#if defined(HAVE_LOG10) && HAVE_LOG10 == 0
#define log10(X) (0.4342944819032517867 * log(X))
#endif
#if defined(HAVE_LOG2) && HAVE_LOG2 == 0
#define log2(X) (1.442695040888963456 * log(X))
#endif

/*
** Implementation of SQL functions:
**
**   ln(X)       - natural logarithm
**   log(X)      - log X base 10
**   log10(X)    - log X base 10
**   log(B,X)    - log X base B
*/
static void logFunc(sqlite3_context* context, int argc, sqlite3_value** argv) {
    double x, b, ans;
    assert(argc == 1 || argc == 2);
    switch (sqlite3_value_numeric_type(argv[0])) {
        case SQLITE_INTEGER:
        case SQLITE_FLOAT:
            x = sqlite3_value_double(argv[0]);
            if (x <= 0.0)
                return;
            break;
        default:
            return;
    }
    if (argc == 2) {
        switch (sqlite3_value_numeric_type(argv[0])) {
            case SQLITE_INTEGER:
            case SQLITE_FLOAT:
                b = log(x);
                if (b <= 0.0)
                    return;
                x = sqlite3_value_double(argv[1]);
                if (x <= 0.0)
                    return;
                break;
            default:
                return;
        }
        ans = log(x) / b;
    } else {
        switch (SQLITE_PTR_TO_INT(sqlite3_user_data(context))) {
            case 1:
                ans = log10(x);
                break;
            case 2:
                ans = log2(x);
                break;
            default:
                ans = log(x);
                break;
        }
    }
    sqlite3_result_double(context, ans);
}

/*
** Functions to converts degrees to radians and radians to degrees.
*/
static double degToRad(double x) {
    return x * (M_PI / 180.0);
}
static double radToDeg(double x) {
    return x * (180.0 / M_PI);
}

/*
** Implementation of 1-argument SQL math functions:
**
**   exp(X)  - Compute e to the X-th power
*/
static void math1Func(sqlite3_context* context, int argc, sqlite3_value** argv) {
    int type0;
    double v0, ans;
    double (*x)(double);
    assert(argc == 1);
    type0 = sqlite3_value_numeric_type(argv[0]);
    if (type0 != SQLITE_INTEGER && type0 != SQLITE_FLOAT)
        return;
    v0 = sqlite3_value_double(argv[0]);
    x = (double (*)(double))sqlite3_user_data(context);
    ans = x(v0);
    sqlite3_result_double(context, ans);
}

/*
** Implementation of 2-argument SQL math functions:
**
**   power(X,Y)  - Compute X to the Y-th power
*/
static void math2Func(sqlite3_context* context, int argc, sqlite3_value** argv) {
    int type0, type1;
    double v0, v1, ans;
    double (*x)(double, double);
    assert(argc == 2);
    type0 = sqlite3_value_numeric_type(argv[0]);
    if (type0 != SQLITE_INTEGER && type0 != SQLITE_FLOAT)
        return;
    type1 = sqlite3_value_numeric_type(argv[1]);
    if (type1 != SQLITE_INTEGER && type1 != SQLITE_FLOAT)
        return;
    v0 = sqlite3_value_double(argv[0]);
    v1 = sqlite3_value_double(argv[1]);
    x = (double (*)(double, double))sqlite3_user_data(context);
    ans = x(v0, v1);
    sqlite3_result_double(context, ans);
}

/*
** Implementation of 0-argument pi() function.
*/
static void piFunc(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 0);
    (void)argv;
    sqlite3_result_double(context, M_PI);
}

int math_init(sqlite3* db) {
    static const int flags = SQLITE_UTF8 | SQLITE_INNOCUOUS | SQLITE_DETERMINISTIC;
    sqlite3_create_function(db, "ceil", 1, flags, xCeil, ceilingFunc, 0, 0);
    sqlite3_create_function(db, "ceiling", 1, flags, xCeil, ceilingFunc, 0, 0);
    sqlite3_create_function(db, "floor", 1, flags, xFloor, ceilingFunc, 0, 0);
    sqlite3_create_function(db, "trunc", 1, flags, trunc, ceilingFunc, 0, 0);
    sqlite3_create_function(db, "ln", 1, flags, 0, logFunc, 0, 0);
    sqlite3_create_function(db, "log", 1, flags, (void*)(1), logFunc, 0, 0);
    sqlite3_create_function(db, "log10", 1, flags, (void*)(1), logFunc, 0, 0);
    sqlite3_create_function(db, "log2", 1, flags, (void*)(2), logFunc, 0, 0);
    sqlite3_create_function(db, "log", 2, flags, 0, logFunc, 0, 0);
    sqlite3_create_function(db, "exp", 1, flags, exp, math1Func, 0, 0);
    sqlite3_create_function(db, "pow", 2, flags, pow, math2Func, 0, 0);
    sqlite3_create_function(db, "power", 2, flags, pow, math2Func, 0, 0);
    sqlite3_create_function(db, "mod", 2, flags, fmod, math2Func, 0, 0);
    sqlite3_create_function(db, "acos", 1, flags, acos, math1Func, 0, 0);
    sqlite3_create_function(db, "asin", 1, flags, asin, math1Func, 0, 0);
    sqlite3_create_function(db, "atan", 1, flags, atan, math1Func, 0, 0);
    sqlite3_create_function(db, "atan2", 2, flags, atan2, math2Func, 0, 0);
    sqlite3_create_function(db, "cos", 1, flags, cos, math1Func, 0, 0);
    sqlite3_create_function(db, "sin", 1, flags, sin, math1Func, 0, 0);
    sqlite3_create_function(db, "tan", 1, flags, tan, math1Func, 0, 0);
    sqlite3_create_function(db, "cosh", 1, flags, cosh, math1Func, 0, 0);
    sqlite3_create_function(db, "sinh", 1, flags, sinh, math1Func, 0, 0);
    sqlite3_create_function(db, "tanh", 1, flags, tanh, math1Func, 0, 0);
    sqlite3_create_function(db, "acosh", 1, flags, acosh, math1Func, 0, 0);
    sqlite3_create_function(db, "asinh", 1, flags, asinh, math1Func, 0, 0);
    sqlite3_create_function(db, "atanh", 1, flags, atanh, math1Func, 0, 0);
    sqlite3_create_function(db, "sqrt", 1, flags, sqrt, math1Func, 0, 0);
    sqlite3_create_function(db, "radians", 1, flags, degToRad, math1Func, 0, 0);
    sqlite3_create_function(db, "degrees", 1, flags, radToDeg, math1Func, 0, 0);
    sqlite3_create_function(db, "pi", 0, flags, 0, piFunc, 0, 0);
    return SQLITE_OK;
}

#endif // SQLEAN_ENABLE_MATH
#ifdef SQLEAN_ENABLE_STATS
// ---------------------------------
// src/stats/extension.c
// ---------------------------------
// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Statistical functions for SQLite.

SQLITE_EXTENSION_INIT3


int stats_init(sqlite3* db) {
    stats_scalar_init(db);
    stats_series_init(db);
    return SQLITE_OK;
}

// ---------------------------------
// src/stats/scalar.c
// ---------------------------------
// Standard deviation and variance by Liam Healy, Public Domain
// extension-functions.c at https://sqlite.org/contrib/

// Percentile by D. Richard Hipp, Public Domain
// https://sqlite.org/src/file/ext/misc/percentile.c

// Modified by Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Statistical functions for SQLite.

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

SQLITE_EXTENSION_INIT3

#pragma region Standard deviation and variance

/*
** An instance of the following structure holds the context of a
** stddev() or variance() aggregate computation.
** implementaion of http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance#Algorithm_II
** less prone to rounding errors
*/
typedef struct StddevCtx StddevCtx;
struct StddevCtx {
    double rM;
    double rS;
    int64_t cnt; /* number of elements */
};

/*
** called for each value received during a calculation of stddev or variance
*/
static void varianceStep(sqlite3_context* context, int argc, sqlite3_value** argv) {
    StddevCtx* p;

    double delta;
    double x;

    assert(argc == 1);
    p = sqlite3_aggregate_context(context, sizeof(*p));
    /* only consider non-null values */
    if (SQLITE_NULL != sqlite3_value_numeric_type(argv[0])) {
        p->cnt++;
        x = sqlite3_value_double(argv[0]);
        delta = (x - p->rM);
        p->rM += delta / p->cnt;
        p->rS += delta * (x - p->rM);
    }
}

/*
** Returns the sample standard deviation value
*/
static void stddevFinalize(sqlite3_context* context) {
    StddevCtx* p;
    p = sqlite3_aggregate_context(context, 0);
    if (p && p->cnt > 1) {
        sqlite3_result_double(context, sqrt(p->rS / (p->cnt - 1)));
    } else {
        sqlite3_result_double(context, 0.0);
    }
}

/*
** Returns the population standard deviation value
*/
static void stddevpopFinalize(sqlite3_context* context) {
    StddevCtx* p;
    p = sqlite3_aggregate_context(context, 0);
    if (p && p->cnt > 1) {
        sqlite3_result_double(context, sqrt(p->rS / p->cnt));
    } else {
        sqlite3_result_double(context, 0.0);
    }
}

/*
** Returns the sample variance value
*/
static void varianceFinalize(sqlite3_context* context) {
    StddevCtx* p;
    p = sqlite3_aggregate_context(context, 0);
    if (p && p->cnt > 1) {
        sqlite3_result_double(context, p->rS / (p->cnt - 1));
    } else {
        sqlite3_result_double(context, 0.0);
    }
}

/*
** Returns the population variance value
*/
static void variancepopFinalize(sqlite3_context* context) {
    StddevCtx* p;
    p = sqlite3_aggregate_context(context, 0);
    if (p && p->cnt > 1) {
        sqlite3_result_double(context, p->rS / p->cnt);
    } else {
        sqlite3_result_double(context, 0.0);
    }
}

#pragma endregion

#pragma region Percentile

/* The following object is the session context for a single percentile()
** function.  We have to remember all input Y values until the very end.
** Those values are accumulated in the Percentile.a[] array.
*/
typedef struct Percentile Percentile;
struct Percentile {
    unsigned nAlloc; /* Number of slots allocated for a[] */
    unsigned nUsed;  /* Number of slots actually used in a[] */
    double rPct;     /* 1.0 more than the value for P */
    double* a;       /* Array of Y values */
};

/*
** Return TRUE if the input floating-point number is an infinity.
*/
static int isInfinity(double r) {
    sqlite3_uint64 u;
    assert(sizeof(u) == sizeof(r));
    memcpy(&u, &r, sizeof(u));
    return ((u >> 52) & 0x7ff) == 0x7ff;
}

/*
** Return TRUE if two doubles differ by 0.001 or less
*/
static int sameValue(double a, double b) {
    a -= b;
    return a >= -0.001 && a <= 0.001;
}

/*
** The "step" function for percentile(Y,P) is called once for each
** input row.
*/
static void percentStep(sqlite3_context* pCtx, double rPct, int argc, sqlite3_value** argv) {
    Percentile* p;
    int eType;
    double y;

    /* Allocate the session context. */
    p = (Percentile*)sqlite3_aggregate_context(pCtx, sizeof(*p));
    if (p == 0)
        return;

    /* Remember the P value.  Throw an error if the P value is different
    ** from any prior row, per Requirement (2). */
    if (p->rPct == 0.0) {
        p->rPct = rPct + 1.0;
    } else if (!sameValue(p->rPct, rPct + 1.0)) {
        sqlite3_result_error(pCtx,
                             "2nd argument to percentile() is not the "
                             "same for all input rows",
                             -1);
        return;
    }

    /* Ignore rows for which Y is NULL */
    eType = sqlite3_value_type(argv[0]);
    if (eType == SQLITE_NULL)
        return;

    /* If not NULL, then Y must be numeric.  Otherwise throw an error.
    ** Requirement 4 */
    if (eType != SQLITE_INTEGER && eType != SQLITE_FLOAT) {
        sqlite3_result_error(pCtx,
                             "1st argument to percentile() is not "
                             "numeric",
                             -1);
        return;
    }

    /* Throw an error if the Y value is infinity or NaN */
    y = sqlite3_value_double(argv[0]);
    if (isInfinity(y)) {
        sqlite3_result_error(pCtx, "Inf input to percentile()", -1);
        return;
    }

    /* Allocate and store the Y */
    if (p->nUsed >= p->nAlloc) {
        unsigned n = p->nAlloc * 2 + 250;
        double* a = sqlite3_realloc64(p->a, sizeof(double) * n);
        if (a == 0) {
            sqlite3_free(p->a);
            memset(p, 0, sizeof(*p));
            sqlite3_result_error_nomem(pCtx);
            return;
        }
        p->nAlloc = n;
        p->a = a;
    }
    p->a[p->nUsed++] = y;
}

static void percentStepCustom(sqlite3_context* pCtx, int argc, sqlite3_value** argv) {
    assert(argc == 2);
    /* Requirement 3:  P must be a number between 0 and 100 */
    int eType = sqlite3_value_numeric_type(argv[1]);
    double rPct = sqlite3_value_double(argv[1]);
    if ((eType != SQLITE_INTEGER && eType != SQLITE_FLOAT) || rPct < 0.0 || rPct > 100.0) {
        sqlite3_result_error(pCtx,
                             "2nd argument to percentile() should be "
                             "a number between 0.0 and 100.0",
                             -1);
        return;
    }
    percentStep(pCtx, rPct, argc, argv);
}

static void percentStep25(sqlite3_context* pCtx, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    percentStep(pCtx, 25, argc, argv);
}

static void percentStep50(sqlite3_context* pCtx, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    percentStep(pCtx, 50, argc, argv);
}

static void percentStep75(sqlite3_context* pCtx, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    percentStep(pCtx, 75, argc, argv);
}

static void percentStep90(sqlite3_context* pCtx, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    percentStep(pCtx, 90, argc, argv);
}

static void percentStep95(sqlite3_context* pCtx, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    percentStep(pCtx, 95, argc, argv);
}

static void percentStep99(sqlite3_context* pCtx, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    percentStep(pCtx, 99, argc, argv);
}

/*
** Compare to doubles for sorting using qsort()
*/
static int SQLITE_CDECL doubleCmp(const void* pA, const void* pB) {
    double a = *(double*)pA;
    double b = *(double*)pB;
    if (a == b)
        return 0;
    if (a < b)
        return -1;
    return +1;
}

/*
** Called to compute the final output of percentile() and to clean
** up all allocated memory.
*/
static void percentFinal(sqlite3_context* pCtx) {
    Percentile* p;
    unsigned i1, i2;
    double v1, v2;
    double ix, vx;
    p = (Percentile*)sqlite3_aggregate_context(pCtx, 0);
    if (p == 0)
        return;
    if (p->a == 0)
        return;
    if (p->nUsed) {
        qsort(p->a, p->nUsed, sizeof(double), doubleCmp);
        ix = (p->rPct - 1.0) * (p->nUsed - 1) * 0.01;
        i1 = (unsigned)ix;
        i2 = ix == (double)i1 || i1 == p->nUsed - 1 ? i1 : i1 + 1;
        v1 = p->a[i1];
        v2 = p->a[i2];
        vx = v1 + (v2 - v1) * (ix - i1);
        sqlite3_result_double(pCtx, vx);
    }
    sqlite3_free(p->a);
    memset(p, 0, sizeof(*p));
}

#pragma endregion

int stats_scalar_init(sqlite3* db) {
    static const int flags = SQLITE_UTF8 | SQLITE_INNOCUOUS;
    sqlite3_create_function(db, "stddev", 1, flags, 0, 0, varianceStep, stddevFinalize);
    sqlite3_create_function(db, "stddev_samp", 1, flags, 0, 0, varianceStep, stddevFinalize);
    sqlite3_create_function(db, "stddev_pop", 1, flags, 0, 0, varianceStep, stddevpopFinalize);
    sqlite3_create_function(db, "variance", 1, flags, 0, 0, varianceStep, varianceFinalize);
    sqlite3_create_function(db, "var_samp", 1, flags, 0, 0, varianceStep, varianceFinalize);
    sqlite3_create_function(db, "var_pop", 1, flags, 0, 0, varianceStep, variancepopFinalize);
    sqlite3_create_function(db, "median", 1, flags, 0, 0, percentStep50, percentFinal);
    sqlite3_create_function(db, "percentile", 2, flags, 0, 0, percentStepCustom, percentFinal);
    sqlite3_create_function(db, "percentile_25", 1, flags, 0, 0, percentStep25, percentFinal);
    sqlite3_create_function(db, "percentile_75", 1, flags, 0, 0, percentStep75, percentFinal);
    sqlite3_create_function(db, "percentile_90", 1, flags, 0, 0, percentStep90, percentFinal);
    sqlite3_create_function(db, "percentile_95", 1, flags, 0, 0, percentStep95, percentFinal);
    sqlite3_create_function(db, "percentile_99", 1, flags, 0, 0, percentStep99, percentFinal);
    return SQLITE_OK;
}

// ---------------------------------
// src/stats/series.c
// ---------------------------------
// Originally by D. Richard Hipp, Public Domain
// https://sqlite.org/src/file/ext/misc/series.c

// Modified by Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean/

// generate_series function.

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

SQLITE_EXTENSION_INIT3

/* series_cursor is a subclass of sqlite3_vtab_cursor which will
** serve as the underlying representation of a cursor that scans
** over rows of the result
*/
typedef struct series_cursor series_cursor;
struct series_cursor {
    sqlite3_vtab_cursor base; /* Base class - must be first */
    int isDesc;               /* True to count down rather than up */
    sqlite3_int64 iRowid;     /* The rowid */
    sqlite3_int64 iValue;     /* Current value ("value") */
    sqlite3_int64 mnValue;    /* Mimimum value ("start") */
    sqlite3_int64 mxValue;    /* Maximum value ("stop") */
    sqlite3_int64 iStep;      /* Increment ("step") */
};

/*
** The seriesConnect() method is invoked to create a new
** series_vtab that describes the generate_series virtual table.
**
** Think of this routine as the constructor for series_vtab objects.
**
** All this routine needs to do is:
**
**    (1) Allocate the series_vtab object and initialize all fields.
**
**    (2) Tell SQLite (via the sqlite3_declare_vtab() interface) what the
**        result set of queries against generate_series will look like.
*/
static int seriesConnect(sqlite3* db,
                         void* pUnused,
                         int argcUnused,
                         const char* const* argvUnused,
                         sqlite3_vtab** ppVtab,
                         char** pzErrUnused) {
    sqlite3_vtab* pNew;
    int rc;

/* Column numbers */
#define SERIES_COLUMN_VALUE 0
#define SERIES_COLUMN_START 1
#define SERIES_COLUMN_STOP 2
#define SERIES_COLUMN_STEP 3

    (void)pUnused;
    (void)argcUnused;
    (void)argvUnused;
    (void)pzErrUnused;
    rc = sqlite3_declare_vtab(db, "CREATE TABLE x(value,start hidden,stop hidden,step hidden)");
    if (rc == SQLITE_OK) {
        pNew = *ppVtab = sqlite3_malloc(sizeof(*pNew));
        if (pNew == 0)
            return SQLITE_NOMEM;
        memset(pNew, 0, sizeof(*pNew));
        sqlite3_vtab_config(db, SQLITE_VTAB_INNOCUOUS);
    }
    return rc;
}

/*
** This method is the destructor for series_cursor objects.
*/
static int seriesDisconnect(sqlite3_vtab* pVtab) {
    sqlite3_free(pVtab);
    return SQLITE_OK;
}

/*
** Constructor for a new series_cursor object.
*/
static int seriesOpen(sqlite3_vtab* pUnused, sqlite3_vtab_cursor** ppCursor) {
    series_cursor* pCur;
    (void)pUnused;
    pCur = sqlite3_malloc(sizeof(*pCur));
    if (pCur == 0)
        return SQLITE_NOMEM;
    memset(pCur, 0, sizeof(*pCur));
    *ppCursor = &pCur->base;
    return SQLITE_OK;
}

/*
** Destructor for a series_cursor.
*/
static int seriesClose(sqlite3_vtab_cursor* cur) {
    sqlite3_free(cur);
    return SQLITE_OK;
}

/*
** Advance a series_cursor to its next row of output.
*/
static int seriesNext(sqlite3_vtab_cursor* cur) {
    series_cursor* pCur = (series_cursor*)cur;
    if (pCur->isDesc) {
        pCur->iValue -= pCur->iStep;
    } else {
        pCur->iValue += pCur->iStep;
    }
    pCur->iRowid++;
    return SQLITE_OK;
}

/*
** Return values of columns for the row at which the series_cursor
** is currently pointing.
*/
static int seriesColumn(sqlite3_vtab_cursor* cur, /* The cursor */
                        sqlite3_context* ctx,     /* First argument to sqlite3_result_...() */
                        int i                     /* Which column to return */
) {
    series_cursor* pCur = (series_cursor*)cur;
    sqlite3_int64 x = 0;
    switch (i) {
        case SERIES_COLUMN_START:
            x = pCur->mnValue;
            break;
        case SERIES_COLUMN_STOP:
            x = pCur->mxValue;
            break;
        case SERIES_COLUMN_STEP:
            x = pCur->iStep;
            break;
        default:
            x = pCur->iValue;
            break;
    }
    sqlite3_result_int64(ctx, x);
    return SQLITE_OK;
}

/*
** Return the rowid for the current row. In this implementation, the
** first row returned is assigned rowid value 1, and each subsequent
** row a value 1 more than that of the previous.
*/
static int seriesRowid(sqlite3_vtab_cursor* cur, sqlite_int64* pRowid) {
    series_cursor* pCur = (series_cursor*)cur;
    *pRowid = pCur->iRowid;
    return SQLITE_OK;
}

/*
** Return TRUE if the cursor has been moved off of the last
** row of output.
*/
static int seriesEof(sqlite3_vtab_cursor* cur) {
    series_cursor* pCur = (series_cursor*)cur;
    if (pCur->isDesc) {
        return pCur->iValue < pCur->mnValue;
    } else {
        return pCur->iValue > pCur->mxValue;
    }
}

/* True to cause run-time checking of the start=, stop=, and/or step=
** parameters.  The only reason to do this is for testing the
** constraint checking logic for virtual tables in the SQLite core.
*/
#ifndef SQLITE_SERIES_CONSTRAINT_VERIFY
#define SQLITE_SERIES_CONSTRAINT_VERIFY 0
#endif

/*
** This method is called to "rewind" the series_cursor object back
** to the first row of output.  This method is always called at least
** once prior to any call to seriesColumn() or seriesRowid() or
** seriesEof().
**
** The query plan selected by seriesBestIndex is passed in the idxNum
** parameter.  (idxStr is not used in this implementation.)  idxNum
** is a bitmask showing which constraints are available:
**
**    1:    start=VALUE
**    2:    stop=VALUE
**    4:    step=VALUE
**
** Also, if bit 8 is set, that means that the series should be output
** in descending order rather than in ascending order.  If bit 16 is
** set, then output must appear in ascending order.
**
** This routine should initialize the cursor and position it so that it
** is pointing at the first row, or pointing off the end of the table
** (so that seriesEof() will return true) if the table is empty.
*/
static int seriesFilter(sqlite3_vtab_cursor* pVtabCursor,
                        int idxNum,
                        const char* idxStrUnused,
                        int argc,
                        sqlite3_value** argv) {
    series_cursor* pCur = (series_cursor*)pVtabCursor;
    int i = 0;
    (void)idxStrUnused;
    if (idxNum & 1) {
        pCur->mnValue = sqlite3_value_int64(argv[i++]);
    } else {
        pCur->mnValue = 0;
    }
    if (idxNum & 2) {
        pCur->mxValue = sqlite3_value_int64(argv[i++]);
    } else {
        pCur->mxValue = 0xffffffff;
    }
    if (idxNum & 4) {
        pCur->iStep = sqlite3_value_int64(argv[i++]);
        if (pCur->iStep == 0) {
            pCur->iStep = 1;
        } else if (pCur->iStep < 0) {
            pCur->iStep = -pCur->iStep;
            if ((idxNum & 16) == 0)
                idxNum |= 8;
        }
    } else {
        pCur->iStep = 1;
    }
    for (i = 0; i < argc; i++) {
        if (sqlite3_value_type(argv[i]) == SQLITE_NULL) {
            /* If any of the constraints have a NULL value, then return no rows.
            ** See ticket https://www.sqlite.org/src/info/fac496b61722daf2 */
            pCur->mnValue = 1;
            pCur->mxValue = 0;
            break;
        }
    }
    if (idxNum & 8) {
        pCur->isDesc = 1;
        pCur->iValue = pCur->mxValue;
        if (pCur->iStep > 0) {
            pCur->iValue -= (pCur->mxValue - pCur->mnValue) % pCur->iStep;
        }
    } else {
        pCur->isDesc = 0;
        pCur->iValue = pCur->mnValue;
    }
    pCur->iRowid = 1;
    return SQLITE_OK;
}

/*
** SQLite will invoke this method one or more times while planning a query
** that uses the generate_series virtual table.  This routine needs to create
** a query plan for each invocation and compute an estimated cost for that
** plan.
**
** In this implementation idxNum is used to represent the
** query plan.  idxStr is unused.
**
** The query plan is represented by bits in idxNum:
**
**  (1)  start = $value  -- constraint exists
**  (2)  stop = $value   -- constraint exists
**  (4)  step = $value   -- constraint exists
**  (8)  output in descending order
*/
static int seriesBestIndex(sqlite3_vtab* pVTab, sqlite3_index_info* pIdxInfo) {
    int i, j;             /* Loop over constraints */
    int idxNum = 0;       /* The query plan bitmask */
    int bStartSeen = 0;   /* EQ constraint seen on the START column */
    int unusableMask = 0; /* Mask of unusable constraints */
    int nArg = 0;         /* Number of arguments that seriesFilter() expects */
    int aIdx[3];          /* Constraints on start, stop, and step */
    const struct sqlite3_index_constraint* pConstraint;

    /* This implementation assumes that the start, stop, and step columns
    ** are the last three columns in the virtual table. */
    assert(SERIES_COLUMN_STOP == SERIES_COLUMN_START + 1);
    assert(SERIES_COLUMN_STEP == SERIES_COLUMN_START + 2);

    aIdx[0] = aIdx[1] = aIdx[2] = -1;
    pConstraint = pIdxInfo->aConstraint;
    for (i = 0; i < pIdxInfo->nConstraint; i++, pConstraint++) {
        int iCol;  /* 0 for start, 1 for stop, 2 for step */
        int iMask; /* bitmask for those column */
        if (pConstraint->iColumn < SERIES_COLUMN_START)
            continue;
        iCol = pConstraint->iColumn - SERIES_COLUMN_START;
        assert(iCol >= 0 && iCol <= 2);
        iMask = 1 << iCol;
        if (iCol == 0)
            bStartSeen = 1;
        if (pConstraint->usable == 0) {
            unusableMask |= iMask;
            continue;
        } else if (pConstraint->op == SQLITE_INDEX_CONSTRAINT_EQ) {
            idxNum |= iMask;
            aIdx[iCol] = i;
        }
    }
    for (i = 0; i < 3; i++) {
        if ((j = aIdx[i]) >= 0) {
            pIdxInfo->aConstraintUsage[j].argvIndex = ++nArg;
            pIdxInfo->aConstraintUsage[j].omit = !SQLITE_SERIES_CONSTRAINT_VERIFY;
        }
    }
    /* The current generate_column() implementation requires at least one
    ** argument (the START value).  Legacy versions assumed START=0 if the
    ** first argument was omitted.  Compile with -DZERO_ARGUMENT_GENERATE_SERIES
    ** to obtain the legacy behavior */
#ifndef ZERO_ARGUMENT_GENERATE_SERIES
    if (!bStartSeen) {
        sqlite3_free(pVTab->zErrMsg);
        pVTab->zErrMsg =
            sqlite3_mprintf("first argument to \"generate_series()\" missing or unusable");
        return SQLITE_ERROR;
    }
#endif
    if ((unusableMask & ~idxNum) != 0) {
        /* The start, stop, and step columns are inputs.  Therefore if there
        ** are unusable constraints on any of start, stop, or step then
        ** this plan is unusable */
        return SQLITE_CONSTRAINT;
    }
    if ((idxNum & 3) == 3) {
        /* Both start= and stop= boundaries are available.  This is the
        ** the preferred case */
        pIdxInfo->estimatedCost = (double)(2 - ((idxNum & 4) != 0));
        pIdxInfo->estimatedRows = 1000;
        if (pIdxInfo->nOrderBy == 1) {
            if (pIdxInfo->aOrderBy[0].desc) {
                idxNum |= 8;
            } else {
                idxNum |= 16;
            }
            pIdxInfo->orderByConsumed = 1;
        }
    } else {
        /* If either boundary is missing, we have to generate a huge span
        ** of numbers.  Make this case very expensive so that the query
        ** planner will work hard to avoid it. */
        pIdxInfo->estimatedRows = 2147483647;
    }
    pIdxInfo->idxNum = idxNum;
    return SQLITE_OK;
}

/*
** This following structure defines all the methods for the
** generate_series virtual table.
*/
static sqlite3_module series_module = {
    .xConnect = seriesConnect,
    .xBestIndex = seriesBestIndex,
    .xDisconnect = seriesDisconnect,
    .xOpen = seriesOpen,
    .xClose = seriesClose,
    .xFilter = seriesFilter,
    .xNext = seriesNext,
    .xEof = seriesEof,
    .xColumn = seriesColumn,
    .xRowid = seriesRowid,
};

int stats_series_init(sqlite3* db) {
    sqlite3_create_module(db, "generate_series", &series_module, 0);
    return SQLITE_OK;
}

#endif // SQLEAN_ENABLE_STATS

// add sqlean_version() sql function that returns the current version of sqlean
void sqlean_version(sqlite3_context* context, int argc, sqlite3_value** argv) {
  sqlite3_result_text(context, SQLEAN_VERSION, -1, SQLITE_STATIC);
}

#ifdef __cplusplus
}
#endif
