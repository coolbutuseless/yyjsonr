
test_that("Scalar values work", {
  expect_identical(read_json_str("true"), TRUE)
  expect_identical(read_json_str("3")   , 3L)
  expect_identical(read_json_str("3.1") , 3.1)
})



test_that("arrays works", {
  str <- '[]'; 
  expect_identical(read_json_str(str), list())
  
  str <- '[true, true, false]'; 
  expect_identical(read_json_str(str), c(TRUE, TRUE, FALSE))
  
  str <- '[1, 2, 3, 99]'; 
  expect_identical(read_json_str(str), c(1L, 2L, 3L, 99L))
  
  str <- '[1, 2, 3, 99.99]'; 
  expect_identical(read_json_str(str), c(1, 2, 3, 99.99))
  
  # str <- '[1, 2, 3, 99.99, "apple"]'; 
  # expect_identical(read_json_str(str), list(1L, 2L, 3L, 99.99, "apple"))
  
  str <- '["apple", "banana"]'; 
  expect_identical(read_json_str(str), c('apple', 'banana'))
})


# test_that("objects work", {
#   str <- '{"greg": 3.1, "apple": 2}'
#   expect_identical(read_json_str(str), list(greg = 3.1, apple = 2L))
# })
