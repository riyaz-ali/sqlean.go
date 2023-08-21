//go:build !sqlean_omit_uuid
// +build !sqlean_omit_uuid

package sqlean

// #cgo CFLAGS: -DSQLEAN_ENABLE_UUID
//
// #include "sqlean.h"
import "C"

func init() {
	register("uuid", func(c DatabaseConnection) error { return ret(C.uuid_init((*C.struct_sqlite3)(c))) })
}
