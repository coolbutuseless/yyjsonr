
test_that("valgrind check works", {
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # This is a json which gets partically parsed before an error occurs.
  # This might be a pathway to trigger a valgrind error as yyjson C lib
  # fails to free all memory?
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  expect_error(
    read_json_str('[1, 2, {"a":1, "b":"hello", "c":"test"}, {1}')
  )
  
})
