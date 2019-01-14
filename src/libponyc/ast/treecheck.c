#include "treecheck.h"
#include "ast.h"
#include "../pass/pass.h"
#include "ponyassert.h"
#include <stdbool.h>
#include <stdio.h>


// This define causes the tree checker to assert immediately on error, rather
// than waiting until it has checked the entire tree. This means that only one
// error can be spotted in each run of the program, but can make errors far
// easier to track down.
// Use this when debugging, but leave disabled when committing.
#define IMMEDIATE_FAIL // TODO: comment out


typedef enum
{
  CHK_OK,        // Tree found and check
  CHK_NOT_FOUND, // Possible AST node ids not found
  CHK_ERROR      // Error has occurred
} check_res_t;

typedef check_res_t (*check_fn_t)(ast_t* ast, errors_t* errors,
  size_t print_width);

typedef enum
{
  SCOPE_NO,
  SCOPE_YES,
  SCOPE_MAYBE
} scope_kind_t;

typedef struct check_state_t
{
  scope_kind_t scope;
  bool has_data;
  check_fn_t type;
  ast_t* child;
  size_t child_index;
  errors_t* errors;
} check_state_t;


// Print an error preamble for the given node.
static void error_preamble(ast_t* ast)
{
  pony_assert(ast != NULL);

  fprintf(stderr, "Internal error: AST node %d (%s), ", ast_id(ast),
    ast_get_print(ast));
}


// Check whether the specified id is in the given list.
// The id list must be TK_EOF terminated.
static bool is_id_in_list(token_id id, const token_id* list)
{
  pony_assert(list != NULL);

  for(size_t i = 0; list[i] != TK_EOF; i++)
    if(list[i] == id)
      return true;

  return false;
}


// Check the given node if it's covered by the specified rule list.
// The rules list must be NULL terminated.
static check_res_t check_from_list(ast_t* ast, const check_fn_t *rules,
  errors_t* errors, size_t print_width)
{
  pony_assert(ast != NULL);
  pony_assert(rules != NULL);

  // Check rules in given list
  for(const check_fn_t* p = rules; *p != NULL; p++)
  {
    check_res_t r = (*p)(ast, errors, print_width);

    if(r != CHK_NOT_FOUND)
      return r;
  }

  // Child does not match any rule in given list
  return CHK_NOT_FOUND;
}


// Check some number of children, all using the same rules.
// The given max and min counts are inclusive. Pass -1 for no maximum limit.
static bool check_children(ast_t* ast, check_state_t* state,
  const check_fn_t *rules, size_t min_count, size_t max_count, errors_t* errors,
  size_t print_width)
{
  pony_assert(ast != NULL);
  pony_assert(state != NULL);
  pony_assert(min_count <= max_count);

  size_t found_count = 0;

  while(found_count < max_count && state->child != NULL)
  {
    // See if next child is suitable
    check_res_t r = check_from_list(state->child, rules, errors, print_width);

    if(r == CHK_ERROR)  // Propagate error
      return false;

    if(r == CHK_NOT_FOUND)  // End of list
      break;

    // Child found
    state->child = ast_sibling(state->child);
    state->child_index++;
    found_count++;
  }

  // There are no more matching children
  if(found_count >= min_count)
    return true;

  // Didn't find enough matching children
  if(state->child == NULL)
  {
    error_preamble(ast);
    fprintf(stderr, "found " __zu " child%s, expected more\n", state->child_index,
      (state->child_index == 1) ? "" : "ren");
    ast_error(state->errors, ast, "Here");
    ast_fprint(stderr, ast, print_width);
#ifdef IMMEDIATE_FAIL
    pony_assert(false);
#endif
  }
  else
  {
    error_preamble(ast);
    fprintf(stderr, "child " __zu " has invalid id %d\n", state->child_index,
      ast_id(state->child));
    ast_error(state->errors, ast, "Here");
    ast_fprint(stderr, ast, print_width);
#ifdef IMMEDIATE_FAIL
    pony_assert(false);
#endif
  }

  return false;
}


