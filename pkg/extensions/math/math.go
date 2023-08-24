package math

// #cgo CFLAGS: -DSQLITE_CORE -I${SRCDIR}/..
// #include "extension.h"
import "C"
import "unsafe"
import registry "github.com/riyaz-ali/sqlean.go/pkg/registry"

func init() {
  registry.Append(func(c unsafe.Pointer) { C.math_init((*C.struct_sqlite3)(c)) })
}
