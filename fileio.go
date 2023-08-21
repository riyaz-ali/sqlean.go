//go:build !sqlean_omit_fileio
// +build !sqlean_omit_fileio

package sqlean

// #cgo CFLAGS: -DSQLEAN_ENABLE_FILEIO
//
// #include "sqlean.h"
import "C"

func init() {
	register("fileio", func(c DatabaseConnection) error { return ret(C.fileio_init((*C.struct_sqlite3)(c))) })
}
