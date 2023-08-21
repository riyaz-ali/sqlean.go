//go:build !sqlean_omit_ipaddr && !windows
// +build !sqlean_omit_ipaddr,!windows

package sqlean

// #cgo CFLAGS: -DSQLEAN_ENABLE_IPADDR
//
// #include "sqlean.h"
import "C"

func init() {
	register("ipaddr", func(c DatabaseConnection) error { return ret(C.ipaddr_init((*C.struct_sqlite3)(c))) })
}
