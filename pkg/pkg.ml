#!/usr/bin/env ocaml
#use "topfind"
#require "topkg"
open Topkg


let () =
  Pkg.describe "tuntap" @@ fun c ->
  Ok [
    Pkg.mllib "lib/tuntap.mllib";
    Pkg.clib "lib/libtuntap_stubs.clib";
    Pkg.test "test/getifaddrs_test";
    Pkg.test "test/nonblock_read";
    Pkg.test "test/open_close_test";
    Pkg.test "test/sendfd_test";
    Pkg.test "test/set_ipv4_test";
  ]
