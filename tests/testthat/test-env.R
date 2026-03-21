
test_that("Environments works", {

  ll <- list(a = 1, b = "hello")
  ee <- as.environment(ll)
  
  ll_str <- write_json_str(ll)
  ee_str <- write_json_str(ee)

  # Should have the same contents, but environment name ordering is dependant
  # upon ordering of the hash-of-the-name, not the name itself
  ll_str
  ee_str

  # So read the strings back into objects
  ll2 <- read_json_str(ll_str)
  ee2 <- read_json_str(ee_str)

  expect_true(is.list(ll2))
  expect_true(is.list(ee2))

  for (nm in names(ll)) {
    expect_identical(ll[[nm]], ll2[[nm]])
    expect_identical(ll[[nm]], ee [[nm]])
  }

  expect_identical(
    sort(unlist(ll2)),
    sort(unlist(ee2))
  )

})

