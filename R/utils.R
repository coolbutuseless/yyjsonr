
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Work-a-like replacement for built-in 'modifyList'
#' 
#' @param old,new lists
#' @return updated list
#' @noRd
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
modify_list <- function(old, new) {
  for (nm in names(new)) old[[nm]] <- new[[nm]]
  old
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Version number of 'yyjson' C library
#' 
#' @export
#' @return Version of included yyjson C library as a string
#' @examples
#' yyjson_version()
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
yyjson_version <- function() {
  .Call(yyjson_version_)
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Tag an atomic vector with a single element as class 'scalar'.  When output
#' to JSON it will be output as a scalar not a vector
#' 
#' @param x atomic vector with length = 1
#' @return If x is an atomic vector with length = 1, then object is returned
#'         with class 'scalar', otherwise object is returned unmodified
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
as_scalar <- function(x) {
  if (is.atomic(x) && length(x) == 1) {
    class(x) <- union('scalar', class(x))
  }
  x
}

