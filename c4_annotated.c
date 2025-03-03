// c4.c - A minimal self-hosting C compiler written in 4 functions
// Author: Robert Swierczek
// This compiler is special because it can compile itself! It's like a program that can make copies of itself.
// It includes just enough C features to be useful for basic system programming.

// These are like importing tools we need:
// stdio.h - for reading/writing (like printf)
// stdlib.h - for memory stuff (like malloc)
// memory.h - for memory operations (like memcmp)
// unistd.h - for system stuff (like read/write)
// fcntl.h - for file control
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include <fcntl.h>

// This makes all integers 64-bit (really big numbers)
// It's like telling the computer "when I say int, I mean a big number"
#define int long long

// These are like bookmarks that help us keep track of where we are in the code
char *p,    // Points to where we're currently reading in the source code (like a finger following text)
     *lp,   // Remembers the start of the current line (helpful for error messages)
     *data; // Points to where we store global variables and constants

// More bookmarks and counters for different parts of the compiler
int *e,     // Points to where we're writing the compiled code
    *le,    // Points to the last instruction we wrote
    *id,    // Points to the name we're currently looking at
    *sym,   // A big list of all names and variables we know about
    tk,     // The current piece of code we're looking at (like +, -, or a name)
    ival,   // If we find a number, we store it here
    ty,     // What type of thing we're dealing with (like int or char)
    loc,    // Helps us keep track of local variables
    line,   // Which line of code we're reading (for error messages)
    src,    // If true, shows the source code while compiling
    debug;  // If true, shows extra information for debugging

// This is like a dictionary of all the special words and symbols the compiler knows
// We start at 128 to avoid mixing up with regular characters (like 'a' which is 97)
enum {
    // These are for numbers and names
    Num = 128, Fun, Sys, Glo, Loc, Id,
    
    // These are the special words in C (keywords)
    Char, Else, Enum, If, Int, Return, Sizeof, While,
    
    // These are all the math and comparison symbols
    // They're in order of priority (like how * happens before +)
    Assign, Cond, Lor, Lan, Or, Xor, And, Eq, Ne, Lt, Gt, Le, Ge,
    Shl, Shr, Add, Sub, Mul, Div, Mod, Inc, Dec, Brak
};

// These are the instructions our simple computer understands
// Think of them like basic commands in a very simple calculator
enum {
    // Commands for moving data around
    LEA,IMM,JMP,JSR,BZ,BNZ,ENT,ADJ,LEV,LI,LC,SI,SC,PSH,
    
    // Math and logic commands
    OR,XOR,AND,EQ,NE,LT,GT,LE,GE,SHL,SHR,ADD,SUB,MUL,DIV,MOD,
    
    // Commands to talk to the computer system
    OPEN,READ,CLOS,PRTF,MALC,FREE,MSET,MCMP,EXIT
};

// The types of data our compiler can handle
// Like different sized boxes for storing different kinds of things
enum { CHAR, INT, PTR };

// This is how we store information about each name/variable in our program
// Since we can't use structs (they're too complex), we use a simple list
// Each entry in the list has these parts in order:
enum {
    Tk,      // What kind of thing it is (like variable or function)
    Hash,    // A number that helps us find it quickly
    Name,    // The actual name (like "x" or "main")
    Class,   // Whether it's a global or local variable or function
    Type,    // What type it is (like int or char)
    Val,     // Its value or where it's stored
    HClass,  // Previous class (for when variables share names)
    HType,   // Previous type
    HVal,    // Previous value
    Idsz     // How big each entry is
};

// The four main functions:
// next() - reads the next piece of code
// expr() - understands expressions (like 2+2)
// stmt() - understands statements (like if/while)
// main() - puts it all together

// This compiler is special because it only uses these four functions to do
// everything! Most compilers have many more functions, but this one shows
// we can make a working compiler with just the basics. 

