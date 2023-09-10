
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