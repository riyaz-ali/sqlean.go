package main

import (
	"archive/tar"
	"bufio"
	"bytes"
	"compress/gzip"
	"flag"
	"fmt"
	"io"
	"log"
	"net/http"
	"os"
	"path"
	"regexp"
	"strings"
	"time"
)

var (
	version    string // version of sqlean to download
	headerFile string // name of the header file to write to
	sourceFile string // name of the source file to write to
)

func init() {
	flag.StringVar(&version, "version", "0.21.6", "version of sqlean to download")
	flag.StringVar(&headerFile, "header", "sqlean.h", "name of the header file")
	flag.StringVar(&sourceFile, "source", "sqlean.c", "name of the source file")
}

// extensions to be skipped
var skip = []string{"src/regexp", "src/fuzzy"}

type File struct {
	Path    string
	Content []byte
}

// download tar.gz source archive from remote, open it and read its content into memory
func download(src string) (_ []*File, err error) {
	var resp *http.Response
	if resp, err = http.Get(src); err != nil {
		return nil, err
	}
	defer resp.Body.Close()

	if resp.StatusCode != http.StatusOK {
		return nil, fmt.Errorf("failed to fetch: server returned %d", resp.StatusCode)
	}

	var compressed, _ = gzip.NewReader(resp.Body)
	var bundle = tar.NewReader(compressed)

	var files []*File
	var entry *tar.Header
	for entry, err = bundle.Next(); err == nil; entry, err = bundle.Next() {
		if entry.Typeflag == tar.TypeReg {
			var buf bytes.Buffer
			if _, err = buf.ReadFrom(bundle); err != nil {
				return nil, err
			}

			files = append(files, &File{Path: entry.Name, Content: buf.Bytes()})
		}
	}

	if err == io.EOF {
		err = nil
	}

	return files, nil
}

// filters files based on provided predicate function
func filter(files []*File, fn func(*File) bool) (ret []*File) {
	for _, file := range files {
		if fn(file) {
			ret = append(ret, file)
		}
	}
	return
}

// groups files based on provided predicate function
func group(files []*File, fn func(*File) string) map[string][]*File {
	var ret = make(map[string][]*File)
	for _, file := range files {
		var g = fn(file)
		ret[g] = append(ret[g], file)
	}
	return ret
}

// shorthand for fmt.Fprintln
func write(w io.Writer, str string) { _, _ = fmt.Fprintln(w, str) }
func writeln(w io.Writer)           { _, _ = fmt.Fprintln(w, "") }

// shorthand for fmt.Sprintf
func sp(format string, args ...interface{}) string { return fmt.Sprintf(format, args...) }

// replaces pattern from buf and returns the resulting code as string
func replace(buf []byte, pattern string) string {
	var re = regexp.MustCompile(pattern)
	var scanner = bufio.NewScanner(bytes.NewReader(buf))

	var builder strings.Builder

	for scanner.Scan() {
		if txt := scanner.Text(); !re.MatchString(txt) {
			write(&builder, txt)
		}
	}

	return builder.String()
}

// writes preamble to the given writer
func preamble(w io.Writer) {
	write(w, "// ---------------------------------")
	write(w, sp("// Following is an amalgamated version of sqlean v%s", version))
	write(w, "// License @ https://github.com/nalgeon/sqlean/blob/main/LICENSE")
	write(w, "// Find more details @ https://github.com/nalgeon/sqlean")
	write(w, "// All copyrights belong to original author(s)")
	write(w, "// ---------------------------------")
	writeln(w)
}

