// Package registry provides a simple registry for available sqlean extensions.
// This package is introduced to break circular dependency between extension packages and the main package.
package registry

import "unsafe"

// ExtFunc represents an extension function defined in sqlean
type ExtFunc func(pointer unsafe.Pointer)

// global registry of extension functions
var extensions []ExtFunc

// Append extension function with global registry
func Append(fn ExtFunc) { extensions = append(extensions, fn) }

// Extensions returns a list of all registered extensions
func Extensions() []ExtFunc { return extensions }
