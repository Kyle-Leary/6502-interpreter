#include "lexer.h"
#include "defines.h"
#include "mempool.h"
#include "util.h"

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PEEK (l->text[l->pos + 1])
// macros to bump the lexer pointers up by a specific amount, with error
// handling.
#define BRK_NEXT(num_bumps)                                                    \
  {                                                                            \
    l->pos += num_bumps;                                                       \
    if (l->pos >= INPUT_LEN) {                                                 \
      break;                                                                   \
    }                                                                          \
    l->curr_char = l->text[l->pos];                                            \
  }
#define RET_NEXT(num_bumps)                                                    \
  {                                                                            \
    l->pos += num_bumps;                                                       \
    if (l->pos >= INPUT_LEN) {                                                 \
      return;                                                                  \
    }                                                                          \
    l->curr_char = l->text[l->pos];                                            \
  }
// dumb
#define RET_TOKEN_NEXT(num_bumps)                                              \
  {                                                                            \
    l->pos += num_bumps;                                                       \
    if (l->pos >= INPUT_LEN) {                                                 \
      return (Token){EMPTY, value};                                            \
    }                                                                          \
    l->curr_char = l->text[l->pos];                                            \
  }

// this is only for bumping the token internally through the next()
// function. doesn't need to be exposed? token_at_cursor leaves the lexer
// state pointing to the last character in the lexeme. for example, if the
// lexeme parsed out is 'a', the cursor will point to the last ' in the
// literal decl after returning from the token_at_cursor method.
static Token token_at_cursor(Lexer *l) {
  // we have our own special definition of NEXT for this function and the other
  // one. this one break;s instead of return;ing
  Lexeme l_type = LEXEME_NULL;
  TokenValue value = 0;

  // we can logically group lexemes into one-character and multi-character ones.
  char ch = l->curr_char;

  printf("Trying to convert one-character lexeme from '%c'.\n", ch);
  if (ch == 0) {
    printf("Found the EOF! Returning EMPTY token...\n");
    l_type = EMPTY;
  } else if ((l->curr_char == '\'') && (isalnum(PEEK))) { // PARSE CHAR LITERAL
    // parse out the literals first, since the ' rule would take precedence over
    // the char literal rule otherwise.
    // TODO: handle escape codes in both char and string literal lexing.
    l_type = CHAR_LITERAL;
    RET_TOKEN_NEXT(1);
    value = l->curr_char;

    // bump past the literal, to the last ' in the literal decl.
    RET_TOKEN_NEXT(1);
  } else if (l->curr_char == '\"') { // PARSE STRING LITERAL
    int sz = 0;

    l_type = STRING_LITERAL;
    RET_TOKEN_NEXT(1); // bump past the first " in the string literal, into the
                       // real string.

    char literal_buf[MAX_STR_LITERAL_SIZE];

    char literal_ch = l->curr_char;
    while (literal_ch != '\"') { // until it hits the other "
      if (sz >= MAX_STR_LITERAL_SIZE) {
        while (literal_ch != '\"') { // if we overflow the max literal size,
                                     // still keep reading out the literal, just
                                     // don't save it to the buffer or the size.
          RET_TOKEN_NEXT(1);
          literal_ch = l->curr_char;
        }
        break;
      }

      literal_buf[sz] = literal_ch;
      sz++;
      RET_TOKEN_NEXT(
          1); // then, bump the pointer and read from the new literal value.
      literal_ch = l->curr_char;
    }

    // then bump the cursor back to the " in the string literal decl.
    RET_TOKEN_NEXT(-1);

    char *temp_value = (char *)malloc(
        sz + 1);           // malloc the size, then copy the buffer right in.
    temp_value[sz] = '\0'; // null term all strings?

    memcpy(temp_value, literal_buf, sz);
    value =
        (TokenValue)temp_value; // then, return the raw pointer to the string as
                                // the token value, so that everything else can
                                // easily access it through the ast or whatever.

  } else if ((l->curr_char == '0') && (PEEK == 'b')) { // binary literal

    l_type = BINARY_LITERAL;

    // bump past the 0b, only parse the actual number in the literal syntax.
    RET_TOKEN_NEXT(2);

    char literal_buf[MAX_STR_LITERAL_SIZE];

    int i = 0;

    char literal_ch =
        l->curr_char; // just like the INT_LITERAL parsing, just keep parsing
                      // until we don't see a number anymore.
#define IS_CH_BOOL(ch) ((ch == '0') || (ch == '1'))
    while (IS_CH_BOOL(literal_ch)) {
      if (i >= MAX_STR_LITERAL_SIZE) {
        while (IS_CH_BOOL(literal_ch)) {
          RET_TOKEN_NEXT(1);
          literal_ch = l->curr_char;
        }
        break;
      }

      literal_buf[i] = literal_ch;
      i++;
      RET_TOKEN_NEXT(
          1); // then, bump the pointer and read from the new literal value.
      literal_ch = l->curr_char;
    }
#undef IS_CH_BOOL

    RET_TOKEN_NEXT(-1);

    value = binary_to_int(literal_buf, i);

  } else if (((l->curr_char == '0') && (PEEK == 'x')) ||
             ((l->curr_char == '$'))) { // hex literal

    l_type = HEX_LITERAL; // assume it's an INT for now.

    // bump past the appropriate amount of header characters and parse the
    // actual number.
    if (l->curr_char == '0') {
      RET_TOKEN_NEXT(2);
    } else if (l->curr_char == '$') {
      RET_TOKEN_NEXT(1);
    }

    char literal_buf[MAX_STR_LITERAL_SIZE];

    int i = 0;

    // if the char is a digit or it's in the a - f hex range on the ascii table.
#define IS_CH_HEX(ch) ((isdigit(ch)) || ((ch >= 97) && (ch <= 102)))

    char literal_ch = l->curr_char;
    while (IS_CH_HEX(literal_ch)) {
      if (i >= MAX_STR_LITERAL_SIZE) {
        while (IS_CH_HEX(literal_ch)) {
          RET_TOKEN_NEXT(1);
          literal_ch = l->curr_char;
        }
        break;
      }

      literal_buf[i] = literal_ch;
      i++;
      RET_TOKEN_NEXT(
          1); // then, bump the pointer and read from the new literal value.
      literal_ch = l->curr_char;
    }
#undef IS_CH_HEX

    RET_TOKEN_NEXT(-1);

    // don't need a nullterm, we pass the length directly.
    value = hex_to_int(literal_buf, i);

    // then, try to convert the value to an int, and put that in the token
    // value.
  } else if (isdigit(l->curr_char)) { // parse numeric literal.

    // TODO: how to parse floats as well in this logic?
    l_type = INT_LITERAL; // assume it's an INT for now.

    char literal_buf[MAX_STR_LITERAL_SIZE];

    int i = 0;

    char literal_ch = l->curr_char;
    while (isdigit(literal_ch)) {
      if (i >= MAX_STR_LITERAL_SIZE) {
        while (isdigit(literal_ch)) {
          RET_TOKEN_NEXT(1);
          literal_ch = l->curr_char;
        }
        break;
      }

      literal_buf[i] = literal_ch;
      i++;
      RET_TOKEN_NEXT(
          1); // then, bump the pointer and read from the new literal value.
      literal_ch = l->curr_char;
    }

    RET_TOKEN_NEXT(-1);

    // null term, then convert the number.
    literal_buf[i] = '\0';
    value = atoi(literal_buf);

  } else {

    // can treat a char like a num and switch over it.
    switch (ch) {
      // first, handle the trivial one-character cases.
    case '+':
      l_type = ADD;
      break;
    case '-':
      l_type = SUB;
      break;
    case '*':
      l_type = MUL;
      break;
    case '/':
      l_type = DIV;
      break;
    case '(':
      l_type = LPAREN;
      break;
    case ')':
      l_type = RPAREN;
      break;
    case '{':
      l_type = LCURLY;
      break;
    case '}':
      l_type = RCURLY;
      break;
    // no SEMI here, it's not useful as a single lexable token.
    case '.':
      l_type = DOT;
      break;
    case '$':
      l_type = DOLLAR;
      break;
    case '#':
      l_type = HASHTAG;
      break;
    case ':':
      l_type = COLON;
      break;
    case '\n':
      l_type = NEWLINE;
      break;
    case ',':
      l_type = COMMA;
      break;
    case '=':
      l_type = EQUAL;
      break;
    case '\'':
      l_type = SINGLE_QUOTE;
      break;
    case '\"':
      l_type = DOUBLE_QUOTE;
      break;
    default: {
      // now, handle things that aren't just simple one-character lexemes.
      // using strlen later, need to zero-alloc this.
      char keyword_buf[MAX_KW_LEN] = {0};
      int i = 0;

      // then, try to parse out keywords if everything else fails.
      while (isalnum(l->curr_char) ||
             l->curr_char == '_') { // while it's a valid character that can
                                    // appear in a keyword or identifier:
        if (i >= MAX_KW_LEN)        // don't overrun the keyword buffer either.
          break;
        keyword_buf[i] =
            l->curr_char; // modify the lexer state directly in the next
                          // function, it's required. the keyword functions
                          // AREN'T a lookahead, they actually move the state
                          // itself ahead.
        i++;
        BRK_NEXT(1); // will automatically break if we overrun the Lexer text
                     // buffer.
      }

      BRK_NEXT(-1);

      printf("Finished parsing keyword from the lexer [%s]\n", keyword_buf);

      // parse all the opcode keywords
      if (strncmp(keyword_buf, "adc", 3) == 0) {
        l_type = ADC;
      } else if (strncmp(keyword_buf, "and", 3) == 0) {
        l_type = AND;
      } else if (strncmp(keyword_buf, "asl", 3) == 0) {
        l_type = ASL;
      } else if (strncmp(keyword_buf, "bcc", 3) == 0) {
        l_type = BCC;
      } else if (strncmp(keyword_buf, "bcs", 3) == 0) {
        l_type = BCS;
      } else if (strncmp(keyword_buf, "beq", 3) == 0) {
        l_type = BEQ;
      } else if (strncmp(keyword_buf, "bit", 3) == 0) {
        l_type = BIT;
      } else if (strncmp(keyword_buf, "bmi", 3) == 0) {
        l_type = BMI;
      } else if (strncmp(keyword_buf, "bne", 3) == 0) {
        l_type = BNE;
      } else if (strncmp(keyword_buf, "bpl", 3) == 0) {
        l_type = BPL;
      } else if (strncmp(keyword_buf, "brk", 3) == 0) {
        l_type = BRK;
      } else if (strncmp(keyword_buf, "bvc", 3) == 0) {
        l_type = BVC;
      } else if (strncmp(keyword_buf, "bvs", 3) == 0) {
        l_type = BVS;
      } else if (strncmp(keyword_buf, "clc", 3) == 0) {
        l_type = CLC;
      } else if (strncmp(keyword_buf, "cld", 3) == 0) {
        l_type = CLD;
      } else if (strncmp(keyword_buf, "cli", 3) == 0) {
        l_type = CLI;
      } else if (strncmp(keyword_buf, "clv", 3) == 0) {
        l_type = CLV;
      } else if (strncmp(keyword_buf, "cmp", 3) == 0) {
        l_type = CMP;
      } else if (strncmp(keyword_buf, "cpx", 3) == 0) {
        l_type = CPX;
      } else if (strncmp(keyword_buf, "cpy", 3) == 0) {
        l_type = CPY;
      } else if (strncmp(keyword_buf, "dec", 3) == 0) {
        l_type = DEC;
      } else if (strncmp(keyword_buf, "dex", 3) == 0) {
        l_type = DEX;
      } else if (strncmp(keyword_buf, "dey", 3) == 0) {
        l_type = DEY;
      } else if (strncmp(keyword_buf, "eor", 3) == 0) {
        l_type = EOR;
      } else if (strncmp(keyword_buf, "inc", 3) == 0) {
        l_type = INC;
      } else if (strncmp(keyword_buf, "inx", 3) == 0) {
        l_type = INX;
      } else if (strncmp(keyword_buf, "iny", 3) == 0) {
        l_type = INY;
      } else if (strncmp(keyword_buf, "jmp", 3) == 0) {
        l_type = JMP;
      } else if (strncmp(keyword_buf, "jsr", 3) == 0) {
        l_type = JSR;
      } else if (strncmp(keyword_buf, "lda", 3) == 0) {
        l_type = LDA;
      } else if (strncmp(keyword_buf, "ldx", 3) == 0) {
        l_type = LDX;
      } else if (strncmp(keyword_buf, "ldy", 3) == 0) {
        l_type = LDY;
      } else if (strncmp(keyword_buf, "lsr", 3) == 0) {
        l_type = LSR;
      } else if (strncmp(keyword_buf, "nop", 3) == 0) {
        l_type = NOP;
      } else if (strncmp(keyword_buf, "ora", 3) == 0) {
        l_type = ORA;
      } else if (strncmp(keyword_buf, "pha", 3) == 0) {
        l_type = PHA;
      } else if (strncmp(keyword_buf, "php", 3) == 0) {
        l_type = PHP;
      } else if (strncmp(keyword_buf, "pla", 3) == 0) {
        l_type = PLA;
      } else if (strncmp(keyword_buf, "plp", 3) == 0) {
        l_type = PLP;
      } else if (strncmp(keyword_buf, "rol", 3) == 0) {
        l_type = ROL;
      } else if (strncmp(keyword_buf, "ror", 3) == 0) {
        l_type = ROR;
      } else if (strncmp(keyword_buf, "rti", 3) == 0) {
        l_type = RTI;
      } else if (strncmp(keyword_buf, "rts", 3) == 0) {
        l_type = RTS;
      } else if (strncmp(keyword_buf, "sbc", 3) == 0) {
        l_type = SBC;
      } else if (strncmp(keyword_buf, "sec", 3) == 0) {
        l_type = SEC;
      } else if (strncmp(keyword_buf, "sed", 3) == 0) {
        l_type = SED;
      } else if (strncmp(keyword_buf, "sei", 3) == 0) {
        l_type = SEI;
      } else if (strncmp(keyword_buf, "sta", 3) == 0) {
        l_type = STA;
      } else if (strncmp(keyword_buf, "stx", 3) == 0) {
        l_type = STX;
      } else if (strncmp(keyword_buf, "sty", 3) == 0) {
        l_type = STY;
      } else if (strncmp(keyword_buf, "tax", 3) == 0) {
        l_type = TAX;
      } else if (strncmp(keyword_buf, "tay", 3) == 0) {
        l_type = TAY;
      } else if (strncmp(keyword_buf, "tsx", 3) == 0) {
        l_type = TSX;
      } else if (strncmp(keyword_buf, "txa", 3) == 0) {
        l_type = TXA;
      } else if (strncmp(keyword_buf, "txs", 3) == 0) {
        l_type = TXS;
      } else if (strncmp(keyword_buf, "tya", 3) == 0) {
        l_type = TYA;

      } else { // else, parse the ID out of the keyword_buf, since it's clearly
               // not a keyword.
        // TODO: there has GOT to be a better way than callocing every time.
        // this sucks hard.
        unsigned long key_sz = strlen(keyword_buf);
        char *id_string_ptr =
            (char *)calloc(key_sz + 1, 1); // punn the pointer as a TokenValue,
                                           // it's 64_t so it doesn't matter.
        memcpy(id_string_ptr, keyword_buf, key_sz);
        id_string_ptr[key_sz] = '\0'; // MAKE SURE TO NULL TERM THE ID STRING
        value = (TokenValue)
            id_string_ptr; // then just punn the pointer back into a TokenValue
                           // and pass it through, so the name of the ID can be
                           // accessed later in the AST.
        l_type = ID;
      }
    } break;
    }
  }

  return (Token){l_type, value};
}

