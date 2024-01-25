
test_that("fast-numerics works", {
  expect_equal(
    write_json_str(mtcars, dataframe = 'columns', fast_numerics = FALSE),
    write_json_str(mtcars, dataframe = 'columns', fast_numerics =  TRUE)
  )
  
  expect_equal(
    write_json_str(iris, dataframe = 'columns', fast_numerics = FALSE),
    write_json_str(iris, dataframe = 'columns', fast_numerics =  TRUE)
  )
})