// Check the extra information. This includes:
// * legal data pointer
// * legal scope symbol table
// * no extra children
static check_res_t check_extras(ast_t* ast, check_state_t* state,
  errors_t* errors, size_t print_width)
{
  pony_assert(ast != NULL);
  pony_assert(state != NULL);


  ast_t* type_field = ast_type(ast);

  if(type_field != NULL)
  {
    if(state->type == NULL)
    {
      error_preamble(ast);
      fprintf(stderr, "unexpected type\n");
      ast_error(state->errors, ast, "Here");
      ast_fprint(stderr, ast, print_width);
#ifdef IMMEDIATE_FAIL
      pony_assert(false);
#endif
      return CHK_ERROR;
    }

    check_res_t r = state->type(type_field, errors, print_width);

    if(r == CHK_ERROR)  // Propagate error
      return CHK_ERROR;

    if(r == CHK_NOT_FOUND)
    {
      error_preamble(ast);
      fprintf(stderr, "type field has invalid id %d\n", ast_id(type_field));
      ast_error(state->errors, ast, "Here");
      ast_fprint(stderr, ast, print_width);
#ifdef IMMEDIATE_FAIL
      pony_assert(false);
#endif
      return CHK_ERROR;
    }
  }

  if(state->child != NULL)
  {
    error_preamble(ast);
    fprintf(stderr, "child " __zu " (id %d, %s) unexpected\n", state->child_index,
      ast_id(state->child), ast_get_print(state->child));
    ast_error(state->errors, ast, "Here");
    ast_fprint(stderr, ast, print_width);
#ifdef IMMEDIATE_FAIL
    pony_assert(false);
#endif
    return CHK_ERROR;
  }

  if(ast_data(ast) != NULL && !state->has_data)
  {
    error_preamble(ast);
    fprintf(stderr, "unexpected data %p\n", ast_data(ast));
    ast_error(state->errors, ast, "Here");
    ast_fprint(stderr, ast, print_width);
#ifdef IMMEDIATE_FAIL
    pony_assert(false);
#endif
    return CHK_ERROR;
  }

  if(ast_has_scope(ast) && (state->scope == SCOPE_NO))
  {
    error_preamble(ast);
    fprintf(stderr, "unexpected scope\n");
    ast_error(state->errors, ast, "Here");
    ast_fprint(stderr, ast, print_width);
#ifdef IMMEDIATE_FAIL
    pony_assert(false);
#endif
    return CHK_ERROR;
  }

  if(!ast_has_scope(ast) && (state->scope == SCOPE_YES))
  {
    error_preamble(ast);
    fprintf(stderr, "expected scope not found\n");
    ast_error(state->errors, ast, "Here");
    ast_fprint(stderr, ast, print_width);
#ifdef IMMEDIATE_FAIL
    pony_assert(false);
#endif
    return CHK_ERROR;
  }

  return CHK_OK;
}


// Defines for first pass, forward declare group and rule functions
#define TREE_CHECK
#define ROOT(...)

#define RULE(name, def, ...) \
  static check_res_t name(ast_t* ast, errors_t* errs, size_t width)

#define GROUP(name, ...) \
  static check_res_t name(ast_t* ast, errors_t* errs, size_t width)

#define LEAF

#include "treecheckdef.h"

#undef ROOT
#undef RULE
#undef GROUP


// Defines for second pass, group and rule functions

#define ROOT(...) \
  check_res_t check_root(ast_t* ast, errors_t* errors, size_t print_width) \
  { \
    const check_fn_t rules[] = { __VA_ARGS__, NULL }; \
    return check_from_list(ast, rules, errors, print_width); \
  }

#define RULE(name, def, ...) \
  static check_res_t name(ast_t* ast, errors_t* errors, size_t print_width) \
  { \
    const token_id ids[] = { __VA_ARGS__, TK_EOF }; \
    if(!is_id_in_list(ast_id(ast), ids)) return CHK_NOT_FOUND; \
    check_state_t state = {SCOPE_NO, false, NULL, NULL, 0, errors}; \
    state.child = ast_child(ast); \
    def \
    return check_extras(ast, &state, errors, print_width); \
  }

#define GROUP(name, ...) \
  static check_res_t name(ast_t* ast, errors_t* errors, size_t print_width) \
  { \
    const check_fn_t rules[] = { __VA_ARGS__, NULL }; \
    return check_from_list(ast, rules, errors, print_width); \
  }

#define IS_SCOPE state.scope = SCOPE_YES;
#define MAYBE_SCOPE state.scope = SCOPE_MAYBE;
#define HAS_DATA state.has_data = true;
#define HAS_TYPE(type_fn) state.type = type_fn;

#define CHILDREN(min, max, ...) \
  { \
    const check_fn_t rules[] = { __VA_ARGS__, NULL }; \
    if(!check_children(ast, &state, rules, min, max, errors, print_width)) \
      return CHK_ERROR; \
  }

#undef OPTIONAL // Annoyingly defined in various Windows headers

#define CHILD(...)        CHILDREN(1, 1, __VA_ARGS__)
#define OPTIONAL(...)     CHILDREN(0, 1, __VA_ARGS__)
#define ZERO_OR_MORE(...) CHILDREN(0, -1, __VA_ARGS__)
#define ONE_OR_MORE(...)  CHILDREN(1, -1, __VA_ARGS__)

#define IF(cond, body) \
  if(cond) \
  { \
    body \
  }

#define CHILD_ID(index) \
  ast_id(ast_childidx(ast, index))

#include "treecheckdef.h"


void check_tree(ast_t* tree, pass_opt_t* opt)
{
#ifdef PONY_NDEBUG
  // Keep compiler happy in release builds.
  (void)tree;
  (void)opt;
#else
  // Only check tree in debug builds.
  pony_assert(tree != NULL);
  check_res_t r = check_root(tree, opt->check.errors, opt->ast_print_width);
  pony_assert(r != CHK_ERROR);

  // Ignore CHK_NOT_FOUND, that means we weren't given a whole tree.
#endif
}
