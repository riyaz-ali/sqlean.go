//go:build !sqlean_omit_math
// +build !sqlean_omit_math

package sqlean

// #cgo CFLAGS: -DSQLEAN_ENABLE_MATH
//
// #include "sqlean.h"
import "C"

func init() {
	register("math", func(c DatabaseConnection) error { return ret(C.math_init((*C.struct_sqlite3)(c))) })
}
