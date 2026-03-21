

test_that("multiplication works", {
  res <- '{"foo": [ ["a", "b", "c"], ["x", "y"]]}' |> 
    read_json_str()
  expect_true(is.list(res$foo))

  res <- '{"foo": [ ["a", "b", "c"], ["x", "y", "z"]]}' |> 
    read_json_str()
  expect_true(is.matrix(res$foo))



  res <- '{"foo": [ ["a", "b", "c"], ["x", "y", "z"]]}' |> 
    read_json_str(arr_of_arrs_to_matrix = FALSE)
  expect_false(is.matrix(res$foo))
  expect_true(is.list(res$foo))
})

