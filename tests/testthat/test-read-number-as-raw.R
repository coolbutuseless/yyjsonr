

test_that("YYJSON_READ_NUMBER_AS_RAW works", {

  s <- "{\"a\":[1.23456789012345]}"
  j <- read_json_str(
    s, 
    opts = opts_read_json(
      yyjson_read_flag = yyjson_read_flag$YYJSON_READ_NUMBER_AS_RAW
    )
  )
  
  expect_identical(
    j$a,
    "1.23456789012345"
  )
  
})

