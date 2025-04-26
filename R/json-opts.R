
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Advanced: Values for setting internal options directly on YYJSON library
#' 
#' This is a list of integer values used for setting flags on the \code{yyjson} 
#' code directly.  This is an ADVANCED option and should be used with caution.
#' 
#' Some of these settings overlap and conflict with code needed to handle
#' the translation of JSON values to R.
#'
#" Pass multiple options with 
#' \code{opts_read_json(yyjson_read_flag = c(yyjson_read_flag$x, yyjson_read_flag$y, ...))}
#' 
#' \describe{
#' \item{YYJSON_READ_NOFLAG}{
#' Default option (RFC 8259 compliant):
#' \itemize{
#'   \item{Read positive integer as uint64_t.}
#'   \item{Read negative integer as int64_t.}
#'   \item{Read floating-point number as double with round-to-nearest mode.}
#'   \item{Read integer which cannot fit in uint64_t or int64_t as double.}
#'   \item{Report error if double number is infinity.}
#'   \item{Report error if string contains invalid UTF-8 character or BOM.}
#'   \item{Report error on trailing commas, comments, inf and nan literals.}
#' }   
#' }
#' 
#' \item{YYJSON_READ_INSITU}{
#'   Read the input data in-situ.
#'   This option allows the reader to modify and use input data to store string
#'   values, which can increase reading speed slightly.
#'   The caller should hold the input data before free the document.
#'   The input data must be padded by at least \code{YYJSON_PADDING_SIZE} bytes.
#'   For example: \code{"[1,2]"} should be \code{"[1,2]\0\0\0\0"}, input length should be 5.
#' }
#' 
#' \item{YYJSON_READ_STOP_WHEN_DONE}{
#'   Stop when done instead of issuing an error if there's additional content
#'   after a JSON document. This option may be used to parse small pieces of JSON
#'   in larger data, such as "NDJSON"
#' }
#'
#' \item{YYJSON_READ_ALLOW_TRAILING_COMMAS}{
#' Allow single trailing comma at the end of an object or array,
#' such as \code{"[1,2,3,]"}
#' }
#' 
#' \item{YYJSON_READ_ALLOW_COMMENTS}{
#' Allow C-style single line and multiple line comments (non-standard).
#' }
#' 
#' \item{YYJSON_READ_ALLOW_INF_AND_NAN}{
#' Allow inf/nan number and literal, case-insensitive,
#' such as 1e999, NaN, inf, -Infinity (non-standard). 
#' }
#'
#' \item{YYJSON_READ_NUMBER_AS_RAW}{
#' Read all numbers as raw strings (value with "YYJSON_TYPE_RAW" type),
#' inf/nan literal is also read as raw with "ALLOW_INF_AND_NAN" flag.
#' }
#' 
#' \item{YYJSON_READ_ALLOW_INVALID_UNICODE}{
#' Allow reading invalid unicode when parsing string values (non-standard).
#' Invalid characters will be allowed to appear in the string values, but
#' invalid escape sequences will still be reported as errors.
#' This flag does not affect the performance of correctly encoded strings.
#' WARNING: Strings in JSON values may contain incorrect encoding when this
#' option is used, you need to handle these strings carefully to avoid security
#' risks.
#' }
#'
#' \item{YYJSON_READ_BIGNUM_AS_RAW}{
#' Read big numbers as raw strings. These big numbers include integers that
#' cannot be represented by "int64_t" and "uint64_t", and floating-point
#' numbers that cannot be represented by finite "double".
#' The flag will be overridden by "YYJSON_READ_NUMBER_AS_RAW" flag.
#' }
#' }
#' 
#' @export
#'
#' @examples
#' read_json_str(
#'    '[12.3]', 
#'    opts = opts_read_json(yyjson_read_flag = yyjson_read_flag$YYJSON_READ_ALLOW_TRAILING_COMMAS)
#'  )
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
yyjson_read_flag <- list(
  YYJSON_READ_NOFLAG                =   0L,
  YYJSON_READ_INSITU                =   1L,
  YYJSON_READ_STOP_WHEN_DONE        =   2L,
  YYJSON_READ_ALLOW_TRAILING_COMMAS =   4L,
  YYJSON_READ_ALLOW_COMMENTS        =   8L, 
  YYJSON_READ_ALLOW_INF_AND_NAN     =  16L, 
  YYJSON_READ_NUMBER_AS_RAW         =  32L, 
  YYJSON_READ_ALLOW_INVALID_UNICODE =  64L, 
  YYJSON_READ_BIGNUM_AS_RAW         = 128L
)


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Advanced: Values for setting internal options directly on YYJSON library
#' 
#' This is a list of integer values used for setting flags on the \code{yyjson} 
#' code directly.  This is an ADVANCED option and should be used with caution.
#' 
#' Some of these settings overlap and conflict with code needed to handle
#' the translation of JSON values to R.
#'
#" Pass multiple options with 
#' \code{opts_write_json(yyjson_write_flag = c(write_flag$x, write_flag$y, ...))}
#' 
#' \describe{
#' \item{YYJSON_WRITE_NOFLAG}{
#' Default value.
#' \itemize{
#'   \item{Write JSON minify.}
#'   \item{Report error on inf or nan number.}
#'   \item{Report error on invalid UTF-8 string.}
#'   \item{Do not escape unicode or slash.}
#' }
#' }
#'\item{YYJSON_WRITE_PRETTY}{Write JSON pretty with 4 space indent.}
#'\item{YYJSON_WRITE_ESCAPE_UNICODE}{Escape unicode as `uXXXX`, make the 
#'output ASCII only.}
#'\item{YYJSON_WRITE_ESCAPE_SLASHES}{Escape '/' as '\/'.}
#'\item{YYJSON_WRITE_ALLOW_INF_AND_NAN}{Write inf and nan number as 'Infinity' 
#'and 'NaN' literal (non-standard).}
#'\item{YYJSON_WRITE_INF_AND_NAN_AS_NULL}{Write inf and nan number as null literal.
#' This flag will override `YYJSON_WRITE_ALLOW_INF_AND_NAN` flag.}
#' \item{YYJSON_WRITE_ALLOW_INVALID_UNICODE}{Allow invalid unicode when encoding 
#' string values (non-standard).
#' Invalid characters in string value will be copied byte by byte.
#' If `YYJSON_WRITE_ESCAPE_UNICODE` flag is also set, invalid character will be
#' escaped as `U+FFFD` (replacement character).
#' This flag does not affect the performance of correctly encoded strings.}
#' \item{YYJSON_WRITE_PRETTY_TWO_SPACES}{Write JSON pretty with 2 space indent.
#' This flag will override `YYJSON_WRITE_PRETTY` flag.}
#' \item{YYJSON_WRITE_NEWLINE_AT_END}{Adds a newline character
#' at the end of the JSON. This can be helpful for text editors or NDJSON}
#' }
#' 
#' @export
#' 
#' @examples
#' write_json_str("hello/there", opts = opts_write_json(
#'   yyjson_write_flag = yyjson_write_flag$YYJSON_WRITE_ESCAPE_SLASHES
#' ))
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
yyjson_write_flag <- list(
  YYJSON_WRITE_NOFLAG                  =   0L,
  YYJSON_WRITE_PRETTY                  =   1L,
  YYJSON_WRITE_ESCAPE_UNICODE          =   2L,
  YYJSON_WRITE_ESCAPE_SLASHES          =   4L,
  YYJSON_WRITE_ALLOW_INF_AND_NAN       =   8L,
  YYJSON_WRITE_INF_AND_NAN_AS_NULL     =  16L,
  YYJSON_WRITE_ALLOW_INVALID_UNICODE   =  32L,
  YYJSON_WRITE_PRETTY_TWO_SPACES       =  64L,
  YYJSON_WRITE_NEWLINE_AT_END          = 128L
)


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Create named list of options for parsing R from JSON
#' 
#' @param int64 how to encode large integers which do not fit into R's integer
#'        type.  'string' imports them as a character vector. 'double' will
#'        convert the integer to a double precision numeric value. 'bit64' will
#'        use the 'integer64' type from the 'bit64' package.  Note that the
#'        'integer64' type is a \emph{signed} integer type, and a warning will
#'        be issued if JSON contains an \emph{unsigned} integer which cannot
#'        be stored in this type.
#' @param df_missing_list_elem R value to use when elements are missing in list 
#'        columns in data.frames. Default: NULL
#' @param obj_of_arrs_to_df logical. Should a named list of equal-length
#'        vectors be promoted to a data.frame?  Default: TRUE.  If FALSE, then
#'        result will be left as a list.
#' @param arr_of_objs_to_df logical. Should an array or objects be promoted to a 
#'        a data.frame? Default: TRUE. If FALSE, then results will be read as a
#'        list-of-lists.
#' @param yyjson_read_flag integer vector of internal \code{yyjson}
#'        options.  See \code{yyjson_read_flag} in this package, and read
#'        the yyjson API documentation for more information.  This is considered
#'        an advanced option.
#' @param str_specials Should \code{'NA'} in a JSON string be converted to the \code{'special'}
#'        \code{NA} value in R, or left as a \code{'string'}.  Default: 'string'
#' @param num_specials Should JSON strings 'NA'/'Inf'/'NaN' in a numeric context 
#'        be converted to the \code{'special'} R numeric values
#'        \code{NA, Inf, NaN}, or left as a \code{'string'}. Default: 'special'
#' @param promote_num_to_string Should numeric values be promoted to strings 
#'        when they occur within an array with other string values?  Default: FALSE 
#'        means to keep numerics as numeric value and promote the \emph{container} to
#'        be a \code{list} rather than an atomic vector when types are mixed.  If \code{TRUE}
#'        then array of mixed string/numeric types will be promoted to all 
#'        string values and returned as an atomic character vector.  Set this to \code{TRUE}
#'        if you want to emulate the behaviour of \code{jsonlite::fromJSON()}
#' @param length1_array_asis logical. Should JSON arrays with length = 1 be 
#'        marked with class \code{AsIs}.  Default: FALSE
#'
#' @seealso [yyjson_read_flag()]
#' @return Named list of options for reading JSON
#' @export
#' 
#' @examples
#' opts_read_json()
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
opts_read_json <- function(
    promote_num_to_string = FALSE,
    df_missing_list_elem  = NULL,
    obj_of_arrs_to_df     = TRUE,
    arr_of_objs_to_df     = TRUE,
    str_specials          = c('string', 'special'),
    num_specials          = c('special', 'string'),
    int64                 = c('string', 'double', 'bit64'),
    length1_array_asis    = FALSE,
    yyjson_read_flag      = 0L
) {
  
  structure(
    list(
      promote_num_to_string = isTRUE(promote_num_to_string),
      df_missing_list_elem  = df_missing_list_elem,
      obj_of_arrs_to_df     = isTRUE(obj_of_arrs_to_df),
      arr_of_objs_to_df     = isTRUE(arr_of_objs_to_df),
      length1_array_asis    = isTRUE(length1_array_asis),
      str_specials          = match.arg(str_specials),
      num_specials          = match.arg(num_specials),
      int64                 = match.arg(int64),
      yyjson_read_flag      = as.integer(yyjson_read_flag)
    ),
    class = "opts_read_json"
  )
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Create named list of options for serializing R to JSON
#'
#' @param digits decimal places to keep for floating point numbers. Default: -1.
#'        Positive values specify number of decimal places. Using zero will
#'        write the numeric value as an integer. Values less than zero mean that
#'        the floating point value should be written as-is (the default).
#'        This argument is ignored if \code{digits_signif} is greater than zero.
#' @param digits_secs decimal places for fractional seconds when converting
#'        times to a string representation. Default: 0.  Valid range: 0 to 6
#' @param digits_signif significant decimal places to store in floating point numbers.
#'        Default: -1 means to output the number as-is (while respecting the 
#'        \code{digits}) argument.  Values above 0 will produce rounding to
#'        the given number of places and the \code{digits} argument will be ignored.
#' @param dataframe how to encode data.frame objects. Options 'rows' or 
#'        columns'.  Default: 'rows'
#' @param factor how to encode factor objects: must be one of 'string' or 'integer'
#'        Default: 'string'
#' @param auto_unbox automatically unbox all atomic vectors of length 1 such that
#'        they appear as atomic elements in JSON rather than arrays of length 1.
#' @param pretty Logical value indicating if the created JSON string should have
#'        whitespace for indentation and linebreaks. Default: FALSE.
#'        Note: this option is equivalent to \code{yyjson_write_flag = write_flag$YYJSON_WRITE_PRETTY}
#' @param name_repair How should unnamed items in a partially named list be handled?
#'        'none' means to leave their names blank in JSON (which may not be valid JSON).
#'        'minimal' means to use the integer position index of the item as its name if 
#'        it is missing.  Default: 'none'
#' @param str_specials Should a special value of \code{NA} in a character vector
#'        be converted to a 
#'        JSON \code{null} value, or converted to a string "NA"?  Default: 'null'
#' @param num_specials Should special numeric values (i.e. NA, NaN, Inf) be
#'        converted to a JSON \code{null} value or converted to a string 
#'        representation e.g. "NA"/"NaN" etc.   Default: 'null'
#' @param fast_numerics Does the user guarantee that there are no NA, NaN or Inf
#'        values in the numeric vectors?  Default: FALSE.  If \code{TRUE} then
#'        numeric and integer vectors will be written to JSON using a faster method.
#'        Note: if there are NA, NaN or Inf values, an error will be thrown.
#'        Expert users are invited to also consider the
#'        \code{YYJSON_WRITE_ALLOW_INF_AND_NAN} and 
#'        \code{YYJSON_WRITE_INF_AND_NAN_AS_NULL} options for \code{yyjson_write_flags}
#'        and should consult the \code{yyjson} API documentation for 
#'        further details.
#' @param yyjson_write_flag integer vector corresponding to internal \code{yyjson}
#'        options.  See \code{yyjson_write_flag} in this package, and read
#'        the yyjson API documentation for more information.  This is considered
#'        an advanced option.
#' 
#' @seealso [yyjson_write_flag()]
#' @return Named list of options for writing JSON
#' @export
#' 
#' @examples
#' write_json_str(head(iris, 3), opts = opts_write_json(factor = 'integer'))
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
opts_write_json <- function( 
    digits            = -1L,
    digits_secs       =  0L,
    digits_signif     = -1L,
    pretty            = FALSE,
    auto_unbox        = FALSE,
    dataframe         = c("rows", "columns"),
    factor            = c("string", "integer"),
    name_repair       = c('none', 'minimal'),
    num_specials      = c('null', 'string'),
    str_specials      = c('null', 'string'),
    fast_numerics     = FALSE,
    yyjson_write_flag = 0L) {
  
  structure(
    list(
      digits            = as.integer(digits),
      digits_secs       = as.integer(digits_secs),
      digits_signif     = as.integer(digits_signif),
      dataframe         = match.arg(dataframe),
      factor            = match.arg(factor),
      auto_unbox        = isTRUE(auto_unbox),
      pretty            = isTRUE(pretty),
      name_repair       = match.arg(name_repair),
      str_specials      = match.arg(str_specials),
      num_specials      = match.arg(num_specials),
      fast_numerics     = isTRUE(fast_numerics),
      yyjson_write_flag = as.integer(yyjson_write_flag)
    ),
    class = "opts_write_json"
  )
}