// next() is like a scanner - it reads one piece of code at a time
// It's similar to how we read text one word at a time
void next() {
    char *pp; // Temporary pointer for remembering where a word starts

    // Keep reading characters until we find something meaningful
    while (tk = *p) {
        ++p; // Move to next character
        
        // Handle new lines - important for keeping track of line numbers
        if (tk == '\n') {
            if (src) { // If we're showing source code while compiling
                // Print the current line and any generated code
                printf("%d: %.*s", line, p - lp, lp);
                lp = p;
                // Print all instructions generated for this line
                while (le < e) {
                    // Print instruction names (each is 5 characters with padding)
                    printf("%8.4s", &"LEA ,IMM ,JMP ,JSR ,BZ  ,BNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,SI  ,SC  ,PSH ,"
                                   "OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,"
                                   "OPEN,READ,CLOS,PRTF,MALC,FREE,MSET,MCMP,EXIT,"[*++le * 5]);
                    // Print argument for instructions that need one
                    if (*le <= ADJ) printf(" %d\n", *++le); else printf("\n");
                }
            }
            ++line; // Count the line
        }
        
        // Handle preprocessor directives (lines starting with #)
        else if (tk == '#') {
            while (*p != 0 && *p != '\n') ++p; // Skip the whole line
        }
        
        // Handle names (variables, functions, etc.)
        // Names can contain letters, numbers, and underscore
        else if ((tk >= 'a' && tk <= 'z') || (tk >= 'A' && tk <= 'Z') || tk == '_') {
            pp = p - 1; // Remember where the name starts
            // Read the whole name
            while ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || 
                   (*p >= '0' && *p <= '9') || *p == '_')
                tk = tk * 147 + *p++; // Calculate hash while reading
            
            // Create unique hash for this name
            tk = (tk << 6) + (p - pp);
            
            // Look for this name in our symbol table
            id = sym;
            while (id[Tk]) {
                // If we find it, return its token type
                if (tk == id[Hash] && !memcmp((char *)id[Name], pp, p - pp)) {
                    tk = id[Tk];
                    return;
                }
                id = id + Idsz;
            }
            
            // If name not found, add it as a new identifier
            id[Name] = (int)pp;
            id[Hash] = tk;
            tk = id[Tk] = Id;
            return;
        }
        
        // Handle numbers (decimal, hex, octal)
        else if (tk >= '0' && tk <= '9') {
            // Decimal numbers
            if (ival = tk - '0') {
                while (*p >= '0' && *p <= '9') ival = ival * 10 + *p++ - '0';
            }
            // Hex numbers (starting with 0x)
            else if (*p == 'x' || *p == 'X') {
                while ((tk = *++p) && ((tk >= '0' && tk <= '9') || 
                       (tk >= 'a' && tk <= 'f') || (tk >= 'A' && tk <= 'F')))
                    ival = ival * 16 + (tk & 15) + (tk >= 'A' ? 9 : 0);
            }
            // Octal numbers (starting with 0)
            else {
                while (*p >= '0' && *p <= '7') ival = ival * 8 + *p++ - '0';
            }
            tk = Num;
            return;
        }
        
        // Handle division operator and comments
        else if (tk == '/') {
            if (*p == '/') { // Single line comment
                ++p;
                while (*p != 0 && *p != '\n') ++p;
            }
            else { // Division operator
                tk = Div;
                return;
            }
        }
        
        // Handle strings and character literals
        else if (tk == '\'' || tk == '"') {
            pp = data; // Remember where string starts in data segment
            while (*p != 0 && *p != tk) {
                if ((ival = *p++) == '\\') { // Handle escape sequences
                    if ((ival = *p++) == 'n') ival = '\n';
                }
                if (tk == '"') *data++ = ival;
            }
            ++p;
            if (tk == '"') ival = (int)pp; else tk = Num;
            return;
        }
        
        // Handle two-character operators
        else if (tk == '=') { if (*p == '=') { ++p; tk = Eq; } else tk = Assign; return; }
        else if (tk == '+') { if (*p == '+') { ++p; tk = Inc; } else tk = Add; return; }
        else if (tk == '-') { if (*p == '-') { ++p; tk = Dec; } else tk = Sub; return; }
        else if (tk == '!') { if (*p == '=') { ++p; tk = Ne; } return; }
        else if (tk == '<') { if (*p == '=') { ++p; tk = Le; } 
                             else if (*p == '<') { ++p; tk = Shl; } else tk = Lt; return; }
        else if (tk == '>') { if (*p == '=') { ++p; tk = Ge; }
                             else if (*p == '>') { ++p; tk = Shr; } else tk = Gt; return; }
        else if (tk == '|') { if (*p == '|') { ++p; tk = Lor; } else tk = Or; return; }
        else if (tk == '&') { if (*p == '&') { ++p; tk = Lan; } else tk = And; return; }
        
        // Single-character operators
        else if (tk == '^') { tk = Xor; return; }
        else if (tk == '%') { tk = Mod; return; }
        else if (tk == '*') { tk = Mul; return; }
        else if (tk == '[') { tk = Brak; return; }
        else if (tk == '?') { tk = Cond; return; }
        // Other single characters that don't need special handling
        else if (tk == '~' || tk == ';' || tk == '{' || tk == '}' || tk == '(' || 
                 tk == ')' || tk == ']' || tk == ',' || tk == ':') return;
    }
}

