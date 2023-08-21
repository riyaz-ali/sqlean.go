//go:build !sqlean_omit_unicode
// +build !sqlean_omit_unicode

package sqlean

// #cgo CFLAGS: -DSQLEAN_ENABLE_UNICODE
//
// #include "sqlean.h"
import "C"

func init() {
	register("unicode", func(c DatabaseConnection) error { return ret(C.unicode_init((*C.struct_sqlite3)(c))) })
}
