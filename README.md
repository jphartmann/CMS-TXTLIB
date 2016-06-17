# CMS-TXTLIB
Build CMS TXTLIB (new style) from text files.

This utility allows you to generate from S/360 TEXT decks a CMS TXTLIB that can be
uploaded in binary.

The maclib is in the "new format" which is not compatible with VM/370.

Unlike the `CMS TXTLIB GEN` command, `txtlib` generates a member for each
`ESD/END` bracket, in effect suppporting the output of a batch assembly.

### Syntax
```
txtlib <outfile> <infile> ...
```