// expr() understands expressions - it's like a calculator that can handle
// complex math and logic. It uses precedence to know which operations to do first
void expr(int lev) {
    int t, *d;

    // Handle numbers, variables, and function calls
    if (!tk) { printf("%d: unexpected eof in expression\n", line); exit(-1); }
    else if (tk == Num) { // Number
        *++e = IMM; *++e = ival;
        next();
    }
    else if (tk == '"') { // String
        *++e = IMM; *++e = ival;
        next();
        while (tk == '"') next();
        data = (char *)((int)data + sizeof(int) & -sizeof(int));
    }
    else if (tk == Id) { // Variable or function
        d = id;
        next();
        if (tk == '(') { // Function call
            next();
            t = 0; // counts number of arguments
            while (tk != ')') { // Process arguments
                expr(Assign);
                *++e = PSH;
                ++t;
                if (tk == ',') next();
            }
            next();
            if (d[Class] == Sys) *++e = d[Val]; // system call
            else if (d[Class] == Fun) { *++e = JSR; *++e = d[Val]; } // function call
            else { printf("%d: bad function call\n", line); exit(-1); } // bad function call
            if (t) { *++e = ADJ; *++e = t; }  // cleanup after call
            ty = d[Type]; // type of function
        }
        else if (d[Class] == Num) { *++e = IMM; *++e = d[Val]; ty = INT; }  // enum value
        else {  // variable access
            if (d[Class] == Loc) { *++e = LEA; *++e = loc - d[Val]; }  // local var
            else if (d[Class] == Glo) { *++e = IMM; *++e = d[Val]; }   // global var
            else { printf("%d: undefined variable\n", line); exit(-1); }
            *++e = ((ty = d[Type]) == CHAR) ? LC : LI;
        }
    }
    else if (tk == '(') {  // handles type casting and grouped expressions
    next();
    if (tk == Int || tk == Char) {  // type cast
      t = (tk == Int) ? INT : CHAR; next();
      while (tk == Mul) { next(); t = t + PTR; }  // handle pointer types in cast
      if (tk == ')') next(); else { printf("%d: bad cast\n", line); exit(-1); }
      expr(Inc);
      ty = t;
    }
    else {  // grouped expression in parentheses
      expr(Assign);
      if (tk == ')') next(); else { printf("%d: close paren expected\n", line); exit(-1); }
    }
  }
  else if (tk == Mul) {  // handles pointer dereference (like *p)
    next(); expr(Inc);
    if (ty > INT) ty = ty - PTR; else { printf("%d: bad dereference\n", line); exit(-1); }
    *++e = (ty == CHAR) ? LC : LI;
  }
  else if (tk == And) {  // handles address-of operator (like &x)
    next(); expr(Inc);
    if (*e == LC || *e == LI) --e; else { printf("%d: bad address-of\n", line); exit(-1); }
    ty = ty + PTR;
  }
  else if (tk == '!') {  // handles logical not (like !x)
    next(); expr(Inc);
    *++e = PSH; *++e = IMM; *++e = 0; *++e = EQ;
    ty = INT;
  }
  else if (tk == '~') {  // handles bitwise not (like ~x)
    next(); expr(Inc);
    *++e = PSH; *++e = IMM; *++e = -1; *++e = XOR;
    ty = INT;
  }
  else if (tk == Add) {  // handles unary plus (like +x)
    next(); expr(Inc); ty = INT;
  }
  else if (tk == Sub) {  // handles unary minus (like -x)
    next(); *++e = IMM;
    if (tk == Num) { *++e = -ival; next(); }  // optimize for constants
    else { *++e = -1; *++e = PSH; expr(Inc); *++e = MUL; }
    ty = INT;
  }
  else if (tk == Inc || tk == Dec) {  // handles ++x and --x (pre-increment/decrement)
    t = tk; next(); expr(Inc);
    if (*e == LC) { *e = PSH; *++e = LC; }
    else if (*e == LI) { *e = PSH; *++e = LI; }
    else { printf("%d: bad lvalue in pre-increment\n", line); exit(-1); }
    *++e = PSH;
    *++e = IMM; *++e = (ty > PTR) ? sizeof(int) : sizeof(char);  // figure out how much to add/subtract
    *++e = (t == Inc) ? ADD : SUB;  // do the increment or decrement
    *++e = (ty == CHAR) ? SC : SI;  // store the result back
  }
  else { printf("%d: bad expression\n", line); exit(-1); }

  while (tk >= lev) { // "precedence climbing" or "Top Down Operator Precedence" method
    // keep going while we see operators of high enough precedence
    t = ty;
    if (tk == Assign) {  // handle = operator
      next();
      if (*e == LC || *e == LI) *e = PSH; else { printf("%d: bad lvalue in assignment\n", line); exit(-1); }
      expr(Assign); *++e = ((ty = t) == CHAR) ? SC : SI;
    }
    else if (tk == Cond) {  // handle ?: operator (like: x ? y : z)
      next();
      *++e = BZ; d = ++e;  // jump if condition is false
      expr(Assign);
      if (tk == ':') next(); else { printf("%d: conditional missing colon\n", line); exit(-1); }
      *d = (int)(e + 3); *++e = JMP; d = ++e;  // jump to else part
      expr(Cond);
      *d = (int)(e + 1);
    }
    else if (tk == Lor) { next(); *++e = BNZ; d = ++e; expr(Lan); *d = (int)(e + 1); ty = INT; }  // ||
    else if (tk == Lan) { next(); *++e = BZ;  d = ++e; expr(Or);  *d = (int)(e + 1); ty = INT; }  // &&
    else if (tk == Or)  { next(); *++e = PSH; expr(Xor); *++e = OR;  ty = INT; }  // |
    else if (tk == Xor) { next(); *++e = PSH; expr(And); *++e = XOR; ty = INT; }  // ^
    else if (tk == And) { next(); *++e = PSH; expr(Eq);  *++e = AND; ty = INT; }  // &
    else if (tk == Eq)  { next(); *++e = PSH; expr(Lt);  *++e = EQ;  ty = INT; }  // ==
    else if (tk == Ne)  { next(); *++e = PSH; expr(Lt);  *++e = NE;  ty = INT; }  // !=
    else if (tk == Lt)  { next(); *++e = PSH; expr(Shl); *++e = LT;  ty = INT; }  // <
    else if (tk == Gt)  { next(); *++e = PSH; expr(Shl); *++e = GT;  ty = INT; }  // >
    else if (tk == Le)  { next(); *++e = PSH; expr(Shl); *++e = LE;  ty = INT; }  // <=
    else if (tk == Ge)  { next(); *++e = PSH; expr(Shl); *++e = GE;  ty = INT; }  // >=
    else if (tk == Shl) { next(); *++e = PSH; expr(Add); *++e = SHL; ty = INT; }  // <<
    else if (tk == Shr) { next(); *++e = PSH; expr(Add); *++e = SHR; ty = INT; }  // >>
    else if (tk == Add) {  // handle addition (+)
      next(); *++e = PSH; expr(Mul);
      if ((ty = t) > PTR) { *++e = PSH; *++e = IMM; *++e = sizeof(int); *++e = MUL; }  // pointer arithmetic
      *++e = ADD;
    }
    else if (tk == Sub) {  // handle subtraction (-)
      next(); *++e = PSH; expr(Mul);
      if (t > PTR && t == ty) { *++e = SUB; *++e = PSH; *++e = IMM; *++e = sizeof(int); *++e = DIV; ty = INT; }  // pointer - pointer
      else if ((ty = t) > PTR) { *++e = PSH; *++e = IMM; *++e = sizeof(int); *++e = MUL; *++e = SUB; }  // pointer - int
      else *++e = SUB;  // int - int
    }
    else if (tk == Mul) { next(); *++e = PSH; expr(Inc); *++e = MUL; ty = INT; }  // *
    else if (tk == Div) { next(); *++e = PSH; expr(Inc); *++e = DIV; ty = INT; }  // /
    else if (tk == Mod) { next(); *++e = PSH; expr(Inc); *++e = MOD; ty = INT; }  // %
    else if (tk == Inc || tk == Dec) {  // handle x++ and x-- (post-increment/decrement)
      if (*e == LC) { *e = PSH; *++e = LC; }
      else if (*e == LI) { *e = PSH; *++e = LI; }
      else { printf("%d: bad lvalue in post-increment\n", line); exit(-1); }
      *++e = PSH; *++e = IMM; *++e = (ty > PTR) ? sizeof(int) : sizeof(char);
      *++e = (tk == Inc) ? ADD : SUB;
      *++e = PSH; *++e = IMM; *++e = (ty > PTR) ? sizeof(int) : sizeof(char);
      *++e = (tk == Inc) ? SUB : ADD;
      next();
    }
    else if (tk == Brak) {  // handle array access (like a[b])
      next(); *++e = PSH; expr(Assign);
      if (tk == ']') next(); else { printf("%d: close bracket expected\n", line); exit(-1); }
      if (t > PTR) { *++e = PSH; *++e = IMM; *++e = sizeof(int); *++e = MUL; }  // scale index by element size
      else if (t < PTR) { printf("%d: pointer type expected\n", line); exit(-1); }
      *++e = ADD;  // add index to array base
      *++e = ((ty = t - PTR) == CHAR) ? LC : LI;  // load the value
    }
    else { printf("%d: compiler error tk=%d\n", line, tk); exit(-1); }
  }
}

