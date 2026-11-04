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
