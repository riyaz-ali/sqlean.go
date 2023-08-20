// ---------------------------------
// Following is an amalgamated version of sqlean v0.21.6
// License @ https://github.com/nalgeon/sqlean/blob/main/LICENSE
// Find more details @ https://github.com/nalgeon/sqlean
// All copyrights belong to original author(s)
// ---------------------------------

#ifndef SQLEAN_H
#define SQLEAN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sqlite3.h>
#include <sqlite3ext.h>
#include <stddef.h>

#define SQLEAN_VERSION "0.21.6"
#define SQLEAN_GENERATE_TIMESTAMP "2023-08-20T11:43:28+05:30"

#ifdef SQLEAN_ENABLE_CRYPTO
// ---------------------------------
// src/crypto/base32.h
// ---------------------------------
// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Base32 encoding/decoding (RFC 4648)

#ifndef _BASE32_H_
#define _BASE32_H_

#include <stdint.h>

uint8_t* base32_encode(const uint8_t* src, size_t len, size_t* out_len);
uint8_t* base32_decode(const uint8_t* src, size_t len, size_t* out_len);

#endif /* _BASE32_H_ */

// ---------------------------------
// src/crypto/base64.h
// ---------------------------------
// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Base64 encoding/decoding (RFC 4648)

#ifndef BASE64_H
#define BASE64_H

#include <stddef.h>
#include <stdint.h>

uint8_t* base64_encode(const uint8_t* src, size_t len, size_t* out_len);
uint8_t* base64_decode(const uint8_t* src, size_t len, size_t* out_len);

#endif /* BASE64_H */

// ---------------------------------
// src/crypto/base85.h
// ---------------------------------
// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Base85 (Ascii85) encoding/decoding

#ifndef _BASE85_H_
#define _BASE85_H_

#include <stddef.h>
#include <stdint.h>

uint8_t* base85_encode(const uint8_t* src, size_t len, size_t* out_len);
uint8_t* base85_decode(const uint8_t* src, size_t len, size_t* out_len);

#endif /* _BASE85_H_ */

// ---------------------------------
// src/crypto/extension.h
// ---------------------------------
// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// SQLite hash and encode/decode functions.

#ifndef CRYPTO_EXTENSION_H
#define CRYPTO_EXTENSION_H


int crypto_init(sqlite3* db);

#endif /* CRYPTO_EXTENSION_H */

// ---------------------------------
// src/crypto/hex.h
// ---------------------------------
// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Hex encoding/decoding

#ifndef _HEX_H_
#define _HEX_H_

#include <stddef.h>
#include <stdint.h>

uint8_t* hex_encode(const uint8_t* src, size_t len, size_t* out_len);
uint8_t* hex_decode(const uint8_t* src, size_t len, size_t* out_len);

#endif /* _HEX_H_ */

// ---------------------------------
// src/crypto/md5.h
// ---------------------------------
/*********************************************************************
 * Filename:   md5.h
 * Author:     Brad Conte (brad AT bradconte.com)
 * Source:     https://github.com/B-Con/crypto-algorithms
 * License:    Public Domain
 * Details:    Defines the API for the corresponding MD5 implementation.
 *********************************************************************/

#ifndef MD5_H
#define MD5_H

/*************************** HEADER FILES ***************************/
#include <stddef.h>

/****************************** MACROS ******************************/
#define MD5_BLOCK_SIZE 16  // MD5 outputs a 16 byte digest

/**************************** DATA TYPES ****************************/
typedef unsigned char BYTE;  // 8-bit byte
typedef unsigned int WORD;   // 32-bit word, change to "long" for 16-bit machines

typedef struct {
    BYTE data[64];
    WORD datalen;
    unsigned long long bitlen;
    WORD state[4];
} MD5_CTX;

/*********************** FUNCTION DECLARATIONS **********************/
void* md5_init();
void md5_update(MD5_CTX* ctx, const BYTE data[], size_t len);
int md5_final(MD5_CTX* ctx, BYTE hash[]);

#endif  // MD5_H

// ---------------------------------
// src/crypto/sha1.h
// ---------------------------------
// Adapted from https://sqlite.org/src/file/ext/misc/sha1.c
// Public domain

#ifndef __SHA1_H__
#define __SHA1_H__