// try to cast the current character to an int, and return it.
int get_int(Lexer *l) { return char_to_int(l->curr_char); }

// handle whitespace skipping and comment skipping, right here in the lexer.
void next(Lexer *l) {
  // just align to the next character in the lexer.

begin_next:

  RET_NEXT(1);

  // try to avoid calling the lexeme_from_char full lexing function if we
  // don't have to, like in the case of comments and whitespace.
  if ((l->curr_char == ' ') || (l->curr_char == '\t')) {
    goto begin_next; // FUCK recursion.
  }

  // assembler comments, just the ; broken by a \n.
  if (l->curr_char == ';') {
    // line comment found
    RET_NEXT(1);
    while (l->curr_char != '\n') {
      RET_NEXT(1); // until the next newline, where the comment breaks.
    }
    // don't goto the begin of this function, since we actually DO need to parse
    // the \n, it's a meaningful token.
  }

  l->curr_token = token_at_cursor(l);
}

void eat(Lexer *lx, Lexeme l) {
  if (lx->curr_token.type != l) {
    error(
        "Eat error: lexemes did not match - Your \"%s\" vs the lexer's \"%s\".",
        lexeme_to_string(l), lexeme_to_string(lx->curr_token.type));
  }
  next(lx);
}

// casting functions, cast from Lexeme type to other helper enums.
BinopType binop_from_lexeme(Lexeme l) {
  switch (l) {
  case ADD:
    return BO_ADD;
    break;
  case SUB:
    return BO_SUB;
    break;
  case MUL:
    return BO_MUL;
    break;
  case DIV:
    return BO_DIV;
    break;
  default:
    return BO_NULL;
    break;
  }
}

