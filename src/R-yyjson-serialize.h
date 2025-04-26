
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Serialization options
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define DATAFRAME_BY_COL 1
#define DATAFRAME_BY_ROW 2

#define FACTOR_AS_INT 1
#define FACTOR_AS_STR 2

#define STR_SPECIALS_AS_NULL    0
#define STR_SPECIALS_AS_STRING  1

#define NUM_SPECIALS_AS_NULL   0
#define NUM_SPECIALS_AS_STRING 1

#define NAME_REPAIR_NONE    0
#define NAME_REPAIR_MINIMAL 1



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
#define INTSXP_FACTOR   32
#define INTSXP_DATE     33
#define INTSXP_POSIXCT  34
#define REALSXP_DATE    35
#define REALSXP_POSIXCT 36
#define REALSXP_INT64   37
#define VECSXP_DF       38  // A list column which actually has type = data.frame




//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Serialize options struct
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct {
  unsigned int data_frame;
  unsigned int factor;
  unsigned int null;
  int digits;
  int digits_secs;
  int digits_signif;
  bool auto_unbox; 
  unsigned int name_repair;
  unsigned int str_specials;
  unsigned int num_specials;
  unsigned int yyjson_write_flag;
  bool fast_numerics;
  bool json_verbatim;
} serialize_options;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Exported funcs needed for ndjson writing
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
serialize_options parse_serialize_options(SEXP serialize_opts_);
yyjson_mut_val *serialize_core(SEXP robj_, yyjson_mut_doc *doc, serialize_options *opt);
yyjson_mut_val *scalar_logical_to_json_val(int32_t rlgl, yyjson_mut_doc *doc, serialize_options *opt);
yyjson_mut_val *scalar_integer_to_json_val(int32_t rint, yyjson_mut_doc *doc, serialize_options *opt);
yyjson_mut_val *scalar_integer64_to_json_val(SEXP vec_, R_xlen_t idx, yyjson_mut_doc *doc, serialize_options *opt);
yyjson_mut_val *scalar_date_to_json_val(SEXP vec_, R_xlen_t idx, yyjson_mut_doc *doc, serialize_options *opt);
yyjson_mut_val *scalar_posixct_to_json_val(SEXP vec_, R_xlen_t idx, yyjson_mut_doc *doc, serialize_options *opt);
yyjson_mut_val *scalar_rawsxp_to_json_val(SEXP vec_, R_xlen_t idx, yyjson_mut_doc *doc, serialize_options *opt);
yyjson_mut_val *scalar_factor_to_json_val(SEXP factor_, R_xlen_t idx,  yyjson_mut_doc *doc, serialize_options *opt);
yyjson_mut_val *scalar_double_to_json_val(double rdbl, yyjson_mut_doc *doc, serialize_options *opt);
yyjson_mut_val *scalar_strsxp_to_json_val(SEXP str_, R_xlen_t idx, yyjson_mut_doc *doc, serialize_options *opt);

yyjson_mut_val *data_frame_row_to_json_object(SEXP df_, unsigned int *col_type, unsigned int row, int skip_col, yyjson_mut_doc *doc, serialize_options *opt);
unsigned int *detect_data_frame_types(SEXP df_, serialize_options *opt);
