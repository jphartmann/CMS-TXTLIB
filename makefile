CFLAGS:=-Wall -Werror -g
.PHONY: all check ok stdok clean

all: txtlib

txtlib: objdeck.h

install: ${HOME}/bin/txtlib | ${HOME}/bin

${HOME}/bin/txtlib: txtlib
	cp -p $< $@

${HOME}/bin:
	mkdir $@

clean:
	rm -f txtlib *.txtlib core stderr stdout

$HOME/bin:
	mkdir ${HOM}/bin

check: txtlib
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
	) 2>stderr >stdout
	@ diff -u stdout.ok stdout && diff -u stderr.ok stderr
	@echo '>>> all pass <<<'

ok: stdok
	mv stderr stderr.ok

stdok:
	mv stdout stdout.ok
