

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Parse Dataset-NDJSON
#'
#' \code{Dataset NDJSON} is a specific format of \code{NDJSON} proposed by 
#' \code{CDISC}
#' 
#' \code{Dataset NDJSON} is a specific format of \code{NDJSON} where the 
#' first row contains metadata for the dataset (including a specification
#' for the data which follows).
#' 
#' After the first row in the data, each of the following rows represent
#' JSON array data. Each element in the array must adhere to the spec given
#' in the first row.
#' 
#' Note: the Dataset-NDJSON file must closely follow the v1.1 specification 
#' in order to be parsed correctly. In particular the \code{columns} specification
#' in the first row (containing the metadata) must include \code{name} and
#' \code{dataType} data.
#' 
#' @inheritParams read_json_str
#' @param rvec raw vector
#' @param filename Filename
#'
#' 
#' @family JSON Parsers
#' @return data.frame with the attribute \code{metadata} containing the 
#'         metadata from the first row of the data.
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
read_dataset_ndjson_str <- function(str, opts = list(), ...) {
  
  spec  <- read_json_str(str, yyjson_read_flag = yyjson_read_flag$YYJSON_READ_STOP_WHEN_DONE)
  nskip <- 1
  
  df <- .Call(
    parse_dataset_ndjson_as_df_,
    str, 
    spec$columns,
    nskip,
    modify_list(opts, list(...)),
    0  # input is string
  )
  
  attr(df, "metadata") <- spec
  df
}



#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' @rdname read_dataset_ndjson_str
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
read_dataset_ndjson_raw <- function(rvec, opts = list(), ...) {
  
  spec  <- read_json_raw(rvec, yyjson_read_flag = yyjson_read_flag$YYJSON_READ_STOP_WHEN_DONE)
  nskip <- 1
  
  df <- .Call(
    parse_dataset_ndjson_as_df_,
    rvec, 
    spec$columns,
    nskip,
    modify_list(opts, list(...)),
    1 # input is raw
  )
  
  attr(df, "metadata") <- spec
  df
}



#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' @rdname read_dataset_ndjson_str
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
read_dataset_ndjson_file <- function(filename, opts = list(), ...) {
  
  spec  <- read_json_file(filename, yyjson_read_flag = yyjson_read_flag$YYJSON_READ_STOP_WHEN_DONE)
  nskip <- 1
  
  df <- .Call(
    parse_dataset_ndjson_as_df_,
    filename, 
    spec$columns,
    nskip,
    modify_list(opts, list(...)),
    2 # input is file
  )
  
  attr(df, "metadata") <- spec
  df
}



