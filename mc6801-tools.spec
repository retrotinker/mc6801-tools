Name:           mc6801-tools
Version:        1.1.3
Release:        1%{?dist}
Summary:        Assembler, linker and binary utilities for the Motorola MC6801

# ld/objcopy.c is GPL-2.0-only, ar/ar.c is GPL-2.0-or-later,
# ld/x86_aout.h is LGPL-2.0-or-later, and the rest is under the GPLv2 in COPYING.
License:        GPL-2.0-only AND GPL-2.0-or-later AND LGPL-2.0-or-later
URL:            https://github.com/retrotinker/mc6801-tools
Source0:        %{url}/archive/v%{version}/%{name}-%{version}.tar.gz

BuildRequires:  gcc
BuildRequires:  make

%description
Unix-style development tools for the Motorola MC6801 and MC6803
microprocessors, and for machines built around them such as the Tandy
MC-10:

 - as01, an assembler producing relocatable object modules or absolute
   binaries
 - ld01, a linker
 - ar01, to build and maintain libraries of object modules
 - objdump01 (also installed as nm01 and size01), to inspect object
   files, libraries and executables
 - objcopy01, to convert executables to S-records, Intel hex, raw
   binaries and cassette images

The tools are derived from the 6809 tools of Bruce Evans' C compiler, as
maintained in the dev86 package.

%prep
%autosetup

%build
%make_build CFLAGS="%{build_cflags}" LDFLAGS="%{build_ldflags}"

%install
%make_install PREFIX=%{_bindir} MANDIR=%{_mandir}

%check
test "$(as/as01 -v)" = "as01 version: v%{version}"
test "$(ld/ld01 -v)" = "ld01 version: v%{version}"
# Build the demo program with the new tools, in a copy so that the
# packaged demo stays clean.
cp -a demo demo-check
PATH="$PWD/as:$PWD/ld:$PWD/ar:$PATH" %make_build -C demo-check
test -s demo-check/main.wav

%files
%license COPYING
%doc README.md doc/mc6801-tools-manual.pdf demo
%{_bindir}/ar01
%{_bindir}/as01
%{_bindir}/ld01
%{_bindir}/nm01
%{_bindir}/objcopy01
%{_bindir}/objdump01
%{_bindir}/size01
%{_mandir}/man1/ar01.1*
%{_mandir}/man1/as01.1*
%{_mandir}/man1/ld01.1*
%{_mandir}/man1/nm01.1*
%{_mandir}/man1/objcopy01.1*
%{_mandir}/man1/objdump01.1*
%{_mandir}/man1/size01.1*

%changelog
* Sun Nov 01 2026 John W. Linville <linville@tuxdriver.com> - 1.1.3-1
- Update to 1.1.3: objcopy01 accepts c10 as another name for the cas format

* Sun Nov 01 2026 John W. Linville <linville@tuxdriver.com> - 1.1.2-1
- Initial package