// takes in a keyword lexeme, returns the associated datatype.
DataType datatype_from_lexeme(Lexeme l) {
  switch (l) {
  case KW_INT:
    return DT_INT;
    break;
  case KW_CHAR:
    return DT_CHAR;
    break;
  case KW_VOID:
    return DT_VOID;
    break;
  default:
    return DT_VOID;
    break;
  }
}

// is the lexeme passed in an instruction keyword?
// if the INSTRUCTION_MASK bit is flipped, then yes, it's in that range.
bool is_instruction(Lexeme l) { return ((l & INSTRUCTION_MASK) != 0); }
bool is_keyword(Lexeme l) { return ((l & KEYWORD_MASK) != 0); }

// returns a statically allocated read-only string literal in the program's data
// section, that's why we can safely return a const char* here and use it in the
// caller without mallocing anything.
const char *lexeme_to_string(Lexeme lexeme) {
  switch (lexeme) {
  case LEXEME_NULL:
    return "LEXEME_NULL";
  case INT_LITERAL:
    return "INT_LITERAL";
  case FLOAT_LITERAL:
    return "FLOAT_LITERAL";
  case CHAR_LITERAL:
    return "CHAR_LITERAL";
  case STRING_LITERAL:
    return "STRING_LITERAL";
  case KW_INT:
    return "KW_INT";
  case KW_CHAR:
    return "KW_CHAR";
  case KW_VOID:
    return "KW_VOID";
  case KW_AUTO:
    return "KW_AUTO";
  case KW_BREAK:
    return "KW_BREAK";
  case KW_CASE:
    return "KW_CASE";
  case KW_CONST:
    return "KW_CONST";
  case KW_CONTINUE:
    return "KW_CONTINUE";
  case KW_DEFAULT:
    return "KW_DEFAULT";
  case KW_DO:
    return "KW_DO";
  case KW_DOUBLE:
    return "KW_DOUBLE";
  case KW_ELSE:
    return "KW_ELSE";
  case KW_ENUM:
    return "KW_ENUM";
  case KW_EXTERN:
    return "KW_EXTERN";
  case KW_FLOAT:
    return "KW_FLOAT";
  case KW_FOR:
    return "KW_FOR";
  case KW_GOTO:
    return "KW_GOTO";
  case KW_IF:
    return "KW_IF";
  case KW_LONG:
    return "KW_LONG";
  case KW_REGISTER:
    return "KW_REGISTER";
  case KW_RETURN:
    return "KW_RETURN";
  case KW_SHORT:
    return "KW_SHORT";
  case KW_SIGNED:
    return "KW_SIGNED";
  case KW_SIZEOF:
    return "KW_SIZEOF";
  case KW_STATIC:
    return "KW_STATIC";
  case KW_STRUCT:
    return "KW_STRUCT";
  case KW_SWITCH:
    return "KW_SWITCH";
  case KW_TYPEDEF:
    return "KW_TYPEDEF";
  case KW_UNION:
    return "KW_UNION";
  case KW_UNSIGNED:
    return "KW_UNSIGNED";
  case KW_VOLATILE:
    return "KW_VOLATILE";
  case KW_WHILE:
    return "KW_WHILE";
  case ID:
    return "ID";
  case ADD:
    return "ADD";
  case SUB:
    return "SUB";
  case MUL:
    return "MUL";
  case DIV:
    return "DIV";
  case LPAREN:
    return "LPAREN";
  case RPAREN:
    return "RPAREN";
  case LCURLY:
    return "LCURLY";
  case RCURLY:
    return "RCURLY";
  case SEMI:
    return "SEMI";
  case COMMA:
    return "COMMA";
  case COLON:
    return "COLON";
  case EQUAL:
    return "EQUAL";
  case DOUBLE_EQUAL:
    return "DOUBLE_EQUAL";
  case SINGLE_QUOTE:
    return "SINGLE_QUOTE";
  case DOUBLE_QUOTE:
    return "DOUBLE_QUOTE";
  case WHITESPACE:
    return "WHITESPACE";
  case EMPTY:
    return "EMPTY";
  case NEWLINE:
    return "NEWLINE";
  default:
    return "UNKNOWN_LEXEME";
  }
}

