#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// Compiler flags
int dump_ir = 1;


// Necessary variables
int src_size = 0;
char *src;
int line = 1;
char token;
char *src;
char *p;
char *last_p;

// Supported tokens and classes
enum {
	Num = 128, Func, Syscall, Main, Glo, Par, Loc, Keyword, Id, Label, Load, Enter,
	Break, Continue, Case, Char, Default, Else, Enum, If, Int, Return,
	Sizeof, Struct, Union, Switch, For, While, DoWhile, Goto,
	Assign,
	Cond,
	Lor, Lan, Or, Xor, And,
	Eq, Ne, Lt, Gt, Le, Ge,
	Shl, Shr, Add, Sub, Mul, Div, Mod,
	Inc, Dec, Dot, Arrow, Bracket
};

// Supported instructions (opcodes)
enum {
	LEA,
	/* 0 */
	
	IMM,
	/* 1 */

	JMP,
	/* 2 */

	CALL,
	/* 3 */

	BZ, BNZ,
	/* 4 5 */

	ENT,
	/* 6 */

	ADJ,
	/* 7 */

	LEV,
	/* 8 */

	LW, LC, SW, SC,
	/* 9 10 11 12 */

	PUSH,
	/* 13 */

	OR, XOR, AND,
	/* 14 15 16 */

	EQ, NEQ,
	/* 17 18 */

	LT, GT, LE, GE,
	/* 19 20 21 22 */

	SHL, SHR,
	/* 23 24 */

	ADD, SUB, MUL, DIV, MOD,
	/* 25 26 27 28 29 */

	OPEN, READ, CLOS, PRTF, FPRT, MALC, MSET, MCMP, EXIT
	/* 30 31 32 33 34 35 36 37 38 */
};


void err_exit(char *errstr) {
	fprintf(stderr, "%s", errstr);
	exit(1);
}

void next() {
	while (token = *p) {
		++p;
		if (token == '\n') {
			if (dump_ir) {
				printf("%d: %.*s", line, p - last_p, last_p);
				last_p = p;
			}
			++line;
		}
		else if (token == '#') {
			while (*p && *p != '\n') {
				++p;
			}
		}
	}
}

void stmt() {

}

void expr() {

}

void program() {
	next();
}

int main(int argc, char **argv) {

	--argc; ++argv;
	if (argc > 0 && !strcmp(*argv, "--dump-ir")) {
		// TODO
		--argc; ++argv;
		dump_ir = 1;
	}
	if (argc < 1) {
		err_exit("usage:\n"
			 "./rvxcc [--dump-ir] <src>\n");
	}

	src_size = 256 * 1024;
	src = malloc(src_size);
	while (argc) {
		int fd = open(*argv, 0);
		int i = 0;
		if (fd < 0) {
			fprintf(stderr, "err: for source file %s\n", *argv);
			err_exit("couldn't open the source file.\n");
		}

		if ((i = read(fd, src, src_size - 1)) <= 0) {
			fprintf(stderr, "err: for source file %s\n", *argv);
			err_exit("err: unable to read the source file\n");
		}
		src[i] = 0;
		close(fd);
		last_p = p = src;
		
		program();

		--argc; ++argv;

	}
	free(src);

	return 0;
}
