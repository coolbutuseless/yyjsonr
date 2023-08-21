
test_that("validation works", {
  
  expect_true(validate_json_str('["hello"]'))
  expect_false(validate_json_str('["hello]'))
  
  out <- capture_output(
    expect_warning(validate_json_str('["hello]', verbose = TRUE), "parsing")
  )  
  
  expect_true(validate_json_file("./examples/test.json"))
  expect_false(validate_json_file("./examples/flights.ndjson"))
  expect_warning(validate_json_file("./examples/flights.ndjson", verbose = TRUE), "parsing")
})
