


test_that("multiplication works", {
  
  ref <- head(iris, 5)
  ref$Species <- as.character(ref$Species)
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  #' Parse array of objects to a data.frame
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  arr_of_objs <- write_json_str(ref, auto_unbox = TRUE, pretty = TRUE)
  x <- read_json_str(arr_of_objs, arr_of_objs_to_df = TRUE)
  expect_identical(x, ref)
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  #' Leave array-of-objects as a list
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  x <- read_json_str(arr_of_objs, arr_of_objs_to_df = FALSE)
  expect_true(is.list(x))
  expect_false(is.data.frame(x))
  expect_length(x, 5)
  expect_null(names(x))
  expect_identical(names(x[[1]]), colnames(ref))
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  #' Object of arrays
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  obj_of_arrs <- write_json_str(as.list(ref), auto_unbox = TRUE, pretty = TRUE)
  
  # Read as data.frame
  x <- read_json_str(obj_of_arrs, obj_of_arrs_to_df = TRUE)
  expect_identical(x, ref)
  
  # read as list
  x <- read_json_str(obj_of_arrs, obj_of_arrs_to_df = FALSE)
  expect_false(is.data.frame(x))
  expect_true(is.list(x))
  expect_identical(names(x), colnames(ref))
  
  
})
