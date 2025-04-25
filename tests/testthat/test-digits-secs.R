

test_that("posixct fractional seconds works", {

  x <- as.POSIXct("2025-01-01", tz = 'UTC') + 123/1e3  
  
  expect_identical(yyjsonr::write_json_str(x)                 , "[\"2025-01-01 00:00:00\"]")
  expect_identical(yyjsonr::write_json_str(x, digits_secs = 0), "[\"2025-01-01 00:00:00\"]")
  expect_identical(yyjsonr::write_json_str(x, digits_secs = 1), "[\"2025-01-01 00:00:00.1\"]")
  expect_identical(yyjsonr::write_json_str(x, digits_secs = 2), "[\"2025-01-01 00:00:00.12\"]")
  expect_identical(yyjsonr::write_json_str(x, digits_secs = 3), "[\"2025-01-01 00:00:00.123\"]")
  expect_identical(yyjsonr::write_json_str(x, digits_secs = 4), "[\"2025-01-01 00:00:00.1230\"]")
  expect_identical(yyjsonr::write_json_str(x, digits_secs = 5), "[\"2025-01-01 00:00:00.12300\"]")
  expect_identical(yyjsonr::write_json_str(x, digits_secs = 6), "[\"2025-01-01 00:00:00.123000\"]")
  
})

