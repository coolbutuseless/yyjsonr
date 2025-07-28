

ref <- iris
ref$Species <- as.character(ref$Species)

# transposed version
tref <- lapply(seq_len(length(ref[[1]])), function(i) lapply(ref, "[[", i))


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Parse
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
test_that("ndjson works", {
  
  nd  <- read_ndjson_file(test_path("ndjson/iris.ndjson"   ), type = 'df')
  ndz <- read_ndjson_file(test_path("ndjson/iris.ndjson.gz"), type = 'df')
  
  expect_identical(nd , ref)  
  expect_identical(ndz, ref)  
  expect_identical(nd , ndz)
  
  
  ndl  <- read_ndjson_file(test_path("ndjson/iris.ndjson"), type = 'list')
  expect_length(ndl, 150)
  lens <- lengths(ndl)
  expect_equal(lens, rep(5, 150))
  
  
  
  nd  <- read_ndjson_file(test_path("ndjson/iris.ndjson"), nprobe = 2, 
                                nskip = 10, nread = 10)
  ref2 <- ref[11:20,]
  rownames(ref2) <- NULL
  expect_identical(nd, ref2)
  
  
  
  nd  <- read_ndjson_file(test_path("ndjson/iris.ndjson"), type = 'list',
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
  ref2 <- write_ndjson_str(iris)
  res <- paste(readLines(file), collapse = "\n")  
  expect_identical(res, ref2)
})

test_that("write_ndjson_file list works", {
  file <- tempfile()
  write_ndjson_file(tref, file)
  res <- read_ndjson_file(file, type = 'list')  
  expect_identical(res, tref)
})



test_that("write_ndjson_str list works", {
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # Check write_ndjson_file() and write_ndjson_str() agree
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  file <- tempfile()
  write_ndjson_file(tref, file)
  ref2 <- write_ndjson_str(tref)
  res <- paste(readLines(file), collapse = "\n")  
  expect_identical(res, ref2)
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # Read NDJSON string as list
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  json <- write_ndjson_str(tref)
  ref2 <- read_ndjson_str(json, type = 'list')  
  expect_identical(ref2, tref)
  
  json <- write_ndjson_str(tref)
  ref2 <- read_ndjson_str(json, type = 'list', nskip = 1)  
  expect_identical(ref2, tref[-1])
  
  json <- write_ndjson_str(tref)
  ref2 <- read_ndjson_str(json, type = 'list', nskip = 2, nread = 3)  
  expect_identical(ref2, tref[3:5])
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # Read NDJSON string as data.frame
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  json <- write_ndjson_str(ref)
  ref2 <- read_ndjson_str(json, type = 'df')  
  expect_identical(ref2, ref)
  
  json <- write_ndjson_str(ref)
  ref2 <- read_ndjson_str(json, type = 'df', nskip  = 1)  
  expect_identical(ref2, ref[-1, ], ignore_attr = TRUE)
  
  json <- write_ndjson_str(ref)
  ref2 <- read_ndjson_str(json, type = 'df', nskip  = 2, nread = 3)  
  expect_identical(ref2, ref[3:5, ], ignore_attr = TRUE)
})



test_that("read_ndjson_str() empty inputs", {
  
  # write_ndjson_str(head(mtcars, 2))
  input <- r"({"mpg":21.0,"cyl":6.0,"disp":160.0,"hp":110.0,"drat":3.9,"wt":2.62,"qsec":16.46,"vs":0.0,"am":1.0,"gear":4.0,"carb":4.0}
{"mpg":21.0,"cyl":6.0,"disp":160.0,"hp":110.0,"drat":3.9,"wt":2.875,"qsec":17.02,"vs":0.0,"am":1.0,"gear":4.0,"carb":4.0})"
  expect_no_error({
    read_ndjson_str(input)
  })
  
  input <- r"({"mpg":21.0,"cyl":6.0,"disp":160.0,"hp":110.0,"drat":3.9,"wt":2.62,"qsec":16.46,"vs":0.0,"am":1.0,"gear":4.0,"carb":4.0})"
  expect_no_error({
    read_ndjson_str(input)
  })
  
  input <- r"(a)"
  expect_error(
    read_ndjson_str(input),
    "Couldn't parse"
  )
  
  
  input <- r"()"
  expect_equal(read_ndjson_raw(input, type =   'df'), data.frame())
  expect_equal(read_ndjson_raw(input, type = 'list'), list())
  
})





test_that("read_ndjson_raw() empty inputs", {
  
  input <- write_ndjson_raw(head(mtcars, 2))
  expect_no_error({
    read_ndjson_raw(input)
  })
  
  
  input <- raw(1)
  expect_error(
    read_ndjson_raw(input),
    "Couldn't parse"
  )
  
  
  input <- raw(0)
  expect_equal(read_ndjson_raw(input, type =   'df'), data.frame())
  expect_equal(read_ndjson_raw(input, type = 'list'), list())
  
})


