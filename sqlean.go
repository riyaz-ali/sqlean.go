// Package sqlean provides an embeddable version of sqlean extensions.
package sqlean

// #cgo CFLAGS: -DSQLITE_CORE
//
// #include <stdlib.h> // for C.CString and C.free below
// #include "sqlean.h"
import "C"
import (
	"fmt"
	"unsafe"
)

// DatabaseConnection represents the underlying sqlite3 database connection
type DatabaseConnection *C.struct_sqlite3

// global registry of extension functions
var extensions = make(map[string]func(DatabaseConnection) error)

// register named extension function with global registry
func register(name string, fn func(DatabaseConnection) error) { extensions[name] = fn }

// Register registers all enabled extensions with the provided connection
func Register(c DatabaseConnection) error {
	for name, fn := range extensions {
		if err := fn(c); err != nil {
			return fmt.Errorf("failed to register extension %s: %w", name, err)
		}
	}
	return nil
}

// ret wraps return code from sqlite3 api, returning non-nil error if c != SQLITE_OK
func ret(c C.int) error {
	if c != C.SQLITE_OK {
		return fmt.Errorf("sqlite error: %d", int(c))
	} else {
		return nil
	}
}

func init() {
	register("sqlean_version", func(c DatabaseConnection) error {
		var name = C.CString("sqlean_version")
		defer C.free(unsafe.Pointer(name))

		var flags C.int = C.SQLITE_UTF8 | C.SQLITE_INNOCUOUS | C.SQLITE_DETERMINISTIC

		return ret(C.sqlite3_create_function((*C.struct_sqlite3)(c), name, C.int(0), flags, nil, (*[0]byte)(C.sqlean_version), nil, nil))
	})
}
