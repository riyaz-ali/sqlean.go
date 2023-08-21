//go:build !sqlean_omit_crypto
// +build !sqlean_omit_crypto

package sqlean

// #cgo CFLAGS: -DSQLEAN_ENABLE_CRYPTO
//
// #include "sqlean.h"
import "C"

func init() {
	register("crypto", func(c DatabaseConnection) error { return ret(C.crypto_init((*C.struct_sqlite3)(c))) })
}
