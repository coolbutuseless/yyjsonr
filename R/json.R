

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Convert JSON in a character string to R
#' 
#' @param str a single character string
#' @param opts Named list of options for parsing. Usually created by \code{from_opts()}
#' @param ... Other named options can be used to override any options in \code{opts}.
#'        The valid named options are identical to arguments to [from_opts()]
#'
#'
#' @examples
#' \dontrun{
#' read_json_str(str, opts = parse_opts(int64 = 'string'))
#' }
#' 
#' @family JSON Parsers
#' @return R object
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
read_json_str <- function(str, opts = list(), ...) {
  .Call(
    parse_from_str_, 
    str, 
    modify_list(opts, list(...))
  )
}

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Convert JSON in a raw vector to R
#' 
#' @inheritParams read_json_str
#' @param raw_vec raw vector
#'
#'
#' @examples
#' \dontrun{
#' a <- nanonext::ncurl("https://postman-echo.com/get", convert = FALSE)
#' read_json_raw(a$raw)
#' }
#' 
#' @family JSON Parsers
#' @return R object
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
read_json_raw <- function(raw_vec, opts = list(), ...) {
  .Call(
    parse_from_raw_, 
    raw_vec, 
    modify_list(opts, list(...))
  )
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Convert JSON to R
#' 
#' @inheritParams read_json_str
#' @param filename full path to text file containing JSON.
#' 
#' 
#' @examples
#' \dontrun{
#' read_json_file("myfile.json")
#' }
#' 
#' @family JSON Parsers
#' @return R object
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
read_json_file <- function(filename, opts = list(), ...) {
  .Call(
    parse_from_file_, 
    filename, 
    modify_list(opts, list(...))
  )
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Parse JSON from an R connection object.
#' 
#' Currently, this isn't very efficient as the entire contents of the connection are 
#' read into R as a string and then the JSON parsed from there.
#' 
#' For plain text files it is faster to use
#' \code{read_json_file()}.
#' 
#' @inheritParams read_json_str
#' @param conn connection object.  e.g. \code{url('https://jsonplaceholder.typicode.com/todos/1')}
#'
#'
#' @examples
#' \dontrun{
#' read_json_conn(url("https://api.github.com/users/hadley/repos"))
#' }
#' 
#' 
#' @family JSON Parsers
#' @return R object
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
read_json_conn <- function(conn, opts = list(), ...) {
  str <- paste(readLines(conn), collapse = "")
  read_json_str(str, opts, ...)
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
#' write_json_str(iris, pretty = TRUE)
#' write_json_str(iris, opts = to_opts(auto_unbox = FALSE))
#' }
#' 
#' @family JSON Serializer
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
write_json_str <- function(x, opts = list(), ...) {
  .Call(
    serialize_to_str_, 
    x, 
    modify_list(opts, list(...))
  )
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Convert R object to JSON file
#' 
#' @inheritParams write_json_str
#' @param filename filename
#' 
#' @examples
#' \dontrun{
#' write_json_str(iris, filename = "iris.json")
#' }
#' 
#' @family JSON Serializer
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
write_json_file <- function(x, filename, opts = list(), ...) {
  .Call(
    serialize_to_file_, 
    x,
    filename, 
    modify_list(opts, list(...))
  )
  
  invisible(NULL)
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Validate JSON in file or string
#'
#' @inheritParams read_json_file
#' @param filename path to file containing JSON
#' @param str character string containing JSON
#' @param verbose logical. If the JSON is not valid, should a warning be 
#'        shown giving details?
#'
#' @return Logical value. TRUE if JSON validates as OK, otherwise FALSE
#' @export 
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
validate_json_file <- function(filename, verbose = FALSE, opts = list(), ...) {
  opts <- modify_list(opts, list(...))
  
  .Call(
    validate_json_file_,
    filename,
    verbose,
    opts
  )
}

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' @rdname validate_json_file
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
validate_json_str <- function(str, verbose = FALSE, opts = list(), ...) {
  opts <- modify_list(opts, list(...))
  
  .Call(
    validate_json_str_,
    str,
    verbose,
    opts
  )
}


