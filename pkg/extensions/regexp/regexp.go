package regexp

// #cgo CFLAGS: -DSQLITE_CORE -I${SRCDIR}/..
// #cgo linux LDFLAGS: -Wl,--unresolved-symbols=ignore-in-object-files
// #cgo darwin LDFLAGS: -Wl,-undefined,dynamic_lookup
// #cgo windows LDFLAGS: -Wl,--allow-multiple-definition
//
// #include "extension.h"
import "C"
import "unsafe"
import registry "github.com/riyaz-ali/sqlean.go/pkg/registry"

func init() {
	registry.Append(func(c unsafe.Pointer) { C.regexp_init((*C.struct_sqlite3)(c)) })
}
