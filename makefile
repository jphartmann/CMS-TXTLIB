.PHONY: all check ok stdok clean stdok errok
CFLAGS:=-Wall -Werror -g

B:=${HOME}/bin
H:=objdeck.h libpds.h subs.c

$B/%: %
	cp -p $< $@

all: txtlib maclib libdir

txtlib.o: $H
maclib.o: $H
libdir.o: libpds.h

install: $B/maclib $B/txtlib $B/libdir | $B

$B:
	mkdir $@

clean:
	rm -f maclib txtlib libdir *.o *.txtlib *.maclib core stderr stdout

check: txtlib maclib libdir
	rm -f core
	- @ ( 											 \
		./txtlib;								 \
		echo No args $$? >&2;				\
		./txtlib null.text;					 \
		echo One arg $$? >&2;				\
		./txtlib null.txtlib null.text ;   \
		echo null	 $$? >&2;				\
		od -t x1 -A x null.txtlib >&2;	 \
		./txtlib stor.txtlib stor390.text ;   \
		echo stor	  $$? >&2;				 \
		od -t x1 -A x stor.txtlib >&2;	 \
		./libdir stor.txtlib; \
		./txtlib tod.txtlib tod390.text ;	\
		echo tod 	 $$? >&2;				\
		od -t x1 -A x tod.txtlib >&2; 	\
		./txtlib todn.txtlib tod390.text null.text ;   \
		echo todnull	  $$? >&2;				 \
		od -t x1 -A x todn.txtlib >&2;	 \
		./txtlib 252.txtlib 252cards.text ;   \
		echo 252 cards $$? >&2; 			  \
		od -t x1 -A x 252.txtlib >&2; 	\
		./txtlib 253.txtlib 253cards.text ;   \
		echo 253 cards $$? >&2; 			  \
		od -t x1 -A x 253.txtlib >&2; 	\
		./maclib 390.maclib run390.copy ;	\
		echo 390 copy $$? >&2;				 \
		od -t x1 -A x 390.maclib >&2; 	\
		./maclib stacked.maclib stacked.copy ;   \
		echo 390 stacked copy $$? >&2;				\
		od -t x1 -A x stacked.maclib >&2;	 \
		./maclib notstacked.maclib notstacked.copy ;   \
		echo 390 notstacked copy $$? >&2;				\
		od -t x1 -A x notstacked.maclib >&2;	 \
		./libdir tod.txtlib; \
	) 2>stderr >stdout
	@ diff -u stdout.ok stdout && diff -u stderr.ok stderr
	@echo '>>> all pass <<<'

ok: outok

outok:
	mv stdout stdout.ok

errok:
	mv stderr stderr.ok