// stmt() understands statements - these are complete instructions like
// if/while/return. It's like understanding complete sentences instead of just words
void stmt()
{
  int *a, *b;  // used for storing jump addresses

  if (tk == If) {  // handle if statements
    next();
    if (tk == '(') next(); else { printf("%d: open paren expected\n", line); exit(-1); }
    expr(Assign);  // parse condition
    if (tk == ')') next(); else { printf("%d: close paren expected\n", line); exit(-1); }
    *++e = BZ; b = ++e;  // add jump instruction for false condition
    stmt();  // parse statement in if block
    if (tk == Else) {  // handle optional else
      *b = (int)(e + 3); *++e = JMP; b = ++e;
      next();
      stmt();  // parse statement in else block
    }
    *b = (int)(e + 1);  // patch jump address
  }
  else if (tk == While) {  // handle while loops
    next();
    a = e + 1;  // save address for loop start
    if (tk == '(') next(); else { printf("%d: open paren expected\n", line); exit(-1); }
    expr(Assign);  // parse condition
    if (tk == ')') next(); else { printf("%d: close paren expected\n", line); exit(-1); }
    *++e = BZ; b = ++e;  // add jump instruction for false condition
    stmt();  // parse statement in while block
    *++e = JMP; *++e = (int)a;  // jump back to start
    *b = (int)(e + 1);  // patch jump address
  }
  else if (tk == Return) {  // handle return statement
    next();
    if (tk != ';') expr(Assign);  // parse return value
    *++e = LEV;  // add return instruction
    if (tk == ';') next(); else { printf("%d: semicolon expected\n", line); exit(-1); }
  }
  else if (tk == '{') {  // handle block of statements
    next();
    while (tk != '}') stmt();  // parse statements until closing brace
    next();
  }
  else if (tk == ';') {  // handle empty statement
    next();
  }
  else {  // handle expression statement
    expr(Assign);
    if (tk == ';') next(); else { printf("%d: semicolon expected\n", line); exit(-1); }
  }
}

