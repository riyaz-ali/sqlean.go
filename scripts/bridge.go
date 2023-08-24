package main

import (
	"flag"
	"fmt"
	"html/template"
	"log"
	"os"
	"path"
)

var bridge = template.Must(template.New("bridge.go").Parse(
	`package {{ .name }}

// #cgo CFLAGS: -DSQLITE_CORE -I${SRCDIR}/..
// #include "extension.h"
import "C"
import "unsafe"
import registry "github.com/riyaz-ali/sqlean.go/pkg/registry"

func init() {
  registry.Append(func(c unsafe.Pointer) { C.{{ .name }}_init((*C.struct_sqlite3)(c)) })
}
`,
))

var importer = template.Must(template.New("import.go").Parse(
	`//go:build !sqlean_omit_{{ .name }}
// +build !sqlean_omit_{{ .name }}

package sqlean

import _ "github.com/riyaz-ali/sqlean.go/pkg/extensions/{{ .name }}"

`))

func main() {
	var err error

	var name string
	flag.StringVar(&name, "name", "", "name of the extension")
	flag.Parse()

	if name == "" {
		flag.PrintDefaults()
		os.Exit(1)
	}

	var cwd, _ = os.Getwd()

	{ // write to pkg/extensions/<name>/<name>.go bridge file
		var filename = path.Join(cwd, "pkg/extensions", name, fmt.Sprintf("%s.go", name))
		var file *os.File
		if file, err = os.Create(filename); err != nil {
			log.Fatalf("failed to open file %s for writing: %v", filename, err)
		}
		defer file.Close()

		if err = bridge.Execute(file, map[string]string{"name": name}); err != nil {
			log.Fatalf("failed to write to bridge file %s: %v", filename, err)
		}
	}

	{ // write to <name>.go importer file
		var filename = path.Join(cwd, fmt.Sprintf("%s.go", name))
		var file *os.File
		if file, err = os.Create(filename); err != nil {
			log.Fatalf("failed to open file %s for writing: %v", filename, err)
		}
		defer file.Close()

		if err = importer.Execute(file, map[string]string{"name": name}); err != nil {
			log.Fatalf("failed to write to bridge file %s: %v", filename, err)
		}
	}
}
