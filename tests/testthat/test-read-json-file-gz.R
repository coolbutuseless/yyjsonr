
test_that("reading from gz compressed files works", {

  expect_identical(  
    read_json_file(testthat::test_path("examples/mtcars.json")),
    read_json_file(testthat::test_path("examples/mtcars.json.gz"))
  )
})
