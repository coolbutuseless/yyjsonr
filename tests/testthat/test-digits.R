
test_that("digits argument works", {
  
  robj <- c(1.51, 2, 3.141592654)
  
  
  expect_equal(
    write_json_str(robj, digits = -1),
    "[1.51,2.0,3.141592654]"
  )
  
  expect_equal(
    write_json_str(robj, digits = 0),
    "[2,2,3]"
  )
  
  expect_equal(
    write_json_str(robj, digits = 1),
    "[1.5,2.0,3.1]"
  )
  
  expect_equal(
    write_json_str(robj, digits = 2),
    "[1.51,2.0,3.14]"
  )
  
  
  
  expect_equal(
    write_json_str(pi, digits = 0),
    "[3]"
  )
  
  expect_equal(
    write_json_str(pi, digits = 1),
    "[3.1]"
  )
  
  expect_equal(
    write_json_str(pi, digits = 4),
    "[3.1416]"
  )
  
  
  expect_equal(
    write_json_str(pi, digits = 4, auto_unbox = TRUE),
    "3.1416"
  )
  
  
})
