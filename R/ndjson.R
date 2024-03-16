

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Parse an NDJSON file to a data.frame or list
#' 
#' If reading as data.frame, each row of NDJSON becomes a row in the data.frame.  
#' If reading as a list, then each row becomes an element in the list.
#' 
#' If parsing NDJSON to a data.frame it is usually better if the json objects
#' are consistent from line-to-line.  Type inference for the data.frame is done
#' during initialisation by reading through \code{nprobe} lines.  Warning: if
#' there is a type-mismatch further into the file than it is probed, then you 
#' will get missing values in the data.frame, or JSON values not captured in 
#' the R data.
#' 
#' No flattening of the namespace is done i.e. nested object remain nested.
#' 
#' @inheritParams read_json_str
#' @param filename Path to file containing NDJSON data. May e a vanilla text 
#'        file or a gzipped file
#' @param type The type of R object the JSON should be parsed into. Valid 
#'        values are 'df' or 'list'.  Default: 'df' (data.frame)
#' @param nread Number of records to read. Default: -1 (reads all JSON strings)
#' @param nskip Number of records to skip before starting to read. Default: 0 
#'        (skip no data)
#' @param nprobe Number of lines to read to determine types for data.frame
#'        columns.  Default: 100.   Use \code{-1} to probe entire file.
#'
#'
#' @examples
#' tmp <- tempfile()
#' write_ndjson_file(head(mtcars), tmp)
#' read_ndjson_file(tmp)
#' 
#' @family JSON Parsers
#' @return NDJSON data read into R as list or data.frame depending 
#'         on \code{'type'} argument
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
read_ndjson_file <- function(filename, type = c('df', 'list'), nread = -1, nskip = 0, nprobe = 100, opts = list(), ...) {
  
  type <- match.arg(type)
  filename <- normalizePath(filename, mustWork = TRUE)
  
  if (type == 'list') {
    .Call(
      parse_ndjson_file_as_list_,
      filename, 
      nread,
      nskip,
      modify_list(opts, list(...))
    )
  } else {
    .Call(
      parse_ndjson_file_as_df_,
      filename, 
      nread,
      nskip,
      nprobe,
      modify_list(opts, list(...))
    )
  }
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Parse an NDJSON file to a data.frame or list
#' 
#' If reading as data.frame, each row of NDJSON becomes a row in the data.frame.  
#' If reading as a list, then each row becomes an element in the list.
#' 
#' If parsing NDJSON to a data.frame it is usually better if the json objects
#' are consistent from line-to-line.  Type inference for the data.frame is done
#' during initialisation by reading through \code{nprobe} lines.  Warning: if
#' there is a type-mismatch further into the file than it is probed, then you 
#' will get missing values in the data.frame, or JSON values not captured in 
#' the R data.
#' 
#' No flattening of the namespace is done i.e. nested object remain nested.
#' 
#' @inheritParams read_ndjson_file
#' @param x string containing NDJSON
#'
#' @examples
#' tmp <- tempfile()
#' json <- write_ndjson_str(head(mtcars))
#' read_ndjson_str(json, type = 'list')
#' 
#' @family JSON Parsers
#' @return NDJSON data read into R as list or data.frame depending 
#'         on \code{'type'} argument
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
read_ndjson_str <- function(x, type = c('df', 'list'), nread = -1, nskip = 0, nprobe = 100, opts = list(), ...) {
  
  type <- match.arg(type)
  
  if (type == 'list') {
    .Call(
      parse_ndjson_str_as_list_,
      x, 
      nread,
      nskip,
      modify_list(opts, list(...))
    )
  } else {
    .Call(
      parse_ndjson_str_as_df_,
      x, 
      nread,
      nskip,
      nprobe,
      modify_list(opts, list(...))
    )
  }
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Write list or data.frame object to NDJSON in a file
#' 
#' For \code{list} input, each element of the list is written as a single JSON string.
#' For \code{data.frame} input, each row of the \code{data.frame} is written
#' as aJSON string.
#' 
#' @inherit write_json_file
#' @param x \code{data.frame} or \code{list} to be written as multiple JSON strings
#' @param filename JSON strings will be written to this file one-line-per-JSON string.
#'
#' @return None
#' @family JSON Serializer
#' @export
#' 
#' @examples
#' tmp <- tempfile()
#' write_ndjson_file(head(mtcars), tmp)
#' read_ndjson_file(tmp)
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
write_ndjson_file <- function(x, filename, opts = list(), ...) {
  opts <- modify_list(opts, list(...))
  filename <- normalizePath(filename, mustWork = FALSE)
  
  if (is.data.frame(x)) {
    .Call(
      serialize_df_to_ndjson_file_,
      x,
      filename,
      opts
    )
  } else {
    .Call(
      serialize_list_to_ndjson_file_,
      x,
      filename,
      opts
    )
  }
  invisible()
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Write list or data.frame object to NDJSON in a string
#' 
#' For \code{list} input, each element of the list is written as a single JSON string.
#' For \code{data.frame} input, each row of the \code{data.frame} is written
#' as aJSON string.
#' 
#' @inherit write_ndjson_file
#'
#' @return String containing multiple JSON strings separated by newlines.
#' @family JSON Serializer
#' @export
#' 
#' @examples
#' write_ndjson_str(head(mtcars))
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
write_ndjson_str <- function(x, opts = list(), ...) {
  opts <- modify_list(opts, list(...))
  
  if (is.data.frame(x)) {
    .Call(
      serialize_df_to_ndjson_str_,
      x,
      opts
    )
  } else {
    .Call(
      serialize_list_to_ndjson_str_,
      x,
      opts
    )
  }
}


