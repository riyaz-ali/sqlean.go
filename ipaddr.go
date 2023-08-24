//go:build !sqlean_omit_ipaddr && !windows
// +build !sqlean_omit_ipaddr,!windows

package sqlean

import _ "github.com/riyaz-ali/sqlean.go/pkg/extensions/ipaddr"
