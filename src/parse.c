#include "parse.h"

#include "arguments.h"
#include "ast.h"
#include "cpu.h"
#include "defines.h"
#include "lexer.h"
#include "util.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// need to have the functions call eachother, be careful with recursion on the
// paren rule. parens are inherently recursive in grammar.
static NodeIndex expr(Lexer *l);

static NodeIndex factor(Lexer *l) {
  NodeIndex ret_val = NULL_INDEX;

  if (l->curr_token.type == INT_LITERAL) { // factor is INT | L expr R
    ret_val = add_node(make_node(
        NT_NUMBER, NULL_INDEX, NULL_INDEX,
        (NodeData){.as_raw_data = get_int(
                       l)})); // put the int in the cursor as the data slot
                              // in the node, and return THAT index.
    eat(l, INT_LITERAL);
  } else if (l->curr_token.type == LPAREN) {
    eat(l, LPAREN);
    ret_val = expr(l); // just parse an expr, and return that node opaquely.
    eat(l, RPAREN);
  } else {
    error("The factor rule could not find either an INT or LPAREN token.\n");
  }

  return ret_val;
}

// basically the same as expr.
static NodeIndex term(Lexer *l) {
  NodeIndex new_root = factor(l);
  NodeIndex second_term = NULL_INDEX;
  Lexeme cl = l->curr_token.type;
  while (cl == MUL || cl == DIV) {
    printf("New expr term: '%c'\n", l->curr_char);
    BinopType bt = binop_from_lexeme(cl);

    if (cl == MUL) {
      eat(l, MUL);
      second_term = factor(l);
    } else if (cl == DIV) {
      eat(l, DIV);
      second_term = factor(l);
    }

    cl = l->curr_token.type; // update the ref near the end.
    new_root = add_node(make_node(
        NT_BINOP, new_root, second_term,
        (NodeData){.as_raw_data =
                       bt})); // keep growing the subtree, and return it
                              // when we can't find any more + or - ops.
  }
  // we make the node, add it to the tree, then return our new index into the
  // array so that the caller can add it to their own root.
  return new_root;
}

static NodeIndex expr(Lexer *l) {
  NodeIndex new_root = term(l);
  NodeIndex second_term = NULL_INDEX;
  Lexeme cl = l->curr_token.type;
  while (cl == ADD || cl == SUB) {
    printf("New expr term: '%c'\n", l->curr_char);
    BinopType bt = binop_from_lexeme(cl);

    if (cl == ADD) {
      eat(l, ADD);
      second_term = term(l);
    } else if (cl == SUB) {
      eat(l, SUB);
      second_term = term(l);
    }

    cl = l->curr_token.type; // update the ref near the end.
    new_root = add_node(make_node(
        NT_BINOP, new_root, second_term,
        (NodeData){.as_raw_data =
                       bt})); // keep growing the subtree, and return it
                              // when we can't find any more + or - ops.
  }
  // we make the node, add it to the tree, then return our new index into the
  // array so that the caller can add it to their own root.
  return new_root;
}

// parse a variable name/ID.
static NodeIndex id(Lexer *l) {
  // punn the 64_t from the tokenvalue to a char*, assume it's a pointer to the
  // ID string data.
  char *id_text_ptr = (char *)l->curr_token.value;
  // eat the ID, move past it.
  eat(l, ID);

  return add_node(make_node(
      NT_ID, NULL_INDEX, NULL_INDEX,
      (NodeData){
          .as_ptr =
              id_text_ptr})); // use the string as the data of this
                              // type. free it on visitation? maybe there could
                              // be a visitation_clean pass that cleans all of
                              // the malloced node data like this?
}

// the "do nothing" statement.
static NodeIndex empty(Lexer *l) {
  return add_node(make_node(NT_EMPTY, NULL_INDEX, NULL_INDEX, NO_NODE_DATA));
}

