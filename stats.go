//go:build !sqlean_omit_stats
// +build !sqlean_omit_stats

package sqlean

// #cgo CFLAGS: -DSQLEAN_ENABLE_STATS
//
// #include "sqlean.h"
import "C"

func init() {
	register("stats", func(c DatabaseConnection) error { return ret(C.stats_init((*C.struct_sqlite3)(c))) })
}
