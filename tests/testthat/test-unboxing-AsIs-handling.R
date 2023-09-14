

test_that("AsIs handling works", {
  library(jsonlite)
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  #' I() forces scalar values to be written as JSON-array
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  l1 <- list(a = 1, b = I(2))
  expect_equal(
    write_json_str(l1, auto_unbox = TRUE),
    '{"a":1.0,"b":[2.0]}'
  )
  expect_equal(
    write_json_str(l1, auto_unbox = FALSE),
    '{"a":[1.0],"b":[2.0]}'
  )
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  #' Unboxing doesn't affect vectors with length > 1
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  l2 <- list(a = 1, b = I(c(2, 3)))
  expect_equal(
    write_json_str  (l2, auto_unbox = TRUE),
    '{"a":1.0,"b":[2.0,3.0]}'
  )
  expect_equal(
    write_json_str  (l2, auto_unbox = FALSE),
    '{"a":[1.0],"b":[2.0,3.0]}'
  )
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  #' 
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 
  s <- '{"a":1,"b":[2]}'
  
  x <- read_json_str(s)
  expect_identical(x, list(a = 1L, b = 2L))
  
  
  x <- read_json_str(s, length1_array_asis = TRUE)
  expect_identical(x, list(a = 1L, b = structure(2L, class = 'AsIs')))
  
  s2 <- write_json_str(x, auto_unbox = TRUE)
  expect_identical(s, s2)
    
})
