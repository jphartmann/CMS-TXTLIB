.PHONY: all check ok stdok clean stdok errok
CFLAGS:=-Wall -Werror -g

B:=${HOME}/bin
H:=objdeck.h libpds.h subs.c

$B/%: %
	cp -p $< $@

all: txtlib maclib

txtlib.o: $H
maclib.o: $H

install: $B/maclib $B/txtlib | $B

$B:
	mkdir $@

clean:
	rm -f maclib txtlib *.o *.txtlib *.maclib core stderr stdout

check: txtlib maclib
	rm -f core
	- @ ( 											 \
		./txtlib;								 \
		echo No args $$? >&2;				\
		./txtlib null.text;					 \
		echo One arg $$? >&2;				\
		./txtlib null.txtlib null.text ;   \
		echo null	 $$? >&2;				\
		od -t x1 -A x null.txtlib >&2;	 \
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
	) 2>stderr >stdout
	@ diff -u stdout.ok stdout && diff -u stderr.ok stderr
	@echo '>>> all pass <<<'

ok: outok

outok:
	mv stdout stdout.ok

errok:
	mv stderr stderr.ok
