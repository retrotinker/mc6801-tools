all: main.wav

.s.o:
	as01 -o $@ $<

libnum.a: one.o two.o three.o
	rm -f $@
	ar01 r $@ $^

main.aout: crtstart.o libnum.a main.o screen.o
	ld01 -lnum -Cstart -T0x434c -o $@ main.o screen.o

main.wav: main.aout
	objcopy01 -T0x434c -O wav main.aout main.wav

clean:
	rm -f *.wav *.aout *.o *.a
