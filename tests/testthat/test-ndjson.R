

ref <- iris
ref$Species <- as.character(ref$Species)

if (FALSE) {
  # jsonlite::stream_out(ref, file("./examples/iris.ndjson"))
}

test_that("ndjson works", {
  
  nd  <- from_ndjson_file_as_df("./examples/iris.ndjson")
  ndz <- from_ndjson_file_as_df("./examples/iris.ndjson.gz")
  
  expect_identical(nd , ref)  
  expect_identical(ndz, ref)  
  expect_identical(nd , ndz)
  
  
  ndl  <- from_ndjson_file_as_list("./examples/iris.ndjson")
  expect_length(ndl, 150)
  lens <- lengths(ndl)
  expect_equal(lens, rep(5, 150))
  
  
  
  nd  <- from_ndjson_file_as_df("./examples/iris.ndjson", nprobe = 2, 
                                nskip = 10, nread = 10)
  ref2 <- ref[11:20,]
  rownames(ref2) <- NULL
  expect_identical(nd, ref2)
  
  
  
  nd  <- from_ndjson_file_as_list("./examples/iris.ndjson",
                                nskip = 10, nread = 10)
  expect_length(nd, 10)  
  
  
  expect_error(from_ndjson_file_as_list("does_not_exist.txt"))
  
  
  
})
 