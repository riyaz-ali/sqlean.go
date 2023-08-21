//go:build !sqlean_omit_text
// +build !sqlean_omit_text

package sqlean

// #cgo CFLAGS: -DSQLEAN_ENABLE_TEXT
//
// #include "sqlean.h"
import "C"

func init() {
	register("text", func(c DatabaseConnection) error { return ret(C.text_init((*C.struct_sqlite3)(c))) })
}
