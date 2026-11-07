#!/bin/sh
#
# Regression tests for the mc6801-tools.
#
# Assembles the test sources and the demo, links them in various ways,
# and compares every result (object files, executables, link maps,
# messages and exit statuses) with the reference copies in ref/.
#
# usage: run.sh [-u]
#   -u  update the reference copies from this build instead of comparing

top=$(cd "$(dirname "$0")/.." && pwd)
tests=$top/tests
out=$tests/out
ref=$tests/ref

AS=$top/as/as01
LD=$top/ld/ld01
AR=$top/ar/ar01
OD=$top/ld/objdump01
OC=$top/ld/objcopy01
SIM=$tests/sim/sim6801
CASDEC=$tests/sim/casdec

update=0
if [ "${1:-}" = "-u" ]; then
	update=1
fi

rm -rf "$out"
mkdir -p "$out"
cd "$out" || exit 1

# assemble NAME SOURCE: object file, listing and symbol file
assemble() {
	"$AS" -w- -o "$1.o" -l "$1.lst" -s "$1.sym" "$2" > /dev/null 2>&1
	echo $? > "$1.as.rc"
}

# link NAME OPTION...: executable, -m/-M map, messages and exit status
# (the map's last line, the linker's own memory use, isn't compared)
link() {
	name=$1
	shift
	"$LD" "$@" -o "$name.out" > "$name.map.tmp" 2> "$name.err"
	echo $? > "$name.rc"
	sed '/^Total memory used: /d' "$name.map.tmp" > "$name.map"
	rm -f "$name.map.tmp"
}