func main() {
	flag.Parse()
	var src = fmt.Sprintf("https://github.com/nalgeon/sqlean/archive/refs/tags/%s.tar.gz", version)
	var err error

	var files []*File
	if files, err = download(src); err != nil {
		log.Fatalf("failed to download %q: %v", src, err)
	}

	for i, file := range files {
		files[i].Path = strings.TrimPrefix(file.Path, fmt.Sprintf("sqlean-%s/", version))
	}

	// filter only source files
	files = filter(files, func(file *File) (ok bool) {
		if ok, _ = path.Match("src/**/*.c", file.Path); !ok {
			ok, _ = path.Match("src/**/*.h", file.Path)
		}
		return ok
	})

	// remove files from skipped directories
	files = filter(files, func(file *File) bool {
		for _, sk := range skip {
			if strings.HasPrefix(file.Path, sk) {
				return false
			}
		}
		return true
	})

	var headers = filter(files, func(file *File) bool { return strings.HasSuffix(file.Path, ".h") })
	{ // write sqlean.h header file
		var buf bytes.Buffer
		preamble(&buf)

		write(&buf, "#ifndef SQLEAN_H")
		write(&buf, "#define SQLEAN_H")
		writeln(&buf)

		// make sure we can call this stuff from c++
		write(&buf, "#ifdef __cplusplus")
		write(&buf, `extern "C" {`)
		write(&buf, "#endif")
		writeln(&buf)

		write(&buf, "#include <sqlite3.h>")
		write(&buf, "#include <sqlite3ext.h>")
		write(&buf, "#include <stddef.h>")
		writeln(&buf)

		// write version info
		write(&buf, sp("#define SQLEAN_VERSION %q", version))
		write(&buf, sp("#define SQLEAN_GENERATE_TIMESTAMP %q", time.Now().Format(time.RFC3339)))
		writeln(&buf)

		var g = group(headers, func(file *File) string { return strings.SplitN(file.Path, "/", 3)[1] })
		for name, hdrs := range g {
			write(&buf, sp("#ifdef SQLEAN_ENABLE_%s", strings.ToUpper(name)))
			for _, hdr := range hdrs {
				write(&buf, "// ---------------------------------")
				write(&buf, sp("// %s", hdr.Path))
				write(&buf, "// ---------------------------------")
				write(&buf, replace(hdr.Content, `#include\s+"[^"]+`))
			}
			write(&buf, sp("#endif // SQLEAN_ENABLE_%s", strings.ToUpper(name)))
		}

		writeln(&buf)
		write(&buf, "// add sqlean_version() sql function that returns the current version of sqlean")
		write(&buf, "void sqlean_version(sqlite3_context* context, int argc, sqlite3_value** argv);")
		writeln(&buf)

		write(&buf, "#ifdef __cplusplus")
		write(&buf, "}")
		write(&buf, "#endif")

		writeln(&buf)
		write(&buf, "#endif  // SQLEAN_H")

		if err = os.WriteFile(headerFile, buf.Bytes(), 0666); err != nil {
			log.Fatalf("failed to write to %s: %v", headerFile, err)
		}
	}

	var sources = filter(files, func(file *File) bool { return strings.HasSuffix(file.Path, ".c") })
	{ // write sqlean.c source file
		var buf bytes.Buffer
		preamble(&buf)

		// include
		write(&buf, sp(`#include %q`, headerFile))
		writeln(&buf)

		// make sure we can call this stuff from c++
		write(&buf, "#ifdef __cplusplus")
		write(&buf, `extern "C" {`)
		write(&buf, "#endif")
		writeln(&buf)

		var g = group(sources, func(file *File) string { return strings.SplitN(file.Path, "/", 3)[1] })
		for name, srcs := range g {
			write(&buf, sp("#ifdef SQLEAN_ENABLE_%s", strings.ToUpper(name)))

			for _, s := range srcs {
				write(&buf, "// ---------------------------------")
				write(&buf, sp("// %s", s.Path))
				write(&buf, "// ---------------------------------")
				write(&buf, replace(s.Content, `#include\s+"[^"]+`))
			}

			write(&buf, sp("#endif // SQLEAN_ENABLE_%s", strings.ToUpper(name)))
		}

		writeln(&buf)
		write(&buf, "// add sqlean_version() sql function that returns the current version of sqlean")
		write(&buf, "void sqlean_version(sqlite3_context* context, int argc, sqlite3_value** argv) {")
		write(&buf, "  sqlite3_result_text(context, SQLEAN_VERSION, -1, SQLITE_STATIC);")
		write(&buf, "}")
		writeln(&buf)

		write(&buf, "#ifdef __cplusplus")
		write(&buf, "}")
		write(&buf, "#endif")

		if err = os.WriteFile(sourceFile, buf.Bytes(), 0666); err != nil {
			log.Fatalf("failed to write to %s: %v", sources, err)
		}
	}
}