#include <stddef.h>

#define SHA1_BLOCK_SIZE 20

typedef struct SHA1Context {
    unsigned int state[5];
    unsigned int count[2];
    unsigned char buffer[64];
} SHA1Context;

void* sha1_init();
void sha1_update(SHA1Context* ctx, const unsigned char data[], size_t len);
int sha1_final(SHA1Context* ctx, unsigned char hash[]);

#endif

// ---------------------------------
// src/crypto/sha2.h
// ---------------------------------
/*
 * FILE:    sha2.h
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
 * $Id: sha2.h,v 1.1 2001/11/08 00:02:01 adg Exp adg $
 */

#ifndef __SHA2_H__
#define __SHA2_H__

#define SHA2_USE_INTTYPES_H
#define SHA2_UNROLL_TRANSFORM
#define NOPROTO

/*
 * Import u_intXX_t size_t type definitions from system headers.  You
 * may need to change this, or define these things yourself in this
 * file.
 */
#include <sys/types.h>

#ifdef SHA2_USE_INTTYPES_H

#include <inttypes.h>

#endif /* SHA2_USE_INTTYPES_H */

/*** SHA-256/384/512 Various Length Definitions ***********************/
#define SHA256_BLOCK_LENGTH 64
#define SHA256_DIGEST_LENGTH 32
#define SHA256_DIGEST_STRING_LENGTH (SHA256_DIGEST_LENGTH * 2 + 1)
#define SHA384_BLOCK_LENGTH 128
#define SHA384_DIGEST_LENGTH 48
#define SHA384_DIGEST_STRING_LENGTH (SHA384_DIGEST_LENGTH * 2 + 1)
#define SHA512_BLOCK_LENGTH 128
#define SHA512_DIGEST_LENGTH 64
#define SHA512_DIGEST_STRING_LENGTH (SHA512_DIGEST_LENGTH * 2 + 1)

/*** SHA-256/384/512 Context Structures *******************************/

typedef struct _SHA256_CTX {
    uint32_t state[8];
    uint64_t bitcount;
    uint8_t buffer[SHA256_BLOCK_LENGTH];
} SHA256_CTX;

typedef struct _SHA512_CTX {
    uint64_t state[8];
    uint64_t bitcount[2];
    uint8_t buffer[SHA512_BLOCK_LENGTH];
} SHA512_CTX;

typedef SHA512_CTX SHA384_CTX;

/*** SHA-256/384/512 Function Prototypes ******************************/

void* sha256_init();
void sha256_update(SHA256_CTX*, const uint8_t*, size_t);
int sha256_final(SHA256_CTX*, uint8_t[SHA256_DIGEST_LENGTH]);

void* sha384_init();
void sha384_update(SHA384_CTX*, const uint8_t*, size_t);
int sha384_final(SHA384_CTX*, uint8_t[SHA384_DIGEST_LENGTH]);

void* sha512_init();
void sha512_update(SHA512_CTX*, const uint8_t*, size_t);
int sha512_final(SHA512_CTX*, uint8_t[SHA512_DIGEST_LENGTH]);

#endif  // MD5_H

// ---------------------------------
// src/crypto/url.h
// ---------------------------------
// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// URL-escape encoding/decoding

#ifndef _URL_H_
#define _URL_H_

#include <stddef.h>
#include <stdint.h>

uint8_t* url_encode(const uint8_t* src, size_t len, size_t* out_len);
uint8_t* url_decode(const uint8_t* src, size_t len, size_t* out_len);

#endif /* _URL_H_ */

#endif // SQLEAN_ENABLE_CRYPTO
#ifdef SQLEAN_ENABLE_FILEIO
// ---------------------------------
// src/fileio/extension.h
// ---------------------------------
// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Read and write files in SQLite.

#ifndef FILEIO_EXTENSION_H
#define FILEIO_EXTENSION_H


int fileio_init(sqlite3* db);

#endif /* FILEIO_EXTENSION_H */

// ---------------------------------
// src/fileio/fileio.h
// ---------------------------------
// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Read and write files in SQLite.

#ifndef FILEIO_INTERNAL_H
#define FILEIO_INTERNAL_H


int fileio_ls_init(sqlite3* db);
int fileio_scalar_init(sqlite3* db);
int fileio_scan_init(sqlite3* db);