// this is the main place the argument type is determined. the addressing mode
// is easy, and can be determined entirely by the string format passed to the
// interpreter.
static NodeIndex argument(Lexer *l) {
  Lexeme cl = l->curr_token.type;
  Arg a; // fill this arg from the stack, we're going to use all 64 bits of it
         // and put it in the argument node for easy parsing.

  switch (cl) {
  // immediate handling
  case HASHTAG: {
    // literal 8 bit value
    a.mode = Immediate;
    eat(l, HASHTAG);
    a.value = l->curr_token.value;
    eat(l, HEX_LITERAL);
  } break;

  // determine the ZP and Abs submodes
  case HEX_LITERAL: {
    // 8 or 16 bit address
    a.value = l->curr_token.value;
    eat(l, HEX_LITERAL);
    if (a.value < 256) {
      // general ZP. then, see if there's the ,X and ,Y at the end, and change
      // the mode accordingly.
      a.mode = ZP;
      cl = l->curr_token.type;
      switch (cl) {

      case COMMA: {
        eat(l, COMMA);
        // kind of a hack, instead of ,X and ,Y being a token i'm just using X
        // and Y as general IDs.
        char *id = (char *)l->curr_token.value;

        if (strncmp(id, "X", 1) == 0) {
          a.mode = ZPX;
        } else if (strncmp(id, "Y", 1) == 0) {
          a.mode = ZPY;
        }

        eat(l, ID);
        break;

      default: {
        // do nothing. we're already set to ZP.
      } break;
      }
      }

    } else {
      // very similar to ZP.
      a.mode = Abs;

      cl = l->curr_token.type;
      switch (cl) {

      case COMMA: {
        eat(l, COMMA);
        // kind of a hack, instead of ,X and ,Y being a token i'm just using X
        // and Y as general IDs.
        char *id = (char *)l->curr_token.value;

        if (strncmp(id, "X", 1) == 0) {
          a.mode = AbsX;
        } else if (strncmp(id, "Y", 1) == 0) {
          a.mode = AbsY;
        }

        eat(l, ID);
        break;

      default: {
        // do nothing. we're already set to ZP.
      } break;
      }
      }
    }
  } break;

  // indirect addressing handler.
  // indirect: ($c0c0)
  // indexed indirect: ($c0,X)
  // indirect indexed: ($c0),Y
  case LPAREN: {
    eat(l, LPAREN);

    a.value =
        l->curr_token
            .value; // operate generically over the value, it's just a hex
                    // literal no matter which weird indirect mode we find here.

    eat(l, HEX_LITERAL);
    cl = l->curr_token.type;

    switch (cl) {
    case RPAREN: {
      a.mode = Indirect;
      eat(l, RPAREN);
      if (l->curr_token.type == COMMA) {
        // we're in the ,Y
        a.mode = IndirectIndexed;
        eat(l, COMMA);
        eat(l, ID);
      }
    } break;

    case COMMA: {
      a.mode = IndexedIndirect;
      eat(l, COMMA);
      eat(l, ID);
      eat(l, RPAREN);
    } break;

    default: {
    } break;
    }

  } break;

  case ID: {
    // a label passed in as an argument.
    eat(l, ID);
  } break;

  case NEWLINE: {
    // implicit addressing, no explicit args were found at the cursor right
    // after the instruction.
    a.mode = Implicit; // gah! why are these not all uppercase! i suck!
    eat(l, NEWLINE);
  } break;

  // this is similar to the newline case, but when it's a blank instruction at
  // the end of a file. this is also interpreted as implicit, just like the
  // NEWLINE.
  case EMPTY: {
    a.mode = Implicit;
  } break;

  default: {
    error("INVALID ARGUMENT STARTING TOKEN: %s", lexeme_to_string(cl));
  }
  }

  // parse out an ID, and just put that in the left slot.
  return add_node(
      make_node(NT_ARGUMENT, NULL_INDEX, NULL_INDEX, (NodeData){.as_arg = a}));
}

static NodeIndex statement_list(Lexer *l);