// don't expose the static methods, just expose one testing method for the main
// testing function to call.
void test_lexer() {
  // re-clear the lexer and copy in the string literal.
#define SETUP_LEX(text_input_literal)                                          \
  {                                                                            \
    memset(l, 0, sizeof(Lexer));                                               \
    const char *text_input = text_input_literal;                               \
    l->text_len = strlen(text_input);                                          \
    l->pos = -1;                                                               \
    l->curr_token.type = EMPTY;                                                \
    memcpy(l->text, text_input, l->text_len);                                  \
  }

  printf("\nBEGIN LEXER TESTING:\n\n\n");

  Lexer *l = (Lexer *)mempool_alloc(sizeof(Lexer) * 1);

  printf("lexer ptr %p\n", l);

  { // test multiple syms without spaces in between
    SETUP_LEX("+-+");
    next(l);
    ASSERT(l->curr_token.type == ADD, "multi lexeme without spaces (1)");
    next(l);
    ASSERT(l->curr_token.type == SUB, "multi lexeme without spaces (2)");
    next(l);
    ASSERT(l->curr_token.type == ADD, "multi lexeme without spaces (3)");
  }

  {
    SETUP_LEX("'a'");
    next(l);
    ASSERT(l->curr_token.type == CHAR_LITERAL, "char lit");
  }

  {
    SETUP_LEX("1235");
    next(l);
    ASSERT(l->curr_token.type == INT_LITERAL, "int lit");
  }

  {
    // ensure that an empty string will properly show up as the EOF Empty
    // lexeme.
    SETUP_LEX("");
    next(l);
    ASSERT(l->curr_token.type == EMPTY, "empty lexeme lexing");
  }

  { // test ints
    SETUP_LEX("1234");
    next(l);
    ASSERT(l->curr_token.type == INT_LITERAL, "int literal lexing type");
    ASSERT(l->curr_token.value == 1234, "int literal lexing value");
  }

  { // test char literals
    SETUP_LEX("'a'");
    next(l);
    ASSERT(l->curr_token.type == CHAR_LITERAL, "char literal lexing type");
    ASSERT(l->curr_token.value == 'a', "char literal lexing value");
  }

  { // test string literals
    SETUP_LEX("\"my_string\"");
    next(l);
    ASSERT(l->curr_token.type == STRING_LITERAL, "string literal lexing type");
    printf("string literal found: \"%s\"\n", (char *)l->curr_token.value);
    // the token should contain a pointer to the proper string.
    ASSERT(strncmp((char *)l->curr_token.value, "my_string", 9) == 0,
           "string literal lexing value");
  }

  { // test identifiers
    SETUP_LEX("_myId__en__tifier");
    next(l);
    ASSERT(l->curr_token.type == ID, "identifier lexing type");
    printf("identifier found: \"%s\"\n", (char *)l->curr_token.value);
    ASSERT(strncmp((char *)l->curr_token.value, "_myId__en__tifier", 17) == 0,
           "identifier lexing value");
  }

  { // test whitespace
    SETUP_LEX("   \t\n\t\t   ");
    next(l);
    ASSERT(l->curr_token.type == NEWLINE,
           "whitespace lexing (1)"); // stop on the newline
  }

  {
    SETUP_LEX("  \t\t\t  \t\t\t\t \t");
    next(l);
    ASSERT(l->curr_token.type == EMPTY,
           "whitespace lexing (2)"); // stop on the newline
  }

  { // test comments and whitespace
    SETUP_LEX(" ; this is a one-line comment. it breaks on the newline.\n  \t "
              "1234;;; this is a comment. it should be skipped in the \n\n "
              ";lexer. \n");
    next(l);
    ASSERT(l->curr_token.type == NEWLINE, "comment lexing");
    next(l);
    ASSERT(l->curr_token.type == INT_LITERAL, "comment lexing");
    next(l);
    ASSERT(l->curr_token.type == NEWLINE, "comment lexing");
  }

  { // test symbols
    SETUP_LEX("+");
    next(l);
    ASSERT(l->curr_token.type == ADD, "add symbol lexing");
  }

  {
    ASSERT(is_instruction(LDA), "is_instruction LDA test throws true");
    ASSERT((!is_instruction(SEMI)), "is_instruction SEMI test throws false");
  }

  {
    ASSERT(is_keyword(KW_INT), "is_keyword KW_INT test throws true");
    ASSERT((!is_keyword(SEMI)), "is_keyword SEMI test throws false");
  }

  {
    SETUP_LEX("StartLabel:\n");
    next(l);
    ASSERT(l->curr_token.type == ID, "label lexing");
    ASSERT(strncmp((char *)l->curr_token.value, "StartLabel", 10) == 0,
           "label lexing (id value)");
    next(l);
    ASSERT(l->curr_token.type == COLON, "label lexing");
    next(l);
    ASSERT(l->curr_token.type == NEWLINE, "label lexing");
  }

  {
    SETUP_LEX("0x55\n");
    next(l);
    ASSERT(l->curr_token.type == HEX_LITERAL, "hex_literal lexing");
    printf("%lu\n", l->curr_token.value);
    ASSERT(l->curr_token.value == 0x55, "hex_literal lexing (value)");
    next(l);
    ASSERT(l->curr_token.type == NEWLINE, "hex_literal lexing");
  }

  {
    SETUP_LEX("0b01101011\n");
    next(l);
    ASSERT(l->curr_token.type == BINARY_LITERAL, "hex_literal lexing");
    ASSERT(l->curr_token.value == 0b01101011, "hex_literal lexing (value)");
    next(l);
    ASSERT(l->curr_token.type == NEWLINE, "hex_literal lexing");
  }

  // testing features that are not yet implemented.
  // { // test float literals
  //   SETUP_LEX("1234.5678");
  //   next(l);
  //   ASSERT(l->curr_token.type == FLOAT_LITERAL, "float literal lexing type");
  //   ASSERT(l->curr_token.value == 1234.5678, "float literal lexing value");
  // }

  printf("\n\nEND LEXER TESTING.\n");

  mempool_free(l);
#undef SETUP_LEX
}

// these macros use locally-scoped function variables, and aren't useful outside
// of here anyway.
#undef PEEK
#undef BRK_NEXT
#undef RET_NEXT
#undef RET_TOKEN_NEXT