#endif /* FILEIO_INTERNAL_H */

#endif // SQLEAN_ENABLE_FILEIO
#ifdef SQLEAN_ENABLE_IPADDR
// ---------------------------------
// src/ipaddr/extension.h
// ---------------------------------
// Copyright (c) 2021 Vincent Bernat, MIT License
// https://github.com/nalgeon/sqlean

// IP address manipulation in SQLite.

#ifndef IPADDR_EXTENSION_H
#define IPADDR_EXTENSION_H


int ipaddr_init(sqlite3* db);

#endif /* IPADDR_EXTENSION_H */

#endif // SQLEAN_ENABLE_IPADDR
#ifdef SQLEAN_ENABLE_MATH
// ---------------------------------
// src/math/extension.h
// ---------------------------------
// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// SQLite math functions.

#ifndef MATH_EXTENSION_H
#define MATH_EXTENSION_H


int math_init(sqlite3* db);

#endif /* MATH_EXTENSION_H */

#endif // SQLEAN_ENABLE_MATH
#ifdef SQLEAN_ENABLE_VSV
// ---------------------------------
// src/vsv/extension.h
// ---------------------------------
// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// CSV files as virtual tables in SQLite

#ifndef VSV_EXTENSION_H
#define VSV_EXTENSION_H


int vsv_init(sqlite3* db);

#endif /* VSV_EXTENSION_H */

#endif // SQLEAN_ENABLE_VSV
#ifdef SQLEAN_ENABLE_DEFINE
// ---------------------------------
// src/define/define.h
// ---------------------------------
// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// User-defined functions in SQLite.

#ifndef DEFINE_INTERNAL_H
#define DEFINE_INTERNAL_H


int define_save_function(sqlite3* db, const char* name, const char* type, const char* body);

int define_eval_init(sqlite3* db);
int define_manage_init(sqlite3* db);
int define_module_init(sqlite3* db);

#endif /* DEFINE_INTERNAL_H */

// ---------------------------------
// src/define/extension.h
// ---------------------------------
// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// User-defined functions in SQLite.

#ifndef DEFINE_EXTENSION_H
#define DEFINE_EXTENSION_H


int define_init(sqlite3* db);

#endif /* DEFINE_EXTENSION_H */

#endif // SQLEAN_ENABLE_DEFINE
#ifdef SQLEAN_ENABLE_STATS
// ---------------------------------
// src/stats/extension.h
// ---------------------------------
// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Statistical functions for SQLite.

#ifndef STATS_EXTENSION_H
#define STATS_EXTENSION_H


int stats_init(sqlite3* db);

#endif /* STATS_EXTENSION_H */

// ---------------------------------
// src/stats/stats.h
// ---------------------------------
// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Statistical functions for SQLite.

#ifndef STATS_INTERNAL_H
#define STATS_INTERNAL_H


int stats_scalar_init(sqlite3* db);
int stats_series_init(sqlite3* db);

#endif /* STATS_INTERNAL_H */

#endif // SQLEAN_ENABLE_STATS
#ifdef SQLEAN_ENABLE_TEXT
// ---------------------------------
// src/text/bstring.h
// ---------------------------------
// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Byte string data structure.

#ifndef BSTRING_H
#define BSTRING_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// ByteString is a string composed of bytes.
typedef struct {
    // array of bytes
    const char* bytes;
    // number of bytes in the string
    size_t length;
    // indicates whether the string owns the array
    // and should free the memory when destroyed
    bool owning;
} ByteString;

// ByteString methods.
struct bstring_ns {
    ByteString (*new)(void);
    ByteString (*from_cstring)(const char* const cstring, size_t length);
    const char* (*to_cstring)(ByteString str);
    void (*free)(ByteString str);

    char (*at)(ByteString str, size_t idx);
    ByteString (*slice)(ByteString str, int start, int end);
    ByteString (*substring)(ByteString str, size_t start, size_t length);

    int (*index)(ByteString str, ByteString other);
    int (*last_index)(ByteString str, ByteString other);
    bool (*contains)(ByteString str, ByteString other);
    bool (*equals)(ByteString str, ByteString other);
    bool (*has_prefix)(ByteString str, ByteString other);
    bool (*has_suffix)(ByteString str, ByteString other);
    size_t (*count)(ByteString str, ByteString other);

