// Package sqlean provides an embeddable version of sqlean extensions.
package sqlean

import "C"
import "unsafe"
import "github.com/riyaz-ali/sqlean.go/pkg/registry"

// DatabaseConnection represents the underlying sqlite3 database connection
type DatabaseConnection *C.struct_sqlite3

// Register registers all enabled extensions with the provided connection
func Register(c DatabaseConnection) error {
	for _, fn := range registry.Extensions() {
		fn(unsafe.Pointer(c))
	}
	return nil
}
