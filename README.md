# sqlean.go

This module provides `go` bindings for [**`sqlean`**](https://github.com/nalgeon/sqlean). It is compatible with the 
popular [`mattn/go-sqlite3`](https://github.com/mattn/go-sqlite3) driver.

## Usage

All sources from [a published release](https://github.com/nalgeon/sqlean/releases) are added to
[`pkg/extensions`](./pkg/extensions), and a bridge is generated using [`scripts/bridge.go`](./scripts/bridge.go), and vendored alongside the bindings. This makes
the package `go get`-able, though you'd still need to enable `cgo` for compilation.

```shell
go get -u github.com/riyaz-ali/sqlean.go
```

**Note**: The module is intended to be used in an application, and therefore be 
[statically linked to the `sqlite3` library](https://www.sqlite.org/loadext.html#statically_linking_a_run_time_loadable_extension).
To enforce that, there is a default `#define SQLITE_CORE` present in [`sqlean.go`](./sqlean.go). If you need to load it as an extension,
you are better off using the original sources / releases.

### Registering extensions

To register `sqlean` with an existing database connection, you need to invoke [`sqlean.Register()`](https://pkg.go.dev/github.com/riyaz-ali/sqlean.go#Register).
The following example shows how to use `sqlean.go` with [`mattn/go-sqlite3`](https://github.com/mattn/go-sqlite3).

```golang
import (
  "database/sql"
  "github.com/mattn/go-sqlite3"
  "github.com/riyaz-ali/sqlean.go"
  "reflect"
)

func init() {
  sql.Register("sqlean", &sqlite3.SQLiteDriver{
    ConnectHook: func(conn *sqlite3.SQLiteConn) error {
      // use reflection to get access to conn.db private field
      // need this until mattn/go-sqlite3#1155 gets resolved
      var db = reflect.Indirect(reflect.ValueOf(conn)).FieldByName("db").UnsafePointer()
      return sqlean.Register(sqlean.DatabaseConnection(db)) // sqlean.DatabaseConnection() is a type-cast
    },
  })
}

// you can then do sql.Open("sqlean", "...")
```

### Disabling extensions

Each extension binding file defines a `!sqlean_omit_<name>` build constraint. This creates a default opt-in build where all extensions
are enabled by default. To omit an extension, build with `-tags sqlean_omit_<name>`.

## What's included?

`sqlean.go` contains the following extensions:

- `crypto`: Hashing, encoding and decoding data
- `define`: User-defined functions and dynamic SQL
- `fileio`: Reading and writing files
- `fuzzy`: Fuzzy string matching and phonetics
- `ipaddr`: IP address manipulation
- `math`: Math functions
- `regexp`: Regular expressions
- `stats`: Math statistics
- `text`: String functions
- `unicode`: Unicode support
- `uuid`: Universally Unique IDentifiers
- `vsv`: CSV files as virtual tables

Newer extensions and / or versions are updated on best-effort basis. To add new extension, run:

```shell
VERSION=<version> ./scripts/download.sh
go run scripts/bridge.go --name <name> # name of the new extension
```

## Credits

Code under `pkg/extensions` is generated from
[source releases](https://github.com/nalgeon/sqlean/releases) and is licensed under MIT License.
Check the original license and copyright at [`nalgeon/sqlean/LICENSE`](https://github.com/nalgeon/sqlean/blob/main/LICENSE)
