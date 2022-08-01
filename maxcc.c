#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// Compiler flags
int dump_ir;

// Memory layout of a process
int *stack, *stack_p;
char *data, *data_p;
int *old_text, *text, *text_p;

// Registers and cycle of a CPU
int *pc, *bp, *sp, ax, cycle;
int idx_of_bp;

// Necessary variables to parse source code
int pool_size;
char *src;
int line;
int token;
int token_num;
int *type_size;
int type_new;
int expr_type;

int *case_addr, *default_addr, *break_addr, *continue_addr;
int switch_cnt;
int break_cnt;
int continue_cnt;

char *p, *last_p;
int *ast_ptr;
int local_var_offset;
int local_var_depth;

// Supported tokens and classes
enum {
	Num = 128, Func, Syscall, Main, Global, Param, Local, Keyword, Id, Label, Load, Enter,
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

enum {CHAR, INT, PTR = 256, PTR2 = 512};

struct ident {
	int token;
	int hash;
	char *name;
	int class;
	int type;
	int val;
	int hclass;
	int htype;
	int hval;
	int struct_type;
} *id, *sym;

struct struct_member {
	struct ident *id;
	int offset;
	int type;
	struct struct_member *next;
} **members;

void err_exit(char *errstr) {
	fprintf(stderr, "%d: %s", line, errstr);
	exit(1);
}

/* 
 * next() - parse the source code and get the token type;
 */
void next() {
	int hash;
	char *id_parser;
	char *str;
	while (token = *p) {
		++p;
		if ((token >= 'A' && token <= 'Z') || (token >= 'a' && token <= 'z') || (token == '_')) {
			// parse identifiers
			id_parser = p - 1;
			hash = token;
			token = *p;
			while ((token >= 'A' && token <= 'Z') || (token >= 'a' && token <= 'z') || (token >= '0' && token <= '9') || (token == '_')) {
				hash = hash * 147 + token;
				p++;
				token = *p;
			}
			hash = (hash << 6) + (p - id_parser);
			for (id = sym; id->token; id++) {
				if (hash == id->hash && !memcmp(id->name, id_parser, p - id_parser)) {
					token = id->token;
					return;
				}
			}
			id->name = id_parser;
			id->hash = hash;
			token = id->token = Id;
			return;
		}
		else if (token >= '0' && token <= '9') {
			// parse number literal constant
			token_num = token - '0';
			
			// beginning with non-zero -> decimal
			if (token_num > 0) {
				token = *p;
				while (token >= '0' && token <= '9') {
					token_num = token * 10 + token - '0';
					++p;
					token = *p;
				}
			}
			// otherwise, binary, octal or hexadecimal
			else {
				if (*p == 'b' || *p == 'B') {
					++p;
					token = *p;
					while (token == '0' || token == '1') {
						token_num = token_num * 2 + token - '0';
						++p;
						token = *p;
					}
				}
				else if (*p == 'x' || *p == 'X') {
					++p;
					token = *p;
					while ((token >= '0' && token <= '9') || (token >= 'a' && token <= 'f') || (token >= 'A' && token <= 'F')) {
						token_num = token_num * 16 + token - '0';
						++p;
						token = *p;
					}
				}
				else {
					token = *p;
					while (token >= '0' && token <= '7') {
						token_num = token_num * 8 + token - '0';
						++p;
						token = *p;
					}
				}
			}
			token = Num;
			return;
		}
		else if (token == '"' || token == '\'') {
			// parse a character or string
			str = data_p;
			while(*p != 0 && *p != token) {
				token_num = *p++;
				if (token_num == '\\') {
					token_num = *p++;
					switch(token_num) {
					case 'n':
						token_num = '\n';
						break;
					case 't':
						token_num = '\t';
						break;
					case '0':
						token_num = '\0';
						break;
					}
				}
				if (token == '"') {
					*data_p++ = token_num;
				}
			}
			p++;

			if (token == '"') {
				token_num = (int) str;
			}
			else {
				token = Num;
			}
			return;
		}
		switch (token) {
		case '\n':
			// ignore newline character
			if (dump_ir) {
				printf("%d: %.*s", line, p - last_p, last_p);
				last_p = p;
			}
			++line;
			break;
		case '#':
			// skip macro
			while (*p && *p != '\n') {
				++p;
			}
			break;
		case '=':
			if (*p == '=') {
				p++;
				token = Eq;
			}
			else {
				token = Assign;
			}
			return;
		case '+':
			if (*p == '+') {
				p++;
				token = Inc;
			}
			else {
				token = Add;
			}
			return;
		case '-':
			if (*p == '-') {
				p++;
				token = Dec;
			}
			else {
				token = Sub;
			}
			return;
		case '*':
			token = Mul;
			return;
		case '/':
			token = *p;
			++p;
			if (token == '/') {
				while (*p && *p != '\n') {
					++p;
				}
			}
			else if (token == '*') {
				while (*p && (*(p - 1) != '*' || *p != '/')) {
					if (*p == '\n');
						++line;
					++p;
				}
			}
			else {
				token = Div;
				return;
			}
			break;
		case '%':
			token = Mod;
			return;
		case '!':
			if (*p == '=') {
				p++;
				token = Ne;
			}
		case '<':
			if (*p == '=') {
				p++;
				token = Le;
			}
			else if (*p == '<') {
				p++;
				token = Shl;
			}
			else {
				token = Lt;
			}
			return;
		case '>':
			if (*p == '=') {
				p++;
				token = Ge;
			}
			else if (*p == '>') {
				p++;
				token = Shr;
			}
			else {
				token = Gt;
			}
			return;
		case '|':
			if (*p == '|') {
				p++;
				token = Lor;
			}
			else {
				token = Or;
			}
			return;
		case '&':
			if (*p == '&') {
				p++;
				token = Lan;
			}
			else {
				token = And;
			}
			return;
		case '^':
			token = Xor;
			return;
		case '[':
			token = Bracket;
			return;
		case '?':
			token = Cond;
			return;
		case ' ':
		case '\t':
		case '\r':
			break;
		case '~':
		case ';':
		case '{':
		case '}':
		case '(':
		case ')':
		case ']':
		case ',':
		case ':':
		default:
			return;
		}
	}
}

void match_token(int expected) {
	if (token == expected) {
		next();
	}
	else {
		fprintf(stderr, "%d: expected token: %c\n", line, expected);
		exit(-1);
	}
}


void expr(int level) {
	int old_token;
	struct ident *d;
	struct struct_member *sm;

	int params_cnt, *params_b;
	int name_len;
	char ch;

	int type, *old_ast_ptr;
	int size;

	switch(token) {
	case '\0': 
		err_exit("error - unexpected EOF in an expression\n");
	case Num:
		*--ast_ptr = token_num;
		*--ast_ptr = Num;
		next();
		expr_type = INT;
		break;
	case '"':
		*--ast_ptr = token_num;
		*--ast_ptr = Num;
		next();
		while (token == '"')
			next();
		data_p = (char *) (((int) data_p + sizeof(int)) & (-sizeof(int)));
		expr_type = PTR;
		break;
	case Sizeof:
		next();
		match_token('(');
		next();
		expr_type = INT;
		switch(token) {
		case Int: 
			next();
			break;
		case Char:
			next();
			expr_type = CHAR;
			break;
		case Struct:
		case Union:
			next();
			if (token != Id) 
				err_exit("error - expected identifier");
			expr_type = id->struct_type;
			next();
			break;
		}
		while (token == Mul) {
			next();
			expr_type += PTR;
		}
		match_token(')');
		next();
		*--ast_ptr = expr_type >= PTR ? sizeof(int) : type_size[expr_type];
		*--ast_ptr = Num;
		expr_type = INT;
		break;
	case Id:
		d = id;
		next();

		if (token == '(') {
			if (d->class < Func || d->class > Main) {
				if (d->class != 0)
					err_exit("error - bad function call\n");
				name_len = d->hash & 0x3f;
				ch = d->name[name_len];
				d->name[name_len] = 0;
				d->val = 0; // should be function address
				d->name[name_len] = ch;
				d->class = Syscall;
				d->type = INT;
			}
			next();
			params_cnt = 0;
			params_b = 0;
			while (token != ')') {
				expr(Assign);
				*--ast_ptr = (int) params_b;
				params_b = ast_ptr;
				++params_cnt;
				if (token == ',') {
					next();
					if (token == ')')
						err_exit("error - unexpected comma in function call\n");
				}
				else if (token != ')') {
					err_exit("error - missing comma in function call\n");

				}
			}
			next();
			*--ast_ptr = params_cnt;
			*--ast_ptr = d->val;
			*--ast_ptr = (int) params_b;
			*--ast_ptr = d->class;
			expr_type = d->type;
		}
		else if (d->class == Num) {
			*--ast_ptr = d->val;
			*--ast_ptr = Num;
			expr_type = INT;
		}
		else {
			switch(d->class) {
			case Local:
				*--ast_ptr = d->val;
				*--ast_ptr = Local;
				break;
			case Global:
				*--ast_ptr = d->val;
				*--ast_ptr = Num;
				break;
			default:
				err_exit("error - undefined variable\n");			
			}
			*--ast_ptr = expr_type = d->type;
			*--ast_ptr = Load;
		}
		break;
	case '(':
	case Mul:
	case And:
	case '!':
	case '~':
	case Add:
	case Sub:
	case Div:
	case Mod:
		break;
	case Inc:
	case Dec:
		break;
	default:
		err_exit("error - bad expression\n");
	}

	while (token >= level) {
		type = expr_type;
		old_ast_ptr = ast_ptr;
		switch(token) {
		case Assign:
			next();
			if (*ast_ptr != Load)
				err_exit("error - bad lvalue in assignment");
			expr(Assign);
			*--ast_ptr = (int) (old_ast_ptr + 2);
			*--ast_ptr = expr_type = type;
			*--ast_ptr = Assign;
			break;
		case Cond:
		case Lor:
		case Lan:
		case Or:
		case Xor:
		case And:
		case Eq:
		case Ne:
		case Lt:
		case Gt:
		case Le:
		case Ge:
		case Shl:
		case Shr:
			break;
		case Add:
			next();
			expr(Mul);
			size = (expr_type = type) >= PTR2 ? sizeof(int) :
							expr_type >= PTR ? type_size[expr_type - PTR] : 1;
			if (*ast_ptr == Num)
				ast_ptr[1] = ast_ptr[1] * size;
			if (*ast_ptr == Num && *old_ast_ptr == Num)
				ast_ptr[1] = ast_ptr[1] + old_ast_ptr[1];
			else {
				*--ast_ptr = (int) old_ast_ptr;
				*--ast_ptr = Add;
			}
			break;
		case Sub:
			next();
			expr(Mul);
			size = type >= PTR2 ? sizeof(int) : type >= PTR ? type_size[type = PTR] : 1;
			if (size > 1 && *ast_ptr == Num) {
				*--ast_ptr = size;
				*--ast_ptr = Num;
				--ast_ptr;
				*ast_ptr = (int) (ast_ptr + 3);
				*--ast_ptr = Mul;
			}
			if (*ast_ptr == Num && *old_ast_ptr == Num)
				ast_ptr[1] = old_ast_ptr[1] - ast_ptr[1];
			else {
				*--ast_ptr = (int) old_ast_ptr;
				*--ast_ptr = Sub;
				if (type == expr_type && size > 1) {
					switch(size) {
					case 4:
						*--ast_ptr = 2;
						*--ast_ptr = Num;
						--ast_ptr;
						*ast_ptr = (int) (ast_ptr + 3);
						*--ast_ptr = Shr;
						break;
					default:
						*--ast_ptr = size;
						*--ast_ptr = Num;
						--ast_ptr;
						*ast_ptr = (int) (ast_ptr + 3);
						*--ast_ptr = Sub;
					}
				}
			}
			break;
		case Mul:
			next();
			expr(Inc);
			if (*ast_ptr == Num && *old_ast_ptr == Num) {
				ast_ptr[1] = ast_ptr[1] * old_ast_ptr[1];
			}
			else {
				*--ast_ptr = (int) old_ast_ptr;
				/*
				if (ast_ptr[1] == Num && ast_ptr[2] > 0 && (ast_ptr[2] & (ast_ptr[2] - 1)) == 0) {
					ast_ptr[2]; // popcount;
				}
				else
				*/
					*--ast_ptr = Mul;
			}
			expr_type = INT;
			break;
		case Inc:
		case Dec:
			break;
		case Div:
		case Mod:
		case Dot:
		case Arrow:
		case Bracket:
		default:
			break;
		}
	}
}

/* 
 * stmt() - parse statements in any blocks
 * 
 * 
 * 
 */
void stmt(int target) {
	int type;

	switch (target) {
	case Func:
		local_var_depth = idx_of_bp;
		match_token('{');
		// parse local variables and other statements.
		while (token == Int || token == Char || token == Struct || token == Union) {
			type = INT;
			while (token != ';') {
	
				switch(token) {
					case Int:
						next();
						break;
					case Char:
						type = CHAR;
						next();
						break;
					case Struct:
					case Union:
						next();
						if (token != Id) 
							err_exit("error - bad struct/union declaration\n");
						type = id->struct_type;
						next();
						break;
				}
	
				if (token != Id) 
					err_exit("error - expected identifier\n");
				next();

				id->hclass = id->class;
				id->class = Local;
				
				id->htype = id->type;
				id->type = type;

				id->hval = id->val;
				id->val = ++local_var_depth;

				if (token == ',')
					next();
	
			}
			next();
		}

		*++text_p = ENT;
		*++text_p = local_var_depth - idx_of_bp;

		while (token != '}') {
			stmt(token);
		}

		*++text_p = LEV;
		break;
	case If:
	case While:
		next();
		match_token('(');
		// parse condition
		match_token(')');
		if (token == '{') {
			next();
			while (token != '}') {
				stmt(token);
			}
		}
		else
			stmt(token);
		break;
	case DoWhile:
		next();
		match_token('{');
		// parse statments
		while (token != '}') {
			stmt(token);
		}
		match_token('}');
		match_token(While);
		match_token('(');
		// parse condition
		match_token(')');	
		match_token(';');
		break;
	case Switch:
		next();
		match_token('(');
		// parse target variable
		match_token(')');
		match_token('{');
		while (token != '}') {
			stmt(token);
		}
		match_token('}');
		break;
	case Case:
		next();
		match_token('(');
		
		match_token(')');
		break;
	case Break:
		next();
		match_token(';');
		break;
	case Continue:
		next();
		match_token(';');
		break;
	case Default:
		next();
		break;
	case Return:
		next();
		if (token != ';') {
			expr(Assign);
		}
		match_token(';');
		break;
	case For:
		next();
		match_token('(');
		stmt(Assign);
		match_token(';');
		stmt(Assign);
		match_token(';');
		stmt(Assign);
		match_token(')');
		if (token == '{') {
			next();
			while (token != '}') {
				stmt(token);
			}
		}
		else
			stmt(token);
		break;
//	case Goto:
		break;
	default:
		expr(Assign);
		next();
		break;
	}
}

/* 
 * parse_global_decl() - parse global variables, functions and composite data types
 * 
 * It should parse the source code matching the following form.
 * 	(...) means function parameters.
 *	{...} means function definition or composite data types definition.
 *
 * 
 * variable:
 * 	<type> [*] <id> [ = (<expr> | {...})] ;
 *
 *		int a = 10;
 *		int aa;
 *		int *b;
 *		char **c;
 *
 * function:
 * 	<type> [*] <id> (...) {...}
 *
 * 		void foo() {...}
 *		int func(int k) {...}
 *
 *		after getting '(', it should start ...
 *			1. parse function parameters.
 *			2. parse function definition. (local variables and statements)
 *				
 * enum:
 * 	enum [<id>] {...} ;
 * 
 * struct:
 *	struct <id> {...} ;
 *
 * union:
 *	union <id> {...} ;
 */
void parse_global_decl() {
	int i;
	int type;
	int params;
	int idx_of_locvar;
	int decl_type;
	int member_type;
	int struct_token;
	struct struct_member *m;

	decl_type = INT;

	switch (token) {
	case Enum:
		next();
		if (token != '{') {
			if (token != Id) 
				err_exit("error - expected identifier");
		}
		if (token == '{') {
			next();
			i = 0;
			while(token != '}') {
				if (token != Id) {
					err_exit("error - bad enum identifier\n");
				}
				next();
				if (token == Assign) {
					// TODO: parse a string literal or an expression.
					next();
					// expr()
					if (token != Num) {
						err_exit("error - bad enum initializer\n");
					}
					i = token_num;
					next();
				}
				id->class = Num;
				id->type = INT;
				id->val = i++;
				
				match_token(',');
			}
			next();

		}

		match_token(';');
		return;
	case Int:
	case Char:
	case Struct:
	case Union:
		type = token;
		switch(token) {
		case Struct:
		case Union:
			i = 0;
			struct_token = token;
			next();
			if (token != Id) 
				err_exit("error - expected identifier");
			if (!id->struct_type)
				id->struct_type = type_new++;
			type = type_new;
			decl_type = type_new;
			if (token == '{') {
				type_size[decl_type] = 0;
				i = 0;
				if (members[decl_type])
					err_exit("error - duplicate structure definition\n");
				while(token != '}') {
					next();
					member_type = INT;
					switch (token) {
					case Int:
						next();
						break;
					case Char:
						next();
						member_type = CHAR;
						break;
					case Struct:
					case Union:
						next();
						if (token != Id)
							err_exit("error - bad struct/union declaration\n");
						member_type = id->struct_type;
						next();
						break;
					}
					
					while (token != ';') {
						expr_type = member_type;
						while (token == Mul) {
							expr_type += PTR;
							next();
						}
		
						if (token != Id) {
							err_exit("error - expected identifier\n");
						}
						
						m = malloc(sizeof(struct struct_member));
						m->id = id;
						m->offset = i;
						m->type = expr_type;
						m->next = members[decl_type];
						members[decl_type] = m;
						i = i + (decl_type >= PTR) ? sizeof(int) : type_size[decl_type];
						i = (i + 3) & -4;
						if (struct_token == Union) {
							if (i > type_size[decl_type])
								type_size[decl_type] = i;
							i = 0;
						}
						next();
						if (token == ',')
							next();
					}
					next();
				}
				match_token('}');
				if (struct_token != Union)
					type_size[decl_type] = i;
			}
			break;
		case Int:
		case Char:
			decl_type = (token == Int) ? INT : CHAR;
			next();
			break;
		}

		if ((type == Int || type == Char) && token == ';') {
			err_exit("bad global declaration\n");
		}
		
		while (token != ';' && token != '}') {
			expr_type = decl_type;
			while (token == Mul) {
				expr_type = expr_type + PTR;
				next();
			}

			if (token != Id) {
				err_exit("error - expected identifier\n");
			}
			next();

			if (token == '(') {
				id->class = Func;
				id->val = (int) (text_p + 1);
				// TODO: parse function parameters and definition
				// parse parameters
				next();
				while (token != ')') {
					type = INT;
					switch (token) {
					case Int:
						next();
						break;
					case Char:
						next();
						type = Char;
						break;
					case Struct:
					case Union:
						next();
						if (token != Id)
							err_exit("error - bad struct type for parameter\n");
						type = id->struct_type;
						next();
						break;
					}
					
					while (token == Mul) {
						type = type + PTR;
						next();
					}

					if (token != Id) {
						err_exit("error - bad parameter declaration\n");
					}

					if (id->class == Local) {
						err_exit("error - duplicate parameter declaration\n");
					}

					if (token != Id) 
						err_exit("error - expected identifier");

					id->hclass = id->class;
					id->class = Local;

					id->htype = id->type;
					id->type = type;
					
					id->hval = id->val;
					id->val = params++;

					next();
					if (token == ',')
						match_token(',');
				}
				idx_of_bp = params + 1;
				next();

				old_text = text_p;
				
				stmt(Func);
				id = sym;

				// clear id table for local variable and label
				while (id->token) {
					if (id->class == Local) {
						id->class = id->hclass;
						id->type = id->htype;
						id->val = id->hval;
					}
					else if (id->class == Label) {
						id->class = 0;
						id->type = 0;
						id->val = 0;
					}
					id++;
				}
			}
			else {
				id->class = Global;
				id->val = (int)(data_p);
				if (expr_type < PTR)
					data_p = data_p + type_size[decl_type];
				else
					data_p = data_p + sizeof(int);
				if (token == ',')
					match_token(',');

			}
		}
	}
	next();
}

void program() {
	next();
	while (token > 0) {
		parse_global_decl();
	}
}

int main(int argc, char **argv) {

	int *ast;
	int i;
	struct ident *id_main;

	dump_ir = 0;
	type_new = 0;

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

	pool_size = 256 * 1024;
	if (!(src = malloc(pool_size))) {
		err_exit("error - couldn't malloc for source code text.\n");
	}
	
	if (!(sym = malloc(pool_size))) {
		err_exit("error - couldn't malloc for symbol table.\n");
	}

	if (!(stack = stack_p = malloc(pool_size))) {
		err_exit("error - couldn't malloc for stack segment.\n");
	}

	if (!(data = data_p = malloc(pool_size))) {
		err_exit("error - couldn't malloc for data segment.\n");
	}

	if (!(text = text_p = malloc(pool_size))) {
		err_exit("error - couldn't malloc for text segment.\n");
	}

	if (!(type_size = malloc(PTR * sizeof(int)))) {
		err_exit("error - couldn't malloc for type size table\n");
	}

	if (!(members = malloc(PTR * sizeof(struct ident *)))) {
		err_exit("error - couldn't malloc for struct member table\n");
	}

	if (!(ast = malloc(pool_size))) {
		err_exit("error - couldn;t malloc for abstract syntax tree\n");
	}

	memset(src, 0, pool_size);
	memset(sym, 0, pool_size);
	memset(type_size, 0, PTR * sizeof(int));
	memset(members, 0, PTR * sizeof(struct ident *));
	memset(ast, 0, pool_size);

	ast_ptr = (int *)((int)ast + pool_size);

	p = "break continue case char default else enum if int return "
	    "sizeof struct union switch for while do goto void main "
	    "open read close printf fprintf malloc memset memcmp exit";

	// C Keywords
	for (i = Break; i < Goto; i++) {
		next();
		id->token = i;
		id->class = Keyword;
	}

	// void
	next();
	id->token = Char;

	// main
	next();
	id_main = id;

	// Syscalls / C std funcs
	for (i = OPEN; i < EXIT; i++) {
		next();
		id->type = INT;
		id->class = Syscall;
		id->val = i;
	}

	type_size[type_new++] = sizeof(char);
	type_size[type_new++] = sizeof(int);

	while (argc) {
		int fd = open(*argv, 0);
		int i = 0;
		if (fd < 0) {
			fprintf(stderr, "error - for source file %s\n", *argv);
			err_exit("couldn't open the source file.\n");
		}

		if ((i = read(fd, src, pool_size - 1)) <= 0) {
			fprintf(stderr, "error - for source file %s\n", *argv);
			err_exit("unable to read the source file\n");
		}
		memset(stack, 0, pool_size);
		memset(data, 0, pool_size);
		memset(text, 0, pool_size);

		src[i] = 0;
		close(fd);
		last_p = p = src;
		line = 1;

		program();

		--argc; ++argv;
	}

	free(src);
	free(sym);
	free(stack);
	free(data);
	free(text);
	free(type_size);
	free(members);

	return 0;
}
