## 1.3.0 (07-Jun-2015):
* Do not leak a file descriptor per tun interface (#12 via Justin Cormack)
* Avoid the need for root access for persistent interfaces by not calling
  `SIOCSIFFLAGS` if not needed (#13 via Justin Cormack).
* Use centralised Travis scripts.
* Work around OS X bug in getifaddrs concerning lo0@ipv6 (#14)
* Force a default of non-blocking for the Linux tuntap file descriptor.
  This works around a kernel bug in 3.19+ that results in 0-byte reads
  causing processes to spin (https://bugzilla.kernel.org/show_bug.cgi?id=96381).
  Workaround is to open the device in nonblock mode, via Justin Cormack.

## 1.2.0 (09-Jan-2015):
* `set_ipaddr` renamed to `set_ipv4` since it can only set IPv4 addresses.
* Improved `getifaddrs` interface to an association list iface -> addr.
* Dropped OCaml < 4.01.x support.
* Added convenience functions `gettifaddrs_v{4,6}`, `v{4,6}_of_ifname`.

## 1.1.0 (25-Nov-2014):
* Do not change the `persist` setting if unspecified when
  opening a new tun interface (#9 from Luke Dunstan).

## 1.0.0 (02-Mar-2014):
* Improve error messages to distinguish where they happen.
* Install otunctl command-line tool to create persistent tun/taps.
* Build debug symbols, annot and bin_annot files by default.
* getifaddrs now lists IPv6 as well, and return a new type.
* set_ipv6 is now called set_ipaddr, and will support IPv6 in the
  future (currently unimplemented).

## 0.7.0 (28-Sep-2013):
* Add FreeBSD support.
* Add Travis continuous integration scripts.

## 0.6 (07-Aug-2013):
* Remove dependency on cstruct
* Add dependency on ipaddr
* Removed redundant functions (now in ipaddr)

## 0.5 (30-May-2013):
* Add a non-blocking packet dumper test.
* New function getifaddrs, binding to getifaddrs(3).
* New version of tunctl, using cmdliner.
* Add a set_ipv4 test to check the behaviour of set_ipv4.

## 0.4 (25-May-2013):
* Fixed MacOS X tuntap support.

## 0.3 (22-May-2013):
* First public release.
