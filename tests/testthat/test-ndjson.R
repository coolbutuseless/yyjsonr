

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





#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Write unnamed data frame rows as JSON arrays
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
test_that("write_ndjson_str with unnamed df produces arrays", {
  df <- data.frame(a = 1:3, b = c("x", "y", "z"), stringsAsFactors = FALSE)

  # Named: should produce objects
  json_named <- write_ndjson_str(df)
  expect_true(grepl("^\\{", strsplit(json_named, "\n")[[1]][1]))

  # Unnamed: should produce arrays
  json_unnamed <- write_ndjson_str(unname(df))
  lines <- strsplit(json_unnamed, "\n")[[1]]
  expect_true(grepl("^\\[", lines[1]))
  expect_equal(lines[1], '[1,"x"]')
  expect_equal(lines[2], '[2,"y"]')
  expect_equal(lines[3], '[3,"z"]')
})

test_that("write_ndjson_file with unnamed df produces arrays", {
  df <- data.frame(a = 1.5, b = TRUE, stringsAsFactors = FALSE)
  tmp <- tempfile()
  write_ndjson_file(unname(df), tmp)
  lines <- readLines(tmp)
  expect_equal(lines[1], '[1.5,true]')
})

test_that("write_ndjson_str and write_ndjson_file agree for unnamed df", {
  df <- unname(data.frame(a = 1:2, b = c("x", "y"), stringsAsFactors = FALSE))
  tmp <- tempfile()
  write_ndjson_file(df, tmp)
  from_file <- paste(readLines(tmp), collapse = "\n")
  from_str <- write_ndjson_str(df)
  expect_identical(from_file, from_str)
})

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Read array-format NDJSON lines as data frame
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
test_that("read_ndjson_str handles array-format lines as df", {
  input <- '[1,"a",true]\n[2,"b",false]\n[3,"c",true]'
  result <- read_ndjson_str(input, type = 'df')
  expect_equal(ncol(result), 3)
  expect_equal(nrow(result), 3)
  expect_equal(names(result), c("V1", "V2", "V3"))
  expect_equal(result$V1, c(1L, 2L, 3L))
  expect_equal(result$V2, c("a", "b", "c"))
  expect_equal(result$V3, c(TRUE, FALSE, TRUE))
})

test_that("read_ndjson_str handles array-format with col_names", {
  input <- '[1,"a"]\n[2,"b"]'
  result <- read_ndjson_str(input, type = 'df', col_names = c("id", "val"))
  expect_equal(names(result), c("id", "val"))
  expect_equal(result$id, c(1L, 2L))
  expect_equal(result$val, c("a", "b"))
})

test_that("read_ndjson_file handles array-format lines", {
  df <- data.frame(a = 1:3, b = c("x", "y", "z"), stringsAsFactors = FALSE)
  tmp <- tempfile()
  write_ndjson_file(unname(df), tmp)
  result <- read_ndjson_file(tmp, col_names = c("a", "b"))
  expect_equal(result$a, 1:3)
  expect_equal(result$b, c("x", "y", "z"))
})

test_that("read_ndjson_str with nskip and array-format", {
  input <- '{"meta":"header"}\n[1,2]\n[3,4]'
  result <- read_ndjson_str(input, type = 'df', nskip = 1, col_names = c("x", "y"))
  expect_equal(nrow(result), 2)
  expect_equal(result$x, c(1L, 3L))
  expect_equal(result$y, c(2L, 4L))
})

test_that("col_names length mismatch errors", {
  input <- '[1,2,3]\n[4,5,6]'
  expect_error(
    read_ndjson_str(input, type = 'df', col_names = c("a", "b")),
    "col_names length"
  )
})

test_that("roundtrip: write unnamed df as arrays, read back", {
  df <- data.frame(
    x = c(1.1, 2.2, 3.3),
    y = c("a", "b", "c"),
    z = c(TRUE, FALSE, TRUE),
    stringsAsFactors = FALSE
  )

  # Write as arrays
  json <- write_ndjson_str(unname(df))

  # Read back with original column names
  result <- read_ndjson_str(json, col_names = names(df))

  expect_equal(result$x, df$x)
  expect_equal(result$y, df$y)
  expect_equal(result$z, df$z)
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


