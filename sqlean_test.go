package sqlean_test

import (
	"database/sql"
	"github.com/mattn/go-sqlite3"
	"github.com/riyaz-ali/sqlean.go"
	"reflect"
	"testing"
)

func init() {
	sql.Register("sqlean", &sqlite3.SQLiteDriver{
		ConnectHook: func(conn *sqlite3.SQLiteConn) error {
			var db = reflect.Indirect(reflect.ValueOf(conn)).FieldByName("db").UnsafePointer()
			return sqlean.Register(sqlean.DatabaseConnection(db))
		},
	})
}

func Open(t *testing.T, url string) (db *sql.DB) {
	var err error
	if db, err = sql.Open("sqlean", url); err != nil {
		t.Fatalf("failed to open connection: %v", err)
	}
	return db
}

func TestSqleanCrypto_sha256(t *testing.T) {
	var db = Open(t, ":memory:")
	defer db.Close()

	var hash string
	if err := db.QueryRow("SELECT encode(sha256(?), 'hex')", "Hello World").Scan(&hash); err != nil {
		t.Errorf("failed to generate sha256 hash: %v", err)
	}

	t.Logf("sha256(%q) => %s", "Hello World", hash)
}

func TestSqleanDefine_plusone(t *testing.T) {
	var db = Open(t, ":memory:")
	defer db.Close()

	if _, err := db.Exec("select define('plus', ':x + 1')"); err != nil {
		t.Fatalf("failed to define function: %v", err)
	}

	rows, err := db.Query("select value, plus(value) from generate_series(1, 10)")
	if err != nil {
		t.Errorf("query failed: %v", err)
	}
	defer rows.Close()

	for rows.Next() {
		var value, plusOne int
		if err = rows.Scan(&value, &plusOne); err != nil {
			t.Errorf("rows.Scan(): %v", err)
		}

		t.Logf("value: %2d\tplusOne: %2d", value, plusOne)
	}

	if err = rows.Err(); err != nil {
		t.Errorf("rows.Err(): %v", err)
	}
}

func TestSqleanFileIO_ls(t *testing.T) {
	var db = Open(t, ":memory:")
	defer db.Close()

	const query = `select * from fileio_ls(?) order by name;`

	//cwd, _ := os.Getwd()
	rows, err := db.Query(query, ".")
	if err != nil {
		t.Fatalf("query failed: %v", err)
	}
	defer rows.Close()

	for rows.Next() {
		var name string
		var mode, mtime, size int
		if err = rows.Scan(&name, &mode, &mtime, &size); err != nil {
			t.Errorf("rows.Scan(): %v", err)
		}

		t.Logf("name: %-25s\tmode: %-5d\tmtime: %-4d\tsize: %8d", name, mode, mtime, size)
	}

	if err = rows.Err(); err != nil {
		t.Errorf("rows.Err(): %v", err)
	}
}

func TestSqleanFuzzy_levenshtein(t *testing.T) {
	var db = Open(t, ":memory:")
	defer db.Close()

	var distance int
	if err := db.QueryRow("select levenshtein(?, ?)", "awesome", "aewsme").Scan(&distance); err != nil {
		t.Errorf("query failed: %v", err)
	}

	t.Logf("levenshtein(%q, %q) => %d", "awesome", "aewsme", distance)
}

func TestSqleanIpAddr_ipnetwork(t *testing.T) {
	var db = Open(t, ":memory:")
	defer db.Close()

	var network string
	if err := db.QueryRow("select ipnetwork(?)", "192.168.16.12/24").Scan(&network); err != nil {
		t.Errorf("query failed: %v", err)
	}

	t.Logf("network() => %s", network)
}

func TestSqleanMath_sqrt(t *testing.T) {
	var db = Open(t, ":memory:")
	defer db.Close()

	var root int
	if err := db.QueryRow("SELECT sqrt(?)", 9).Scan(&root); err != nil {
		t.Errorf("query failed: %v", err)
	}

	t.Logf("sqrt(%d) => %d", 9, root)
}

func TestSqleanStats_median(t *testing.T) {
	var db = Open(t, ":memory:")
	defer db.Close()

	const query = `SELECT median(value) FROM generate_series(1, 100)`

	var median float64
	if err := db.QueryRow(query).Scan(&median); err != nil {
		t.Errorf("query failed: %v", err)
	}

	t.Logf("median() => %f", median)
}

func TestSqleanText_substr(t *testing.T) {
	var db = Open(t, ":memory:")
	defer db.Close()

	var substr string
	if err := db.QueryRow("SELECT text_substring(?, ?)", "Hello World", 7).Scan(&substr); err != nil {
		t.Errorf("query failed: %v", err)
	}

	t.Logf("text_substring(%s, %d) => %s", "Hello World", 7, substr)
}

func TestSqleanUnicode_unaccent(t *testing.T) {
	var db = Open(t, ":memory:")
	defer db.Close()

	var str string
	if err := db.QueryRow("SELECT unaccent(?)", "hôtel").Scan(&str); err != nil {
		t.Errorf("query failed: %v", err)
	}

	t.Logf("unaccent(%s) => %s", "hôtel", str)
}

func TestSqleanUuid_uuidv4(t *testing.T) {
	var db = Open(t, ":memory:")
	defer db.Close()

	var id string
	if err := db.QueryRow("SELECT uuid4()").Scan(&id); err != nil {
		t.Errorf("query failed: %v", err)
	}

	t.Logf("uuid() => %s", id)
}

func TestSqlean_Version(t *testing.T) {
	var db = Open(t, ":memory:")
	defer db.Close()

	var version string
	if err := db.QueryRow("SELECT sqlean_version()").Scan(&version); err != nil {
		t.Errorf("query failed: %v", err)
	}

	t.Logf("sqlean_version() => %s", version)
}
