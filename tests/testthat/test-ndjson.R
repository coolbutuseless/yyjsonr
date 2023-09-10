

ref <- iris
ref$Species <- as.character(ref$Species)

# transposed version
tref <- lapply(seq_len(length(ref[[1]])), function(i) lapply(ref, "[[", i))


if (FALSE) {
  # jsonlite::stream_out(ref, file("./examples/iris.ndjson"))
}

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Parse
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
test_that("ndjson works", {
  
  nd  <- read_ndjson_file("./examples/iris.ndjson", type = 'df')
  ndz <- read_ndjson_file("./examples/iris.ndjson.gz", type = 'df')
  
  expect_identical(nd , ref)  
  expect_identical(ndz, ref)  
  expect_identical(nd , ndz)
  
  
  ndl  <- read_ndjson_file("./examples/iris.ndjson", type = 'list')
  expect_length(ndl, 150)
  lens <- lengths(ndl)
  expect_equal(lens, rep(5, 150))
  
  
  
  nd  <- read_ndjson_file("./examples/iris.ndjson", nprobe = 2, 
                                nskip = 10, nread = 10)
  ref2 <- ref[11:20,]
  rownames(ref2) <- NULL
  expect_identical(nd, ref2)
  
  
  
  nd  <- read_ndjson_file("./examples/iris.ndjson", type = 'list',
                                nskip = 10, nread = 10)
  expect_length(nd, 10)  
  expect_error(read_ndjson_file("does_not_exist.txt"))
})


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Serialize
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
test_that("write_ndjson_file df works", {
  file <- tempfile()
  write_ndjson_file(iris, file)
  res <- read_ndjson_file(file)  
  expect_identical(res, ref)
  
  res <- read_ndjson_file(file, type = 'list')
  expect_identical(res, tref)
})
 

test_that("write_ndjson_str df works", {
  file <- tempfile()
  write_ndjson_file(iris, file)
  ref <- write_ndjson_str(iris)
  res <- paste(readLines(file), collapse = "\n")  
  expect_identical(res, ref)
})

test_that("write_ndjson_file list works", {
  file <- tempfile()
  write_ndjson_file(tref, file)
  res <- read_ndjson_file(file, type = 'list')  
  expect_identical(res, tref)
})



test_that("write_ndjson_str list works", {
  file <- tempfile()
  write_ndjson_file(tref, file)
  ref <- write_ndjson_str(tref)
  res <- paste(readLines(file), collapse = "\n")  
  expect_identical(res, ref)
})







