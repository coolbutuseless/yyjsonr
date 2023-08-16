
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
#' \code{from_opts(yyjson_read_flag = c(read_flag$x, read_flag$y, ...))}
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
#' such as \code{"[1,2,3,]"},  "{"a":1,"b":2,}" (non-standard).
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
#' @examples
#' \dontrun{
#' from_json_str(str, opts = from_opts(yyjson_read_flag = read_flag$YYJSON_READ_NUMBER_AS_RAW))
#' }
#' 
#'
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
read_flag <- list(
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
#' \code{to_opts(yyjson_write_flag = c(write_flag$x, write_flag$y, ...))}
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
#' }
#' 
#'
#' @examples
#' \dontrun{
#' to_json_str(str, opts = to_opts(yyjson_write_flag = write_flag$YYJSON_WRITE_ESCAPE_SLASHES))
#' }
#' 
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
write_flag <- list(
  YYJSON_WRITE_NOFLAG                  =  0L,
  YYJSON_WRITE_PRETTY                  =  1L,
  YYJSON_WRITE_ESCAPE_UNICODE          =  2L,
  YYJSON_WRITE_ESCAPE_SLASHES          =  4L,
  YYJSON_WRITE_ALLOW_INF_AND_NAN       =  8L,
  YYJSON_WRITE_INF_AND_NAN_AS_NULL     = 16L,
  YYJSON_WRITE_ALLOW_INVALID_UNICODE   = 32L,
  YYJSON_WRITE_PRETTY_TWO_SPACES       = 64L
)


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Create named list of options for parsing R from JSON
#' 
#' @param int64 how to encode large integers which do not fit into R's integer
#'        type.  'string' imports them as a character vector.  'bit64' will
#'        use the 'integer64' type from the 'bit64' package.  Note that the
#'        'integer64' type is a \emph{signed} integer type, and a warning will
#'        be issued if JSON contains an \emph{unsigned} integer which cannot
#'        be stored in this type.
#' @param missing_list_elem how to handle missing elements in list columns in 
#'        data.frames. Options, 'na', or 'null.  Default: 'null'
#' @param vectors_to_df logical. Should a named list of equal-length
#'        vectors be promoted to a data.frame?  Default: TRUE.  If FALSE, then
#'        result will be left as a list.
#' @param yyjson_read_flag integer vector of internal \code{yyjson}
#'        options.  See \code{read_flag} in this package, and read
#'        the yyjson API documentation for more information.  This is considered
#'        an advanced option.
#' @param str_specials Should \code{'NA'} in a JSON string be converted to the \code{'special'}
#'        \code{NA} value in R, or left as a \code{'string'}.  Default: 'string'
#' @param num_specials Should jsong strings 'NA'/'Inf'/'NaN' in a numeric context 
#'        be converted to the \code{'special'} R numeric values
#'        \code{NA, Inf, NaN}, or left as a \code{'string'}. Default: 'special'
#'
#' @seealso [read_flag()]
#' @return Named list of options
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
from_opts <- function(
    int64             = c('string', 'bit64'),
    missing_list_elem = c('null', 'na'),
    vectors_to_df     = TRUE,
    str_specials      = c('string', 'special'),
    num_specials      = c('special', 'string'),
    yyjson_read_flag  = 0L
) {
  
  structure(
    list(
      int64             = match.arg(int64),
      missing_list_elem = match.arg(missing_list_elem),
      vectors_to_df     = isTRUE(vectors_to_df),
      str_specials      = match.arg(str_specials),
      num_specials      = match.arg(num_specials),
      yyjson_read_flag  = as.integer(yyjson_read_flag)
    ),
    class = "from_opts"
  )
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Create named list of options for serializing R to JSON
#'
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
#' @param yyjson_write_flag integer vector corresponding to internal \code{yyjson}
#'        options.  See \code{write_flag} in this package, and read
#'        the yyjson API documentation for more information.  This is considered
#'        an advanced option.
#' 
#' @examples
#' \dontrun{
#' to_json_str(iris, opts = to_opts(factor = 'integer'))
#' }
#' 
#' 
#' @seealso [write_flag()]
#' @return Named list of options
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
to_opts <- function( 
    dataframe         = c("rows", "columns"),
    factor            = c("string", "integer"),
    auto_unbox        = FALSE,
    pretty            = FALSE,
    name_repair       = c('none', 'minimal'),
    num_specials      = c('null', 'string'),
    str_specials      = c('null', 'string'),
    yyjson_write_flag = 0L) {
  
  structure(
    list(
      dataframe         = match.arg(dataframe),
      factor            = match.arg(factor),
      auto_unbox        = isTRUE(auto_unbox),
      pretty            = isTRUE(pretty),
      name_repair       = match.arg(name_repair),
      str_specials      = match.arg(str_specials),
      num_specials      = match.arg(num_specials),
      yyjson_write_flag = as.integer(yyjson_write_flag)
    ),
    class = "to_opts"
  )
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Convert JSON to R
#' 
#' @param str a single character string
#' @param opts Named list of options for parsing. Usually created by \code{from_opts()}
#' @param ... Other named options can be used to override any options in \code{opts}.
#'        The valid named options are identical to arguments to [from_opts()]
#'
#'
#' @examples
#' \dontrun{
#' from_json_str(str, opts = iris_opts(int64 = 'string'))
#' }
#' 
#' @family JSON Parsers
#' @return R object
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
from_json_str <- function(str, opts = from_opts(), ...) {
  .Call(
    parse_from_str_, 
    str, 
    utils::modifyList(opts, list(...))
  )
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Convert JSON to R
#' 
#' @inheritParams from_json_str
#' @param filename full path to text file containing JSON.
#' 
#' 
#' @examples
#' \dontrun{
#' from_json_file("myfile.json")
#' }
#' 
#' @family JSON Parsers
#' @return R object
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
from_json_file <- function(filename, opts = from_opts(), ...) {
  .Call(
    parse_from_file_, 
    filename, 
    utils::modifyList(opts, list(...))
  )
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Parse JSON from an R connection object.
#' 
#' Currently, this isn't very efficient as the entire contents of the connection are 
#' read into R as a string and then the JSON parsed from there.
#' 
#' For plain text files it is faster to use
#' \code{from_json_file()}.
#' 
#' @inheritParams from_json_str
#' @param conn connection object.  e.g. \code{url('https://jsonplaceholder.typicode.com/todos/1')}
#'
#'
#' @examples
#' \dontrun{
#' from_json_conn(url("https://api.github.com/users/hadley/repos"))
#' }
#' 
#' 
#' @family JSON Parsers
#' @return R object
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
from_json_conn <- function(conn, opts = from_opts(), ...) {
  str <- paste(readLines(conn), collapse = "")
  from_json_str(str, opts, ...)
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Convert R object to JSON string
#' 
#' @param x the object to be encoded
#' @param opts Named list of serialization options. Usually created by [to_opts()]
#' @param ... Other named options can be used to override any options in \code{opts}.
#'        The valid named options are identical to arguments to [to_opts()]
#' 
#' @return Character string
#' 
#' @examples
#' \dontrun{
#' to_json_str(iris, pretty = TRUE)
#' to_json_str(iris, opts = to_opts(auto_unbox = FALSE))
#' }
#' 
#' @family JSON Serializer
#' @importFrom utils modifyList
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
to_json_str <- function(x, opts = to_opts(), ...) {
  .Call(
    serialize_to_str_, 
    x, 
    utils::modifyList(opts, list(...), keep.null = FALSE)
  )
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Convert R object to JSON file
#' 
#' @inheritParams to_json_str
#' @param filename filename
#' 
#' @examples
#' \dontrun{
#' to_json_str(iris, filename = "iris.json")
#' }
#' 
#' @family JSON Serializer
#' @importFrom utils modifyList
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
to_json_file <- function(x, filename, opts = to_opts(), ...) {
  .Call(
    serialize_to_file_, 
    x,
    filename, 
    utils::modifyList(opts, list(...), keep.null = FALSE)
  )
  
  invisible(NULL)
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Parse an ndjson file to a data.frame or list
#' 
#' If reading as data.frame, each row of ndjson becomes a row in the data.frame.  
#' If reading as a list, then each row becomes an element in the list.
#' 
#' If parsing ndjson to a data.frame it is usually better if the json objects
#' are consistent from line-to-line.  Type inference for the data.frame is done
#' during initialisation by reading through \code{nprobe} lines.  Warning: if
#' there is a type-mismatch further into the file than it is probed, then you 
#' will get missing values in the data.frame.
#' 
#' No flattening of the namespace is done i.e. nested object remain nested.
#' 
#' @inheritParams from_json_str
#' @param filename text file or gzipped file
#' @param nread number of records to read. Default: -1 (reads all rows)
#' @param nskip number of records to skip before starting to read. Default: 0
#' @param nprobe Number of lines to read to determine types for data.frame
#'        columns.  Default: 100. Only valid for \code{from_ndjson_file_as_df()}
#'
#'
#' @examples
#' \dontrun{
#' from_ndjson_file_as_df("flights.ndjson", nskip = 1000, nread = 200)
#' from_ndjson_file_as_list("flights.ndjson", nskip = 1000, nread = 200)
#' }
#' 
#' @family JSON Parsers
#' @return data.frame
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
from_ndjson_file_as_df <- function(filename, nread = -1, nskip = 0, nprobe = 100, opts = from_opts(), ...) {
  
  .Call(
    parse_ndjson_file_as_df_,
    filename, 
    nread,
    nskip,
    nprobe,
    utils::modifyList(opts, list(...), keep.null = FALSE)
  )
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' @rdname from_ndjson_file_as_df
#'
#' @family JSON Parsers
#' @return list
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
from_ndjson_file_as_list <- function(filename, nread = -1, nskip = 0, opts = from_opts(), ...) {
  .Call(
    parse_ndjson_file_as_list_,
    filename, 
    nread,
    nskip,
    utils::modifyList(opts, list(...), keep.null = FALSE)
  )
}


