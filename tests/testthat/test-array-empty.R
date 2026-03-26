
test_that("empty_array option works", {
  
  res <- read_json_str("[]")
  expect_identical(res, list())
  
  res <- read_json_str("[]", empty_array = 'list')
  expect_identical(res, list())
  
  res <- read_json_str("[]", empty_array = 'NULL')
  expect_identical(res, NULL)
  
  
  res <- read_json_str('{"a":[],"b":[],"c":3}', empty_array = 'list')
  expect_identical(res, list(a = list(), b = list(), c = 3L))
  
  res <- read_json_str('{"a":[],"b":[],"c":3}', empty_array = 'NULL')
  expect_identical(res, list(a = NULL, b = NULL, c = 3L))
  
})
