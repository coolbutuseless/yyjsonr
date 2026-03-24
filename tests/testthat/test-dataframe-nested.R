



test_that("data.frame with list-columns works", {
  df <- data.frame(x = c('a', 'b')) # two rows in outer df
  df1 <- data.frame(m1 = c(11L), m2 = c(12L)) # one row in nested df
  df2 <- data.frame(m1 = c(21L), m2 = c(22L)) # one row in nested df
  df$params <- list(df1, df2)
  
  res <- write_json_str(df)
  
  expected <- '[{"x":"a","params":[{"m1":11,"m2":12}]},{"x":"b","params":[{"m1":21,"m2":22}]}]'
  expect_identical(res, expected)
})




# This is NOT a list column
test_that("single row data.frame with raw nested dataframe works", {
  df <- data.frame(x = c('a')) # one row in outer df
  df1 <- data.frame(m1 = c(11L), m2 = c(12L))
  df$params <- df1

  res <- write_json_str(df)

  expected <- '[{"x":"a","params":{"m1":11,"m2":12}}]'
  expect_identical(res, expected)
})


# This is NOT a list column
test_that("multi row data.frame with raw nested dataframe works", {
  df <- data.frame(x = c('a', 'b')) # two rows in outer df
  df1 <- data.frame(m1 = c(11L, 12L), m2 = c(21L, 22L))
  df$params <- df1

  res <- write_json_str(df)

  expected <- '[{"x":"a","params":{"m1":11,"m2":21}},{"x":"b","params":{"m1":12,"m2":22}}]'
  expect_identical(res, expected)
})


