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

# one-byte relocations (forced direct addresses, indexed offsets): in
# page zero they link; elsewhere each is an error naming the module, or
# the library member, the target and the address
link dp-zp	-m -M -S dpzp.ld dpref.o dpvars.o
link dp-far	-T 0xE000 -D 0x5000 dpref.o dpvars.o
"$AR" rc libdp.a dpref.o
link dp-lib	-T 0xE000 -D 0x5000 dpcall.o libdp.a dpvars.o
rm -f libdp.a

# DPAGE: direct addresses, 8-bit immediates and FCB data with page-zero
# addresses; with counter 4 in page zero they link, elsewhere each is
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
for f in srec ihex wav; do
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
rm -f allops.bin allops.lst allops.as allops.sim

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

# dpage.s linked with counter 4 in page zero: the page-zero data it
# changed, its registers, and its table of addresses
"$SIM" -d 0x0080-0x008F -d 0xE024-0xE029 dpage.out > sim-dpage.run 2>&1
echo $? > sim-dpage.rc

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
