# MAXCC = Multiple Architectures Cross C Compiler
```
MMMMMMM       MMMMMMM      AAAAAAAAA      XXXXX       XXXXX     CCCCCCCCCCCCCCC       CCCCCCCCCCCCCCC
M     M       M     M     A         A      X   X     X   X     C               C     C               C
M      M     M      M    A     A     A      X   X   X   X     C                C    C                C
M       M   M       M   A     A A     A      X   X X   X     C       CCCCCCCCCC    C       CCCCCCCCCC
M        MMM        M  A     A   A     A      X   X   X      C      C              C      C
M                   M  A    A     A    A       X     X       C     C               C     C
M    MM       MM    M  A    A     A    A        X   X        C    C                C    C
M    M M     M M    M  A    A     A    A       X     X       C     C               C     C
M    M  M   M  M    M  A    AAAAAAA    A      X   X   X      C      C              C      C
M    M   MMM   M    M  A    A     A    A     X   X X   X     C       CCCCCCCCCC    C       CCCCCCCCCC
M    M         M    M  A    A     A    A    X   X   X   X     C                C    C                C
M    M         M    M  A    A     A    A   X   X     X   X     C               C     C               C
MMMMMM         MMMMMM  AAAAAA     AAAAAA  XXXXX       XXXXX     CCCCCCCCCCCCCCC       CCCCCCCCCCCCCCC
```

## Introduction
The purpose of ```maxcc``` is wanting to be a cross compiler, which can support different architectures.

The current targets of ```maxcc``` are listed as following.
* Implement **Intermediate Representation (IR)**
* Generate 32-bit architectures assembly first, the preferred ISAs are listed as following
	* RISC-V (1st)
	* x86_64 (2nd)
	* ARM (3rd)
* Support basic C syntax
	* basic statements and expression operators
	* basic data types
	* partial C89, C99, C23 features

## Advanced targets
* Self-hosting (self-compiling)
* Other/advanced C syntax and features
* Generate binary executable
	* ELF format
* Support 64-bit architectures
* Optimization

## Build and Run
* Generate the binary executable ```maxcc```
```
$ make
```
* Compile the source code by ```maxcc```
```
$ ./maxcc [--dump-ir] <source file>
```