static NodeIndex pragma(Lexer *l) { return NULL_INDEX; }

static NodeIndex label(Lexer *l) {
  // transparent wrapper around an ID, there might be more data here at some
  // point.
  NodeIndex left = id(l);
  eat(l, COLON);
  if (l->curr_token.type != NEWLINE) {
    error("Extra garbage after the label, couldn't parse it.\n");
  }
  return add_node(make_node(NT_LABEL, left, NULL_INDEX, NO_NODE_DATA));
}

static NodeIndex instruction(Lexer *l) {
  // we're already pointed at the instruction variant.
  Lexeme instruction = l->curr_token.type;
  // an instruction an a single, optional argument.
  NodeIndex left = NULL_INDEX;

  // eat through the instruction at the cursor, then try to parse the
  // argument.
  eat(l, l->curr_token.type);

  left =
      argument(l); // parse the argument anyway, since it'll parse out an
                   // "implicit" argument node if none is found. this makes the
                   // "instruction" AST node have a more uniform structure, with
                   // all the argument left nodes always being valid.

  // only pass the instruction, then any optional arguments as child nodes.
  return add_node(make_node(NT_INSTRUCTION, left, NULL_INDEX,
                            (NodeData){.as_raw_data = instruction}));
}

// OR over a bunch of potential statement types.
static NodeIndex statement(Lexer *l) {
  Lexeme cl = l->curr_token.type;

  printf("Parsing new statement. Starting with token %s.\n",
         lexeme_to_string(cl));

  // while (cl == NEWLINE) {
  //   eat(l, NEWLINE);
  // }
  //
  // cl = l->curr_token.type;

  if (cl == DOT) {
    printf("Choosing pragma branch in statement parser\n");
    return pragma(l);
  } else if (cl == ID) { // an ID in the first slot, like a variable or
                         // function name.
    printf("Choosing label branch in statement parser\n");
    return label(l);
  } else if (is_instruction(cl)) {
    printf("Choosing instruction branch in statement parser\n");
    return instruction(l);
  } else if (cl == NEWLINE || cl == EMPTY) {
    printf("Choosing empty statement branch in statement parser: found %s\n",
           lexeme_to_string(cl));
    return empty(l);
  } else {
    // print the lexeme_to_string ptr, not the cl value as a pointer x_x
    error("Invalid statement starting token found: %s", lexeme_to_string(cl));
  }
}

// either an assembler pragma, an instruction or a label.
static NodeIndex statement_list(Lexer *l) {
  NodeIndex left = statement(l);
  NodeIndex second_term = NULL_INDEX;

  Lexeme cl = l->curr_token.type;

  printf("after parsing statement, found lexeme: %s\n", lexeme_to_string(cl));

  if (cl == NEWLINE) { // if there's another statement found, parse it and
                       // add it onto the tree.
    printf("Found another newline in the statement list, parsing another "
           "statement...\n");
    eat(l, NEWLINE); // move past the NEWLINE, positioning at the start of
                     // the new next statement.
    second_term = statement_list(l);
  }

  return add_node(
      make_node(NT_STATEMENT_LIST, left, second_term, NO_NODE_DATA));
}

// parse is the only non-static API method of the parser. it parses the
// whole program into the global AST.
//
// will return the index of the root node into the
// global ast Node array.
NodeIndex parse(char text_input[INPUT_LEN]) {
  Lexer *l = (Lexer *)malloc(
      sizeof(Lexer)); // pass through the lexer manually and malloc that. TODO:
                      // is there a better way to place the lexer in memory?

  // use the actual string length and not the buffer length to do the EOF
  // check.
  l->text_len = strlen(text_input);
  l->pos = -1; // set to -1, the init next() call will put it at 0.
  l->curr_token.type = EMPTY;

  memcpy(l->text, text_input, INPUT_LEN);
  next(l);

  // everything in C is just a list of top-level declarations.
  return statement_list(l);
}

void test_parse() {}
