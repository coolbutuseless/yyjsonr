test_that("yyjson flags work", {
  

  expect_error(
    capture_output(
      read_json_str('[1,2,3,]'), 'comma'
    )
  )
  
  expect_equal(
    read_json_str('[1,2,3,]', yyjson_read_flag = read_flag$YYJSON_READ_ALLOW_TRAILING_COMMAS),
    c(1L, 2L, 3L)
  )
  
})