// main() is where everything starts - it reads the input file and
// coordinates all the other parts of the compiler
int main(int argc, char **argv) {
    int fd, bt, ty, poolsz, *idmain;
    int *pc, *sp, *bp, a, cycle; // vm registers
    int i, *t; // temps

    // Process command line arguments
    --argc; ++argv;
    if (argc > 0 && **argv == '-' && (*argv)[1] == 's') { src = 1; --argc; ++argv; }
    if (argc > 0 && **argv == '-' && (*argv)[1] == 'd') { debug = 1; --argc; ++argv; }
    if (argc < 1) { printf("usage: c4 [-s] [-d] file ...\n"); return -1; }

    // Initialize memory pools
    poolsz = 256*1024; // arbitrary size
    if (!(sym = malloc(poolsz))) { printf("could not malloc(%d) symbol area\n", poolsz); return -1; }
    if (!(le = e = malloc(poolsz))) { printf("could not malloc(%d) text area\n", poolsz); return -1; }
    if (!(data = malloc(poolsz))) { printf("could not malloc(%d) data area\n", poolsz); return -1; }
    if (!(sp = malloc(poolsz))) { printf("could not malloc(%d) stack area\n", poolsz); return -1; }

    // Initialize symbol table with keywords
    memset(sym,  0, poolsz);
    memset(e,    0, poolsz);
    memset(data, 0, poolsz);

    p = "char else enum if int return sizeof while "
        "open read close printf malloc free memset memcmp exit void main";
    
    // Add keywords to symbol table
    i = Char;
    while (i <= While) { // Add keywords
        next();
        id[Tk] = i++;
    }
    i = OPEN;
    while (i <= EXIT) { // Add library functions
        next();
        id[Class] = Sys;
        id[Type] = INT;
        id[Val] = i++;
    }
    next(); id[Tk] = Char; // handle void type
    next(); idmain = id; // keep track of main

  // read program into memory
  if (!(lp = p = malloc(poolsz))) { printf("could not malloc(%d) source area\n", poolsz); return -1; }
  if ((i = read(fd, p, poolsz-1)) <= 0) { printf("read() returned %d\n", i); return -1; }
  p[i] = 0;  // null terminate
  close(fd);

  // parse declarations
  line = 1;
  next();
  while (tk) {  // parse declarations
    bt = INT; // basetype
    if (tk == Int) next();
    else if (tk == Char) { next(); bt = CHAR; }
    else if (tk == Enum) {  // handle enum declaration
      next();
      if (tk != '{') next();
      if (tk == '{') {
        next();
        i = 0;  // enum value
        while (tk != '}') {
          if (tk != Id) { printf("%d: bad enum identifier %d\n", line, tk); return -1; }
          next();
          if (tk == Assign) {  // handle explicit enum value
            next();
            if (tk != Num) { printf("%d: bad enum initializer\n", line); return -1; }
            i = ival;
            next();
          }
          id[Class] = Num; id[Type] = INT; id[Val] = i++;  // add to symbol table
          if (tk == ',') next();
        }
        next();
      }
    }
    
    // parse declarations
    while (tk != ';' && tk != '}') {
      ty = bt;  // current type
      while (tk == Mul) { next(); ty = ty + PTR; }  // handle pointers
      if (tk != Id) { printf("%d: bad global declaration\n", line); return -1; }
      if (id[Class]) { printf("%d: duplicate global definition\n", line); return -1; }
      next();
      id[Type] = ty;
      if (tk == '(') {  // function
        id[Class] = Fun;
        id[Val] = (int)(e + 1);
        next(); i = 0;
        while (tk != ')') {  // parse parameters
          ty = INT;
          if (tk == Int) next();
          else if (tk == Char) { next(); ty = CHAR; }
          while (tk == Mul) { next(); ty = ty + PTR; }
          if (tk != Id) { printf("%d: bad parameter declaration\n", line); return -1; }
          if (id[Class] == Loc) { printf("%d: duplicate parameter definition\n", line); return -1; }
          id[HClass] = id[Class]; id[Class] = Loc;
          id[HType]  = id[Type];  id[Type] = ty;
          id[HVal]   = id[Val];   id[Val] = i++;
          next();
          if (tk == ',') next();
        }
        next();
        if (tk != '{') { printf("%d: bad function definition\n", line); return -1; }
        loc = ++i;
        next();
        while (tk == Int || tk == Char) {  // parse local declarations
          bt = (tk == Int) ? INT : CHAR;
          next();
          while (tk != ';') {
            ty = bt;
            while (tk == Mul) { next(); ty = ty + PTR; }
            if (tk != Id) { printf("%d: bad local declaration\n", line); return -1; }
            if (id[Class] == Loc) { printf("%d: duplicate local definition\n", line); return -1; }
            id[HClass] = id[Class]; id[Class] = Loc;
            id[HType]  = id[Type];  id[Type] = ty;
            id[HVal]   = id[Val];   id[Val] = ++i;
            next();
            if (tk == ',') next();
          }
          next();
        }
        *++e = ENT; *++e = i - loc;  // enter function
        while (tk != '}') stmt();  // parse statements
        *++e = LEV;  // add leave instruction
        id = sym;  // unwind symbol table locals
        while (id[Tk]) {
          if (id[Class] == Loc) {
            id[Class] = id[HClass];
            id[Type] = id[HType];
            id[Val] = id[HVal];
          }
          id = id + Idsz;
        }
      }
      else {  // global variable
        id[Class] = Glo;
        id[Val] = (int)data;
        data = data + sizeof(int);
      }
      if (tk == ',') next();
    }
    next();
  }

  // setup virtual machine and run
  if (!(pc = (int *)idmain[Val])) { printf("main() not defined\n"); return -1; }
  if (src) return 0;  // return if we only wanted to parse

  // setup stack
  bp = sp = (int *)((int)sp + poolsz);
  *--sp = EXIT;  // call exit if main returns
  *--sp = PSH; t = sp;
  *--sp = argc;
  *--sp = (int)argv;
  *--sp = (int)t;

  // run virtual machine
  cycle = 0;
  while (1) {
    i = *pc++; ++cycle;
    if (debug) {  // print debug info
      printf("%d> %.4s", cycle,
        &"LEA ,IMM ,JMP ,JSR ,BZ  ,BNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,SI  ,SC  ,PSH ,"
         "OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,"
         "OPEN,READ,CLOS,PRTF,MALC,FREE,MSET,MCMP,EXIT,"[i * 5]);
      if (i <= ADJ) printf(" %d\n", *pc); else printf("\n");
    }
    if      (i == LEA) a = (int)(bp + *pc++);                             // load local address
    else if (i == IMM) a = *pc++;                                         // load immediate
    else if (i == JMP) pc = (int *)*pc;                                   // jump
    else if (i == JSR) { *--sp = (int)(pc + 1); pc = (int *)*pc; }        // jump to subroutine
    else if (i == BZ)  pc = a ? pc + 1 : (int *)*pc;                      // branch if zero
    else if (i == BNZ) pc = a ? (int *)*pc : pc + 1;                      // branch if not zero
    else if (i == ENT) { *--sp = (int)bp; bp = sp; sp = sp - *pc++; }     // enter subroutine
    else if (i == ADJ) sp = sp + *pc++;                                   // stack adjust
    else if (i == LEV) { sp = bp; bp = (int *)*sp++; pc = (int *)*sp++; } // leave subroutine
    else if (i == LI)  a = *(int *)a;                                     // load int
    else if (i == LC)  a = *(char *)a;                                    // load char
    else if (i == SI)  *(int *)*sp++ = a;                                 // store int
    else if (i == SC)  a = *(char *)*sp++ = a;                            // store char
    else if (i == PSH) *--sp = a;                                         // push

    else if (i == OR)  a = *sp++ |  a;  // bitwise or
    else if (i == XOR) a = *sp++ ^  a;  // bitwise xor
    else if (i == AND) a = *sp++ &  a;  // bitwise and
    else if (i == EQ)  a = *sp++ == a;  // equal
    else if (i == NE)  a = *sp++ != a;  // not equal
    else if (i == LT)  a = *sp++ <  a;  // less than
    else if (i == GT)  a = *sp++ >  a;  // greater than
    else if (i == LE)  a = *sp++ <= a;  // less than or equal
    else if (i == GE)  a = *sp++ >= a;  // greater than or equal
    else if (i == SHL) a = *sp++ << a;  // shift left
    else if (i == SHR) a = *sp++ >> a;  // shift right
    else if (i == ADD) a = *sp++ +  a;  // add
    else if (i == SUB) a = *sp++ -  a;  // subtract
    else if (i == MUL) a = *sp++ *  a;  // multiply
    else if (i == DIV) a = *sp++ /  a;  // divide
    else if (i == MOD) a = *sp++ %  a;  // modulo

    else if (i == OPEN) a = open((char *)sp[1], *sp);  // open file
    else if (i == READ) a = read(sp[2], (char *)sp[1], *sp);  // read from file
    else if (i == CLOS) a = close(*sp);  // close file
    else if (i == PRTF) { t = sp + pc[1]; a = printf((char *)t[-1], t[-2], t[-3], t[-4], t[-5], t[-6]); }  // printf
    else if (i == MALC) a = (int)malloc(*sp);  // malloc
    else if (i == FREE) free((void *)*sp);  // free
    else if (i == MSET) a = (int)memset((char *)sp[2], sp[1], *sp);  // memset
    else if (i == MCMP) a = memcmp((char *)sp[2], (char *)sp[1], *sp);  // memcmp
    else if (i == EXIT) { printf("exit(%d) cycle = %d\n", *sp, cycle); return *sp; }  // exit
    else { printf("unknown instruction = %d! cycle = %d\n", i, cycle); return -1; }  // unknown instruction
  }
}