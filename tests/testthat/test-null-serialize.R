

test_that("null option for serialization works", {
  
  
  json <- write_json_str(NULL, null = 'null')
  expect_identical(json, "null")
  
  json <- write_json_str(list(NULL, NULL), null = 'null')
  expect_identical(json, "[null,null]")
  
  json <- write_json_str(list(a = NULL, b = NULL), null = 'null')
  expect_identical(json, '{"a":null,"b":null}')
  
  
  
  json <- write_json_str(NULL, null = 'empty_array')
  expect_identical(json, "[]")
  
  json <- write_json_str(list(NULL, NULL), null = 'empty_array')
  expect_identical(json, "[[],[]]")
  
  json <- write_json_str(list(a = NULL, b = NULL), null = 'empty_array')
  expect_identical(json, '{"a":[],"b":[]}')
  
  
})