    ByteString (*split_part)(ByteString str, ByteString sep, size_t part);
    ByteString (*join)(ByteString* strings, size_t count, ByteString sep);
    ByteString (*concat)(ByteString* strings, size_t count);
    ByteString (*repeat)(ByteString str, size_t count);

    ByteString (*replace)(ByteString str, ByteString old, ByteString new, size_t max_count);
    ByteString (*replace_all)(ByteString str, ByteString old, ByteString new);
    ByteString (*reverse)(ByteString str);

    ByteString (*trim_left)(ByteString str);
    ByteString (*trim_right)(ByteString str);
    ByteString (*trim)(ByteString str);

    void (*print)(ByteString str);
};

extern struct bstring_ns bstring;

#endif /* BSTRING_H */

// ---------------------------------
// src/text/extension.h
// ---------------------------------
// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// SQLite extension for working with text.

#ifndef TEXT_EXTENSION_H
#define TEXT_EXTENSION_H


int text_init(sqlite3* db);

#endif /* TEXT_EXTENSION_H */

// ---------------------------------
// src/text/rstring.h
// ---------------------------------
// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Rune (UTF-8) string data structure.

#ifndef RSTRING_H
#define RSTRING_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// RuneString is a string composed of UTF-8 characters (runes).
typedef struct {
    // array of utf-8 characters
    const int32_t* runes;
    // number of characters in the string
    size_t length;
    // number of bytes in the string
    size_t size;
    // indicates whether the string owns the array
    // and should free the memory when destroyed
    bool owning;
} RuneString;

// RuneString methods.
struct rstring_ns {
    RuneString (*new)(void);
    RuneString (*from_cstring)(const char* const utf8str);
    char* (*to_cstring)(RuneString str);
    void (*free)(RuneString str);

    int32_t (*at)(RuneString str, size_t idx);
    RuneString (*slice)(RuneString str, int start, int end);
    RuneString (*substring)(RuneString str, size_t start, size_t length);

    int (*index)(RuneString str, RuneString other);
    int (*last_index)(RuneString str, RuneString other);

    RuneString (*translate)(RuneString str, RuneString from, RuneString to);
    RuneString (*reverse)(RuneString str);

    RuneString (*trim_left)(RuneString str, RuneString chars);
    RuneString (*trim_right)(RuneString str, RuneString chars);
    RuneString (*trim)(RuneString str, RuneString chars);
    RuneString (*pad_left)(RuneString str, size_t length, RuneString fill);
    RuneString (*pad_right)(RuneString str, size_t length, RuneString fill);

    void (*print)(RuneString str);
};

extern struct rstring_ns rstring;

#endif /* RSTRING_H */

// ---------------------------------
// src/text/runes.h
// ---------------------------------
// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// UTF-8 characters (runes) <-> C string conversions.

#ifndef RUNES_H
#define RUNES_H

#include <stdint.h>
#include <stdlib.h>

int32_t* runes_from_cstring(const char* const str, size_t length);
char* runes_to_cstring(const int32_t* runes, size_t length);

#endif /* RUNES_H */

#endif // SQLEAN_ENABLE_TEXT
#ifdef SQLEAN_ENABLE_UNICODE
// ---------------------------------
// src/unicode/extension.h
// ---------------------------------
// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Unicode support for SQLite.

#ifndef UNICODE_EXTENSION_H
#define UNICODE_EXTENSION_H


int unicode_init(sqlite3* db);

#endif /* UNICODE_EXTENSION_H */

#endif // SQLEAN_ENABLE_UNICODE
#ifdef SQLEAN_ENABLE_UUID
// ---------------------------------
// src/uuid/extension.h
// ---------------------------------
// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Universally Unique IDentifiers (UUIDs) in SQLite

#ifndef UUID_EXTENSION_H
#define UUID_EXTENSION_H


int uuid_init(sqlite3* db);

#endif /* UUID_EXTENSION_H */

#endif // SQLEAN_ENABLE_UUID

// add sqlean_version() sql function that returns the current version of sqlean
void sqlean_version(sqlite3_context* context, int argc, sqlite3_value** argv);

#ifdef __cplusplus
}
#endif

#endif  // SQLEAN_H
