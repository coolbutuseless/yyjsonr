

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
#' @inheritParams read_json_str
#' @param filename text file or gzipped file
#' @param type 'df' or 'list'.  Default: 'df' (data.frame)
#' @param nread number of records to read. Default: -1 (reads all rows)
#' @param nskip number of records to skip before starting to read. Default: 0
#' @param nprobe Number of lines to read to determine types for data.frame
#'        columns.  Default: 100. Only valid for \code{read_ndjson_file()}
#'
#'
#' @examples
#' \dontrun{
#' read_ndjson_file("flights.ndjson", nskip = 1000, nread = 200)
#' read_ndjson_file("flights.ndjson", nskip = 1000, nread = 200)
#' }
#' 
#' @family JSON Parsers
#' @return list or data.frame
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
read_ndjson_file <- function(filename, type = c('df', 'list'), nread = -1, nskip = 0, nprobe = 100, opts = list(), ...) {
  
  type <- match.arg(type)

  filename <- path.expand(filename[1])
  
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


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Write list or data.frame object to ndjson 
#' 
#' @inherit write_json_file
#' @param x data.frame or list
#' @param filename filename 
#'
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
write_ndjson_file <- function(x, filename, opts = list(), ...) {
  opts <- modify_list(opts, list(...))
  
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
#' @rdname write_ndjson_file
#' @export
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


