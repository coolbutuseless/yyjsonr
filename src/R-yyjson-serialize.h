
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
// Serialize options struct
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct {
  unsigned int data_frame;
  unsigned int factor;
  unsigned int null;
  bool auto_unbox; 
  unsigned int name_repair;
  unsigned int str_specials;
  unsigned int num_specials;
  unsigned int yyjson_write_flag;
} serialize_options;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Exported funcs needed for ndjson writing
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
serialize_options parse_serialize_options(SEXP serialize_opts_);
yyjson_mut_val *serialize_core(SEXP robj_, yyjson_mut_doc *doc, serialize_options *opt);
yyjson_mut_val *scalar_logical_to_json_val(int32_t rlgl, yyjson_mut_doc *doc, serialize_options *opt);
yyjson_mut_val *scalar_integer_to_json_val(int32_t rint, yyjson_mut_doc *doc, serialize_options *opt);
yyjson_mut_val *scalar_integer64_to_json_val(SEXP vec_, unsigned int idx, yyjson_mut_doc *doc, serialize_options *opt);
yyjson_mut_val *scalar_date_to_json_val(SEXP vec_, unsigned int idx, yyjson_mut_doc *doc, serialize_options *opt);
yyjson_mut_val *scalar_posixct_to_json_val(SEXP vec_, unsigned int idx, yyjson_mut_doc *doc, serialize_options *opt);
yyjson_mut_val *scalar_rawsxp_to_json_val(SEXP vec_, unsigned int idx, yyjson_mut_doc *doc, serialize_options *opt);
yyjson_mut_val *scalar_factor_to_json_val(SEXP factor_, unsigned int idx,  yyjson_mut_doc *doc, serialize_options *opt);
yyjson_mut_val *scalar_double_to_json_val(double rdbl, yyjson_mut_doc *doc, serialize_options *opt);
yyjson_mut_val *scalar_strsxp_to_json_val(SEXP str_, unsigned int idx, yyjson_mut_doc *doc, serialize_options *opt);
