

test_that("option 'single_null' works", {
  
  expect_identical(
    read_json_str('{"a":null,"b":3}'),
    list(a = NULL, b = 3L)
  )
  
  
  
  expect_identical(
    read_json_str('{"a":null,"b":3}', single_null = "hello"),
    list(a = "hello", b = 3L)
  )
  
})
