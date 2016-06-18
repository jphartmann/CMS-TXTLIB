# CMS-TXTLIB
Build CMS TXTLIB and MACLIB (new style).

These two utilities are similar to the CMS commands, except that they
run on UNIX.
They both produce "new format" output; that is `LIBPDS` with 32 bit
numbers.
This format is not compatible with VM/370.

The output from both is EBCDIC and should be uploaded
```
site fix 80
bin
```
`txtlib` generates a CMS TXTLIB from S/360 TEXT decks, which are
EBCDIC F(80).
You will need a workstation assembler to generate such decks,
or a utility that converts `elf` to S/360 object format.

Unlike the `CMS TXTLIB GEN` command, `txtlib` generates a member for each
`ESD/END` bracket, in effect suppporting the output of a batch assembly
as individual members.
The directory contains an entry for each control section (SD) and each
entry (LD).

`maclib` geneates an EBCDIC macro library from variable length
ASCII files.
Stacked copy (`*COPY` member delimiters) is supported for
files that have a `.copy` extension if the first line
of the file is such
a delimiter.


### Syntax
```
txtlib <outfile> <infile> ...
maclib <outfile> <infile> ...
```
