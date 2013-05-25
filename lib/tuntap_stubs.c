/*
 * Copyright (c) 2010-2013 Anil Madhavapeddy <anil@recoil.org>
 * Copyright (c) 2013 Vincent Bernardoff <vb@luminar.eu.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <caml/mlvalues.h>
#include <caml/memory.h>
#include <caml/alloc.h>
#include <caml/fail.h>

#if defined(__linux__)
#include <linux/if_tun.h>

static int
tun_alloc(char *dev, int kind, int pi, int persist, int user, int group)
{
  struct ifreq ifr;
  int fd;

  if ((fd = open("/dev/net/tun", O_RDWR)) == -1) {
    perror("open");
    caml_failwith("Unable to open /dev/net/tun");
  }

  memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_flags = 0;
  ifr.ifr_flags |= (kind ? IFF_TUN : IFF_TAP);
  ifr.ifr_flags |= (pi ? 0 : IFF_NO_PI);

  if (*dev)
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);

  if (ioctl(fd, TUNSETIFF, (void *)&ifr) < 0) {
    close(fd);
    perror("TUNSETIFF");
    caml_failwith("TUNSETIFF");
  }

  if(ioctl(fd, TUNSETPERSIST, persist) < 0) {
    close(fd);
    perror("TUNSETPERSIST");
    caml_failwith("TUNSETPERSIST");
  }

  if(user != -1) {
    if(ioctl(fd, TUNSETOWNER, user) < 0) {
      close(fd);
      perror("TUNSETOWNER");
      caml_failwith("TUNSETOWNER");
    }
  }

  if(group != -1) {
    if(ioctl(fd, TUNSETGROUP, group) < 0) {
      close(fd);
      perror("TUNSETGROUP");
      caml_failwith("TUNSETGROUP");
    }
  }

  strcpy(dev, ifr.ifr_name);
  return fd;
}

CAMLprim value
get_hwaddr(value devname) {
  CAMLparam1(devname);
  CAMLlocal1(hwaddr);

  int fd;
  struct ifreq ifq;

  fd = socket(PF_INET, SOCK_DGRAM, 0);
  strcpy(ifq.ifr_name, String_val(devname));
  if (ioctl(fd, SIOCGIFHWADDR, &ifq) == -1)
    perror("SIOCIFHWADDR");

  hwaddr = caml_alloc_string(6);
  memcpy(String_val(hwaddr), ifq.ifr_hwaddr.sa_data, 6);

  CAMLreturn (hwaddr);
}

#elif defined(__APPLE__) && defined(__MACH__)
#include <net/if_dl.h>
#include <ifaddrs.h>

static int
tun_alloc(char *dev, int kind, int pi, int persist, int user, int group)
{
  // On MacOSX, we need that dev is not NULL, and all options other
  // than kind are to be ignored because not supported.
  char name[IFNAMSIZ];
  int fd;

  snprintf(name, sizeof name, "/dev/%s", String_val(dev));

  fd = open(name, O_RDWR);
  if (fd == -1)
    {
      fprintf(stderr, "%s\n", name);
      perror("open");
      caml_failwith("Unable to open the TUN or TAP interface");
    }

  return fd;
}

CAMLprim value
get_hwaddr(value devname) {
  CAMLparam1(devname);
  CAMLlocal1(v_mac);

  struct ifaddrs *ifap, *p;
  char *mac_addr[6];
  int found = 0;
  char name[IFNAMSIZ];
  snprintf(name, sizeof name, "%s", String_val(devname));

  if (getifaddrs(&ifap) != 0) {
    err(1, "get_mac_addr");
  }

  for(p = ifap; p != NULL; p = p->ifa_next) {
    if((strcmp(p->ifa_name, name) == 0) &&
      (p->ifa_addr != NULL)){
      char *tmp = LLADDR((struct sockaddr_dl *)(p)->ifa_addr);
      memcpy(mac_addr, tmp, 6);
      found = 1;
      break;
    }
  }

  freeifaddrs(ifap);
  if (!found)
    err(1, "get_mac_addr");

  v_mac = caml_alloc_string(6);
  memcpy(String_val(v_mac), mac_addr, 6);
  CAMLreturn (v_mac);
}
#endif

// Code for all architectures

CAMLprim value
set_up_and_running(value dev)
{
  CAMLparam1(dev);

  int fd;
  struct ifreq ifr;

  if((fd = socket(PF_INET, SOCK_DGRAM, 0)) == -1)
    {
      perror("socket");
      caml_failwith("Impossible to open socket");
    }

  strncpy(ifr.ifr_name, String_val(dev), IFNAMSIZ);
  ifr.ifr_addr.sa_family = AF_INET;

  if (ioctl(fd, SIOCGIFFLAGS, &ifr) == -1)
    {
      perror("SIOCGIFFLAGS");
      caml_failwith("SIOCGIFFLAGS");
    }

  strncpy(ifr.ifr_name, String_val(dev), IFNAMSIZ);

  ifr.ifr_flags |= (IFF_UP|IFF_RUNNING|IFF_BROADCAST|IFF_MULTICAST);

  if (ioctl(fd, SIOCSIFFLAGS, &ifr) == -1)
    {
      perror("SIOCSIFFLAGS");
      caml_failwith("SIOCSIFFLAGS");
    }

  CAMLreturn(Val_unit);
}

CAMLprim value
set_ipv4(value dev, value ipv4, value netmask)
{
  CAMLparam3(dev, ipv4, netmask);

  int fd;
  struct ifreq ifr;
  struct sockaddr_in* addr = (struct sockaddr_in*)&ifr.ifr_addr;

  memset(&ifr, 0, sizeof(struct ifreq));

  if((fd = socket(PF_INET, SOCK_DGRAM, 0)) == -1)
    {
      perror("socket");
      caml_failwith("Impossible to open socket");
    }

  strncpy(ifr.ifr_name, String_val(dev), IFNAMSIZ);
  ifr.ifr_addr.sa_family = AF_INET;
  inet_pton(AF_INET, String_val(ipv4), &(addr->sin_addr));

  if (ioctl(fd, SIOCSIFADDR, &ifr) == -1)
    {
      perror("SIOCSIFADDR");
      caml_failwith("SIOCSIFADDR");
    }

  if(caml_string_length(netmask) > 0)
    {
      inet_pton(AF_INET, String_val(netmask), &(addr->sin_addr));

      if (ioctl(fd, SIOCSIFNETMASK, &ifr) == -1)
        {
          perror("SIOCSIFNETMASK");
          caml_failwith("SIOCSIFNETMASK");
        }
    }

  // Set interface up and running
  set_up_and_running(dev);

  CAMLreturn(Val_unit);
}

CAMLprim value
tun_opendev(value devname, value kind, value pi, value persist, value user, value group)
{
  CAMLparam5(devname, kind, pi, persist, user);
  CAMLxparam1(group);
  CAMLlocal2(res, dev_caml);

  char dev[IFNAMSIZ];
  int fd;

#if defined (__APPLE__) && defined (__MACH__)
  if (caml_string_length(devname) < 4)
    caml_failwith("On MacOSX, you need to specify the name of the device, e.g. tap0");
#endif

  memset(dev, 0, sizeof dev);
  memcpy(dev, String_val(devname), caml_string_length(devname));

  // All errors are already checked by tun_alloc, returned fd is valid
  // otherwise it would have crashed before
  fd = tun_alloc(dev, Int_val(kind), Bool_val(pi), Bool_val(persist), Int_val(user), Int_val(group));

  res = caml_alloc_tuple(2);
  dev_caml = caml_copy_string(dev);

  Store_field(res, 0, Val_int(fd));
  Store_field(res, 1, dev_caml);

  CAMLreturn(res);
}

CAMLprim value
tun_opendev_byte(value *argv, int argn)
{
  return tun_opendev(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5]);
}

CAMLprim value
get_ifnamsiz()
{
  CAMLparam0();
  CAMLreturn(Val_int(IFNAMSIZ));
}
