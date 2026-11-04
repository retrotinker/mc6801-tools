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

# conversions of the demo executable
for f in srec ihex wav; do
	"$OC" -O $f -n MAIN demo.out demo.$f > /dev/null 2>&1
	echo $? > demo.$f.rc
done

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