for f in "$top"/demo/*.s "$tests"/src/*.s; do
	assemble "$(basename "$f" .s)" "$f"
done

"$AR" rc libnum.a one.o two.o three.o 2> ar.err

# the demo program, as built by demo/Makefile, and variants
link demo	-m -M -lnum -Cstart -T0x434c main.o screen.o
link demo-s	-s -lnum -Cstart -T0x434c main.o screen.o
link demo-d	-d -lnum -Cstart -T0x434c main.o screen.o

# symbol names longer than 8 characters (msg_three) are kept whole,
# unless -y- asks for the old table, which cuts them
link demo-short	-y- -lnum -Cstart -T0x434c main.o screen.o
for f in demo demo-short; do
	"$OD" -n $f.out > $f.nm
done

# several location counters, common blocks, relocations between
# modules and counters
link segs	-m -M -T0 segs.o helper.o
link segs-TD	-m -M -T0x1000 -D0x8000 segs.o helper.o
link segs-i	-m -M -i -T0x1000 segs.o helper.o
link segs-H	-m -M -T0xe000 -H0x100 segs.o helper.o
link segs-d	-d -T0xe000 segs.o helper.o

# addresses that aren't a multiple of 4 (before per-module placement,
# symbol values and code disagreed in this case)
link segs-odd	-m -M -T0x1001 -D0x8003 segs.o helper.o
link segs-oddd	-d -T0x1001 segs.o helper.o

# location counter 2 moved to the next page
link page	-m -M -T0 page.o

# entry point selection
link mainent	-m -M mainent.o
link noentry	noentry.o

# linker scripts (copied here, so messages name them without a path)
cp "$tests"/scripts/*.ld .
link custom	-m -M -S custom.ld usesym.o segs.o helper.o
link custom-A	-m -M -S custom.ld -A __heap_top=0x6000 -A extra=7 \
		usesym.o segs.o helper.o
"$OD" -n custom.out > custom.nm	# __heap_top: a long script symbol
link orphan	-m -M -S orphan.ld segs.o helper.o
link orphan-Z	-Z -S orphan.ld segs.o helper.o
link orphan2	-m -M -S orphan2.ld segs.o helper.o
for f in syntax undef overlap range both settle entry unknown; do
	link err-$f -S err-$f.ld segs.o helper.o
done
link err-onepage -S err-onepage.ld page.o

# ROM layout: MEMORY, AT>, NOLOAD, KEEP, FILL, BYTE, WORD, ASSERT,
# INCLUDE, and the __name_load symbols
link rom	-m -M -S rom.ld romcrt.o romvec.o segs.o helper.o
link rom-srec	-S rom-srec.ld romcrt.o romvec.o segs.o helper.o
link rom-ihex	-S rom-ihex.ld romcrt.o romvec.o segs.o helper.o
link design-rom	-m -M -S design-rom.ld crt0.o vectors.o
link bss	-m -M -S bss.ld segs.o helper.o
link err-assert	-S rom-nokeep.ld romcrt.o romvec.o segs.o helper.o
for f in at-aout region noregion lmaoverlap; do
	link err-$f -S err-$f.ld segs.o helper.o
done

# 2.0's default layout: .BSS (counter 4) and the common blocks are
# uninitialized data, not in the file; .bss in a pattern means the same
# as the default layout; initialized data under .BSS is an error; and
# 1.6's default script still gives 1.6's layout (demo-1.6.out is the
# same as 1.6's demo.out)
link bssprog	-m -M -T 0x1000 bssprog.o
link bss-pat	-S bss-pat.ld bssprog.o
if cmp -s bssprog.out bss-pat.out; then
	echo "ok   bss-pat.out is the same as bssprog.out"
else
	echo "DIFF bss-pat.out and bssprog.out"
fi > bss-pat.result
rm -f bss-pat.out
link bssinit	-T 0x1000 bssinit.o
link demo-1.6	-S default-1.6.ld -lnum -Cstart main.o screen.o

# initialized data in a NOLOAD section, which isn't written, is an
# error (once for each module and counter); reserved space isn't, and
# BYTE or WORD there is a script error
link err-noload	-S err-noload.ld noload.o
link noload-ok	-m -M -S noload-ok.ld noload.o
link err-noload-byte	-S err-noload-byte.ld noload.o

# EXCLUDE_FILE: the ROM layout with the vector table's section after
# .text, which excludes it, gives the same output as rom.ld; input that a
# section excludes and nothing else takes is an orphan, placed after a
# section that takes all files; and two syntax errors
link rom-excl	-m -M -S rom-excl.ld romcrt.o romvec.o segs.o helper.o
if cmp -s rom.out rom-excl.out; then
	echo "ok   rom-excl.out is the same as rom.out"
else
	echo "DIFF rom-excl.out and rom.out"
fi > excl.result
link excl-orphan	-m -M -S excl-orphan.ld segs.o helper.o
for f in excl1 excl2; do
	link err-$f -S err-$f.ld segs.o helper.o
done

# one-byte relocations (forced direct addresses, indexed offsets): in
# page zero they link; elsewhere each is an error naming the module, or
# the library member, the target and the address
link dp-zp	-m -M -S dpzp.ld dpref.o dpvars.o
link dp-far	-T 0xE000 -D 0x5000 dpref.o dpvars.o
"$AR" rc libdp.a dpref.o
link dp-lib	-T 0xE000 -D 0x5000 dpcall.o libdp.a dpvars.o
rm -f libdp.a

# an import under a different counter from its definition, and of an
# EQU constant: the definition wins, whichever module comes first, with
# a warning naming both counters (only for the variable); before, with
# the import first, the symbol ended up undefined
link imp-ab	-m -T 0x1000 impuse.o impdef.o
link imp-ba	-m -T 0x1000 impdef.o impuse.o

# DPAGE: direct addresses, 8-bit immediates and FCB data with page-zero
# addresses; with counter 5 in page zero they link, elsewhere each is
# an error
link dpage	-m -M -S dpage.ld dpage.o dpshared.o
link dpage-far	-T 0xE000 -D 0x5000 dpage.o dpshared.o

# the default script, as printed by -P, and linking with it: the output
# must be the same as without a script, with or without the options
"$LD" -P > default.ld 2> default.err
echo $? > default.rc
"$LD" -P -S custom.ld > p-and-s.out 2> p-and-s.err
echo $? > p-and-s.rc
roundtrip() {
	rt=$1		# (link() uses $name)
	shift
	"$LD" -P "$@" > "$rt-P.ld"
	link "$rt-P" -S "$rt-P.ld" "$@" $objs
	link "$rt-Ponly" -S "$rt-P.ld" $flags $objs
	for v in P Ponly; do
		if cmp -s "$rt.out" "$rt-$v.out" &&
		   cmp -s "$rt.map" "$rt-$v.map" &&
		   cmp -s "$rt.rc" "$rt-$v.rc"; then
			echo "ok   $rt-$v"
		else
			echo "DIFF $rt-$v"
		fi
		rm -f "$rt-$v.out" "$rt-$v.map" "$rt-$v.err" "$rt-$v.rc"
	done >> roundtrip.result
}
objs="-lnum -Cstart main.o screen.o" flags="-m -M"
roundtrip demo -m -M -T0x434c
objs="segs.o helper.o"
roundtrip segs -m -M -T0
roundtrip segs-TD -m -M -T0x1000 -D0x8000
flags="-m -M -i"
roundtrip segs-i -m -M -i -T0x1000
flags="-m -M"
roundtrip segs-H -m -M -T0xe000 -H0x100
flags="-d"
roundtrip segs-d -d -T0xe000
flags="-m -M"
roundtrip segs-odd -m -M -T0x1001 -D0x8003
objs="page.o"
roundtrip page -m -M -T0
objs="mainent.o"
roundtrip mainent -m -M

# the linker script examples in the user's manual, linked as it shows,
# in a directory of their own since they use the same file names as other
# tests; their results are then renamed manual-*
mkdir manual
cd manual || exit 1
cp "$tests"/manual/* .
for f in *.s "$top"/demo/crtstart.s "$top"/demo/main.s "$top"/demo/screen.s \
    "$top"/demo/one.s "$top"/demo/two.s "$top"/demo/three.s; do
	assemble "$(basename "$f" .s)" "$f"
done
"$AR" rc libnum.a one.o two.o three.o 2> ar.err
link mc10	-M -S mc10.ld -lnum crtstart.o main.o screen.o
link rom	-M -S rom.ld crt0.o vectors.o prog.o
"$OC" -O srec -n MAIN mc10.out mc10.srec > objcopy.err 2>&1
echo $? > objcopy.rc
"$AR" t libnum.a > libnum.t
rm -f libnum.a
# each example file must appear, word for word, in the manual
for f in "$tests"/manual/*; do
	if awk -v RS='\001' 'NR == FNR { m = $0; next }
	    { exit index(m, $0) ? 0 : 1 }' "$top/doc/manual.md" "$f"; then
		echo "ok   $(basename "$f")"
	else
		echo "DIFF $(basename "$f") isn't in doc/manual.md"
	fi
done > insync.result
cd ..
for f in manual/*; do
	mv "$f" "manual-$(basename "$f")"
done
rmdir manual

# objdump01 -d: object modules (relocated operands by name, data in
# code) and an executable (labels, calls and branches by name)
for f in disasm dpage romcrt one; do
	"$OD" -d $f.o | sed -n '/^DISASSEMBLY/,$p' > $f.dis
done
"$OD" -d demo.out | sed -n '/^DISASSEMBLY/,$p' > demo.dis

# warnings are on by default, and -w turns them off; either way the
# exit status is the same
"$AS" -o warn-on.o -l warn-on.lst "$tests"/src/warn.s > warn-on.out 2>&1
echo $? > warn-on.rc
"$AS" -w -o warn-off.o -l warn-off.lst "$tests"/src/warn.s > warn-off.out 2>&1
echo $? > warn-off.rc
rm -f warn-on.o warn-off.o

# relocatable output: an archive, compared by its contents since the
# member headers contain file dates
"$LD" -r -o rel.a segs.o helper.o > rel.err 2>&1
echo $? > rel.rc
"$OD" rel.a > rel.dump 2>&1
rm -f rel.a

# a file that isn't an object file: an error, and exit status 1
echo "not an object file" > notobj.txt
"$OD" notobj.txt > notobj.dump 2>&1
echo $? > notobj.rc
"$OD" -n notobj.txt > notobj-n.dump 2>&1
echo $? > notobj-n.rc
rm -f notobj.txt

# conversions of the demo executable
for f in srec ihex cas wav; do
	"$OC" -O $f -n MAIN demo.out demo.$f > /dev/null 2>&1
	echo $? > demo.$f.rc
done

# programs run on the MC6801 simulator: the output is the registers and
# memory at the end (and any characters printed), and the exit status

# its instruction decoding must match the assembler's: every instruction
# in every addressing mode, at the addresses in as01's listing (the
# result is the differences, so it should be empty, and the count)
"$AS" -w- -b allops.bin -l allops.lst "$tests"/sim/allops.s > /dev/null 2>&1
awk -F'\t' '/^[0-9][0-9][0-9][0-9][0-9] [0-9A-F][0-9A-F][0-9A-F][0-9A-F] / {
	split($1, f, " "); print f[2], f[3], toupper($2) }' allops.lst > allops.as
"$SIM" -L -a 0 allops.bin | awk '{ print $1, $2, $3 }' > allops.sim
diff allops.as allops.sim > sim-allops.diff
wc -l < allops.as | tr -d ' ' > sim-allops.count
# objdump01 -d uses the same table, on an object file: it must agree too
"$AS" -w- -o allops.o "$tests"/sim/allops.s > /dev/null 2>&1
"$OD" -d allops.o | sed -n '/^DISASSEMBLY/,$p' | awk '
	/^[0-9a-f][0-9a-f][0-9a-f][0-9a-f]  / {
		m = substr($0, 26, 6); sub(/ +$/, "", m)
		print toupper(substr($0, 1, 4)), toupper(substr($0, 7, 2)), toupper(m) }' \
	> allops.od
diff allops.as allops.od > od-allops.diff
rm -f allops.bin allops.lst allops.as allops.sim allops.o allops.od

# instructions and their condition codes, worked out from the data sheet
# (the sources' "expect" comments)
for t in arith branch stack; do
	"$AS" -w- -b $t.bin "$tests"/sim/$t.s > /dev/null 2>&1
	"$SIM" -a 0x1000 -e 0x1000 -d 0x0080-0x00CF $t.bin \
		> sim-$t.run 2>&1
	echo $? > sim-$t.rc
	rm -f $t.bin
done

# the demo, called like the MC-10's EXEC, printing through the ROM's
# output routine (its address is at $FFDE): ONE, TWO and THREE, and a
# cleared screen
"$SIM" -s 0x4FFF -r 0xFF00 -w 0xFFDE=0xFF10 -p 0xFF10 -d 0x4000-0x401F \
	demo.srec > sim-demo.run 2>&1
echo $? > sim-demo.rc

# the user's manual's ROM example, from the reset vector: its data copied
# to RAM, and 45 times round main's loop
"$SIM" -n 200 -d 0x0000-0x0003 -d 0x0080-0x008C -d 0x00F8-0x00FF \
	manual-rom.out > sim-manual-rom.run 2>&1
echo $? > sim-manual-rom.rc

# dpref.s linked with its data in page zero: each one-byte reference
# reads the right byte (A=$99 from local+2, after $17 and B=$42)
"$SIM" -n 4 dp-zp.out > sim-dp-zp.run 2>&1
echo $? > sim-dp-zp.rc

# dpage.s linked with counter 5 in page zero: the page-zero data it
# changed, its registers, and its table of addresses
"$SIM" -d 0x0080-0x008F -d 0xE024-0xE029 dpage.out > sim-dpage.run 2>&1
echo $? > sim-dpage.rc

# bssprog.s: its uninitialized data starts as garbage ($AA), and the
# start-up code clears it before the program stores its values
"$OC" -O srec bssprog.out bssprog.srec > /dev/null 2>&1
bss=$("$OD" -n bssprog.out | awk '$3 == "__bss_start" { print $1 }')
bss=$((0x$bss))
"$SIM" -w $bss=0xAAAA -w $((bss + 2))=0xAAAA -w $((bss + 4))=0xAAAA \
	-d $((bss - 1))-$((bss + 6)) bssprog.srec > sim-bssprog.run 2>&1
echo $? > sim-bssprog.rc
rm -f bssprog.srec

# the other output formats, run rather than just compared: the demo
# from objcopy01's Intel hex, and decoded from its cassette file and
# audio (as the MC-10 would load them), runs as from its S-records; so
# does castest.s, which takes several cassette blocks; and ld01's Intel
# hex output loads the same bytes as its S-records
demo_run() {
	"$SIM" -s 0x4FFF -r 0xFF00 -w 0xFFDE=0xFF10 -p 0xFF10 \
		-d 0x4000-0x401F "$@" 2>&1
}
same() {	# NAME FILE1 FILE2
	if cmp -s "$2" "$3"; then
		echo "ok   $1"
	else
		echo "DIFF $1"
	fi
}
{
	demo_run demo.ihex > run.tmp
	same "demo.ihex runs as demo.srec" sim-demo.run run.tmp
	"$CASDEC" demo.cas > dec.srec 2> demo-cas.dec
	demo_run dec.srec > run.tmp
	same "demo.cas, decoded, runs as demo.srec" sim-demo.run run.tmp
	"$CASDEC" -w demo.wav > dec.srec 2> demo-wav.dec
	demo_run dec.srec > run.tmp
	same "demo.wav, decoded, runs as demo.srec" sim-demo.run run.tmp

	"$LD" -T 0x1000 -o castest.out castest.o
	for f in srec cas wav; do
		"$OC" -O $f -n CASTEST castest.out castest.$f
	done
	"$SIM" castest.srec > castest.run 2>&1
	"$CASDEC" castest.cas > dec.srec 2> castest-cas.dec
	"$SIM" dec.srec > run.tmp 2>&1
	same "castest.cas, decoded, runs as castest.srec" castest.run run.tmp
	"$CASDEC" -w castest.wav > dec.srec 2> castest-wav.dec
	"$SIM" dec.srec > run.tmp 2>&1
	same "castest.wav, decoded, runs as castest.srec" castest.run run.tmp

	"$SIM" -L rom-srec.out > rom-srec.list
	"$SIM" -L rom-ihex.out > run.tmp
	same "rom-ihex.out loads as rom-srec.out" rom-srec.list run.tmp
} > formats.result
rm -f run.tmp dec.srec castest.out castest.srec castest.cas castest.wav \
	rom-srec.list

# an illegal opcode ($02, after a NOP) is an error
printf '\001\002' > illegal.bin
"$SIM" -a 0 -e 0 illegal.bin > sim-illegal.run 2>&1
echo $? > sim-illegal.rc
rm -f illegal.bin

# the library also has file dates in its member headers: keep only the
# member list
"$AR" t libnum.a > libnum.t
rm -f libnum.a

if [ $update = 1 ]; then
	rm -rf "$ref"
	mkdir -p "$ref"
	cp -p "$out"/* "$ref"/
	echo "updated $(ls "$ref" | wc -l) reference files"
	exit 0
fi

failed=0
for f in "$ref"/*; do
	name=$(basename "$f")
	if [ ! -f "$out/$name" ]; then
		echo "MISSING $name"
		failed=$((failed + 1))
	elif ! cmp -s "$f" "$out/$name"; then
		echo "FAIL    $name"
		failed=$((failed + 1))
	fi
done
for f in "$out"/*; do
	name=$(basename "$f")
	if [ ! -f "$ref/$name" ]; then
		echo "NEW     $name (not in ref/)"
		failed=$((failed + 1))
	fi
done

total=$(ls "$ref" | wc -l)
if [ $failed = 0 ]; then
	echo "all $total test files match"
	exit 0
fi
echo "$failed of $total test files differ (outputs are in $out)"
exit 1
