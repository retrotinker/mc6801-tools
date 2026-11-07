Name:           mc6801-tools
Version:        2.1.0
Release:        2%{?dist}
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

%package doc
Summary:        Documentation and a demo program for %{name}
BuildArch:      noarch

%description doc
The user's manual for the mc6801-tools assembler, linker and binary
utilities, as a PDF, and a small modular demo program for the Tandy
MC-10 that shows how the tools are used together.

%prep
%autosetup

%build
%make_build CFLAGS="%{build_cflags}" LDFLAGS="%{build_ldflags}"

%install
%make_install PREFIX=%{_bindir} MANDIR=%{_mandir}

%check
test "$(as/as01 -v)" = "as01 version: v%{version}"
test "$(ld/ld01 -v)" = "ld01 version: v%{version}"
# The regression tests.
%make_build check
# Build the demo program with the new tools, in a copy so that the
# packaged demo stays clean.
cp -a demo demo-check
PATH="$PWD/as:$PWD/ld:$PWD/ar:$PATH" %make_build -C demo-check
test -s demo-check/main.wav

%files
%license COPYING
%doc README.md
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

%files doc
%license COPYING
%doc doc/mc6801-tools-manual.pdf demo

%changelog
* Sat Nov 07 2026 John W. Linville <linville@tuxdriver.com> - 2.1.0-2
- Move the user's manual and the demo program into a -doc subpackage

* Sat Nov 07 2026 John W. Linville <linville@tuxdriver.com> - 2.1.0-1
- Update to 2.1.0: as01 uses direct addressing for forward references to
  page zero (DPAGE symbols and EQU constants)

* Sat Nov 07 2026 John W. Linville <linville@tuxdriver.com> - 2.0.1-1
- Update to 2.0.1: .BSS symbols and sizes are reported as bss in object
  files and executables, and counter 1 symbols as text

* Sat Nov 07 2026 John W. Linville <linville@tuxdriver.com> - 2.0.0-1
- Update to 2.0.0: .BSS uses location counter 4 and is left out of the
  executable, with the common blocks; no padding between modules and no
  rounding of common blocks; as01 warnings on by default

* Sat Nov 07 2026 John W. Linville <linville@tuxdriver.com> - 1.6.1-1
- Update to 1.6.1: ld01 reports initialized data, or BYTE and WORD, in
  a NOLOAD section instead of silently leaving it out

* Sat Nov 07 2026 John W. Linville <linville@tuxdriver.com> - 1.6.0-1
- Update to 1.6.0: executables keep symbol names longer than 8
  characters (ld01 -y is the default), and objdump01 -d shows them

* Sat Nov 07 2026 John W. Linville <linville@tuxdriver.com> - 1.5.0-1
- Update to 1.5.0: EXCLUDE_FILE in ld01 linker script patterns; ORG and
  BLOCK in a relocatable module are now errors in as01

* Sat Nov 07 2026 John W. Linville <linville@tuxdriver.com> - 1.4.0-1
- Update to 1.4.0: objdump01 -d disassembles executables and object
  modules, showing relocated operands by name

* Sat Nov 07 2026 John W. Linville <linville@tuxdriver.com> - 1.3.1-1
- Update to 1.3.1: ld01 links an import to its definition whatever the
  order of the object files, warning if their location counters differ;
  as01 lists each error once; nm01 and size01 name themselves in errors

* Sat Nov 07 2026 John W. Linville <linville@tuxdriver.com> - 1.3.0-1
- Update to 1.3.0: the as01 DPAGE directive gives page-zero variables
  placed by a linker script direct addressing, and allows their
  addresses in 8-bit immediates and FCB data

* Sat Nov 07 2026 John W. Linville <linville@tuxdriver.com> - 1.2.4-1
- Update to 1.2.4: as01 and ld01 report one-byte addresses that don't
  fit in a byte, instead of silently truncating them; the regression
  tests run programs on an MC6801 simulator

* Sat Nov 07 2026 John W. Linville <linville@tuxdriver.com> - 1.2.3-1
- Update to 1.2.3: fix ar01 corrupting an archive after a failed run,
  as01 crashing on long -n names and rejecting long paths, and objdump01
  exiting 0 after errors; run the regression tests in %%check

* Fri Nov 06 2026 John W. Linville <linville@tuxdriver.com> - 1.2.2-1
- Update to 1.2.2: ar01 keeps the .o suffix when truncating a long
  member name given with a directory, and the build has no -Wall warnings

* Fri Nov 06 2026 John W. Linville <linville@tuxdriver.com> - 1.2.1-1
- Update to 1.2.1: report failed reads and writes in as01, ar01 and
  objdump01 instead of producing bad output, and build without warnings

* Wed Nov 04 2026 John W. Linville <linville@tuxdriver.com> - 1.2.0-1
- Update to 1.2.0: linker scripts for ld01, S-record and Intel hex output
  from ld01, regression tests, and fixes

* Sun Nov 01 2026 John W. Linville <linville@tuxdriver.com> - 1.1.3-1
- Update to 1.1.3: objcopy01 accepts c10 as another name for the cas format

* Sun Nov 01 2026 John W. Linville <linville@tuxdriver.com> - 1.1.2-1
- Initial package
