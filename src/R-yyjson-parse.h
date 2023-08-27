


//===========================================================================
// Container types
//===========================================================================
#define CTN_NONE 1 << 0
#define CTN_OBJ  1 << 1
#define CTN_ARR  1 << 2


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// YYJSON Value types.
// Need:
//   - A set of types to shadow the YYJSON types
//   - Not overlap so they can be used in a bitset to represent all types 
//     seen within a container
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define VAL_NONE    1 << 0
#define VAL_RAW     1 << 1
#define VAL_NULL    1 << 2
#define VAL_BOOL    1 << 3
#define VAL_INT     1 << 4
#define VAL_REAL    1 << 5
#define VAL_STR     1 << 6
#define VAL_STR_INT 1 << 7 // Integer promoted to string
#define VAL_ARR     1 << 8
#define VAL_OBJ     1 << 9
#define VAL_INT64   1 << 10

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Give numeric values to a few flags.
// These are specific numeric flags, as I think they could be expanded to 
// 3 or more options in the future.
// E.g. Maybe want to add "INT64_AS_DBL" or "STR_SPECIALS_AS_NULL" 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define INT64_AS_STR   1 << 0
#define INT64_AS_BIT64 1 << 1

#define STR_SPECIALS_AS_SPECIAL 0
#define STR_SPECIALS_AS_STRING  1

#define NUM_SPECIALS_AS_SPECIAL 0
#define NUM_SPECIALS_AS_STRING  1

#define MISSING_AS_NULL 0
#define MISSING_AS_NA   1

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Existing SEXPs
//   0	NILSXP	NULL
//   1	SYMSXP	symbols
//   2	LISTSXP	pairlists
//   3	CLOSXP	closures
//   4	ENVSXP	environments
//   5	PROMSXP	promises
//   6	LANGSXP	language objects
//   7	SPECIALSXP	special functions
//   8	BUILTINSXP	builtin functions
//   9	CHARSXP	internal character strings
//  10	LGLSXP	logical vectors
//  13	INTSXP	integer vectors
//  14	REALSXP	numeric vectors
//  15	CPLXSXP	complex vectors
//  16	STRSXP	character vectors
//  17	DOTSXP	dot-dot-dot object
//  18	ANYSXP	make “any” args work
//  19	VECSXP	list (generic vector)
//  20	EXPRSXP	expression vector
//  21	BCODESXP	byte code
//  22	EXTPTRSXP	external pointer
//  23	WEAKREFSXP	weak reference
//  24	RAWSXP	raw vector
//  25	S4SXP	S4 classes not of simple type
//
// Extra types to handle special types
//
//  INT64SXP -
//     Can decide what to allocate by testing for (INT64SXP & REALSXP)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define INT64SXP (REALSXP & 1 << 16)


//===========================================================================
// max columns currently allocated for parsing data.frames
// TODO: Instead of static value, switch to dynamic allocation and grow
//       as-needed.  Did not want to think about this at the moment, so 
//       just set a high-ish value.  Code throws an error if this is 
//       exceeded during a parse.
//===========================================================================
#define MAX_DF_COLS 128

//===========================================================================
// Struct of parse options
//===========================================================================
typedef struct {
  unsigned int int64;
  unsigned int missing_list_elem;
  bool vectors_to_df;
  unsigned int str_specials;
  unsigned int num_specials;
  unsigned int yyjson_read_flag;
} parse_options;

//===========================================================================
// Number of context characters to print when an error occurs
//===========================================================================
#define ERR_CONTEXT 20

parse_options create_parse_options(SEXP parse_opts_);
SEXP parse_json_from_str(const char *str, parse_options *opt);


unsigned int update_type_bitset(unsigned int type_bitset, yyjson_val *val, parse_options *opt);
unsigned int get_best_sexp_to_represent_type_bitset(unsigned int type_bitset, parse_options *opt);
void dump_type_bitset(unsigned int type_bitset);

int32_t json_val_to_logical(yyjson_val *val, parse_options *opt);
int32_t json_val_to_integer(yyjson_val *val, parse_options *opt);
double json_val_to_double(yyjson_val *val, parse_options *opt);
long long json_val_to_integer64(yyjson_val *val, parse_options *opt);
SEXP json_val_to_charsxp(yyjson_val *val, parse_options *opt);
SEXP json_as_robj(yyjson_val *val, parse_options *opt);

//===========================================================================
// Error reporting
//===========================================================================
void output_verbose_error(const char *str, yyjson_read_err err);
