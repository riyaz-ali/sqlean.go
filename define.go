//go:build !sqlean_omit_define
// +build !sqlean_omit_define

package sqlean

// #cgo CFLAGS: -DSQLEAN_ENABLE_DEFINE
//
// #include "sqlean.h"
import "C"

func init() {
	register("define", func(c DatabaseConnection) error { return ret(C.define_init((*C.struct_sqlite3)(c))) })
}
