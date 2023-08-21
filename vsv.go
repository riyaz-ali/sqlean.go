//go:build !sqlean_omit_vsv
// +build !sqlean_omit_vsv

package sqlean

// #cgo CFLAGS: -DSQLEAN_ENABLE_VSV
//
// #include "sqlean.h"
import "C"

func init() {
	register("vsv", func(c DatabaseConnection) error { return ret(C.vsv_init((*C.struct_sqlite3)(c))) })
}
