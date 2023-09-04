
modify_list <- function(old, new) {
  
  # print(names(old))
  # print(names(new))
  
  if (is.null(names(new))) {
    return(old)
  }
  for (nm in names(new)) old[[nm]] <- new[[nm]]
  old
}

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' 
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
rearrange_list <- function(x) {
  if (is.null(x)) {
    return(NULL)
  }

  att <- attributes(x)

  # re-arrange members of list
  x <- lapply(x, rearrange)

  if (!is.null(att)) {
    # cat("L\n")
    attributes(x) <- modify_list(att, attributes(x))
  }
  
  x
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' 
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
rearrange_df <- function(x) {
  # cat("D")
  expect_true(inherits(x, 'data.frame'))

  att <- attributes(x)
  att$names <- NULL

  x <- x[,sort(colnames(x))]

  x <- lapply(x, rearrange)

  attributes(x) <- modify_list(att, attributes(x))

  x
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Rearrange an object such that all its name, and attributes are 
# in alphabetical order
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
rearrange <- function(x) {
  
  if (is.null(x)) {
    # do nothin
  } else if (is.data.frame(x)) {
    x <- rearrange_df(x)
  } else if (is.list(x)) {
    x <- rearrange_list(x)
  } else {
    att <- attributes(x)
    if (!is.null(att)) {
      # att <- rearrange(att)
      # cat(".\n")
      attributes(x) <- modify_list(att, attributes(x))
    }
  }
  
  x
}

# rearrange(mtcars)


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Set load path depending on whether debugging or testing within package
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
if (testthat::is_testing()) {
  examples_dir <- "./geojson"
} else {
  # manually running during development
  examples_dir <- "tests/testthat/geojson"
}

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Determine list of JSON files to test
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
json_files <- list.files(examples_dir, pattern = "json$", full.names = TRUE)

json <- lapply(json_files, function(x) {
  paste(readLines(x), collapse = "\n")
})
names(json) <- basename(tools::file_path_sans_ext(json_files))

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Generate reference objects using geojson
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
if (FALSE) {
  # json_files <- list.files(examples_dir, pattern = "json$", full.names = TRUE)
  # 
  # ref <- list(sf = list(), sfc = list())
  # 
  # prep <- function(json_file) {
  #   nm <- basename(json_file)
  #   ref$sf [[nm]] <<- geojsonsf::geojson_sf (json_file)
  #   ref$sfc[[nm]] <<- geojsonsf::geojson_sfc(json_file)
  # }
  # 
  # for (json_file in json_files) {
  #   prep(json_file)
  # }
  # 
  # saveRDS(ref, "tests/testthat/geojson/ref.rds", compress = 'xz')
}

ref <- readRDS(file.path(examples_dir, "ref.rds"))


expect_equal_when_attributes_sorted <- function(tst, ref, label = NULL) {
  tst <- rearrange(tst)
  ref <- rearrange(ref)
  expect_equal(tst, ref, label = label)
}


expect_identical_when_attributes_sorted <- function(tst, ref, label = NULL) {
  tst <- rearrange(tst)
  ref <- rearrange(ref)
  expect_identical(tst, ref, label = label)
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' 
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
test_that("ensure helper 'expect_equal_when_attributes_sorted()' works", {
  mtcars2 <- mtcars[, sample(colnames(mtcars))]
  expect_equal_when_attributes_sorted(mtcars2, mtcars)
  
  
  attributes(mtcars2$cyl) <- list(a = 1)
  
  expect_error({  
    expect_equal_when_attributes_sorted(mtcars2, mtcars)
  }, "absent")
  
  mtcars3 <- mtcars
  attributes(mtcars3$cyl) <- list(a = 1)
  expect_equal_when_attributes_sorted(mtcars3, mtcars2)
  
  expect_error({  
    expect_identical_when_attributes_sorted(mtcars2, mtcars)
  }, "absent")
  
  mtcars3 <- mtcars
  attributes(mtcars3$cyl) <- list(a = 1)
  expect_identical_when_attributes_sorted(mtcars3, mtcars2)
  
  
  df <- data.frame(a = 1:2)
  df$b <- list(x = 1, y = 2)
  attributes(df$b[[1]]) <- list(x = 10)
  
  df1 <- df
  expect_equal_when_attributes_sorted(df1, df)

  
  attributes(df1$b[[1]]) <- list(x = 10, y = 11)
  expect_error(
    expect_equal_when_attributes_sorted(df1, df),
    "expected\\$b\\$x.*?absent"
  )

})


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' 
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
test_that("Compare input to geojsonsf", {
  
  json_file <- "tests/testthat/geojson/standard-example.json"
  
  for (json_file in json_files) {
    
    nm <- basename(json_file)
    # cat(nm, "\n")

    sf <- from_geojson_file(json_file, type = 'sf')
    if (nm == 'standard-example.json') {
      # When geojsonsf promotos double 0.0 to string, it makes it '0'
      # whereas yyjsonr makes it "0.000000"
      sf$prop1 <- gsub("0.000000", "0", sf$prop1)
    }
    # expect_equal(sf, ref$sf[[nm]], label = nm)
    expect_equal_when_attributes_sorted(sf, ref$sf[[nm]], label = nm)
    # expect_identical_when_attributes_sorted(sf, ref$sf[[nm]], label = nm)
    
    
    sfc <- from_geojson_file(json_file, type = 'sfc')
    expect_equal_when_attributes_sorted(sfc, ref$sfc[[nm]], label = nm)
    # expect_identical_when_attributes_sorted(sfc, ref$sfc[[nm]], label = nm)
  }
  
})


# if (FALSE) {
#   js <- json$`coord-class-multipolygon-xyzm`
#   cat(js)
#   
#   tst <- from_geojson_str(js, type = 'sfc')
#   ref <- geojsonsf::geojson_sfc(js)
#   expect_same_contents(tst, ref)
# 
#   tst <- from_geojson_str(js)
#   ref <- geojsonsf::geojson_sf(js)
#   expect_same_contents(tst, ref)
# }


# if (FALSE) {
#   bench::mark(
#     geojsonsf::geojson_sf(geojsonsf::geo_melbourne),
#     from_geojson_str     (geojsonsf::geo_melbourne),
#     check = FALSE
#   )
# }
# 
# 
# if (FALSE) {
#   library(sf)
#   library(microbenchmark)
#   
#   nc = st_read(system.file("shape/nc.shp", package="sf"))
#   
#   dst_gjsf = tempfile(fileext = ".geojson")
#   dst_gjyy = tempfile(fileext = ".geojson")
#   
#   bench::mark(
#     sfgj = cat(geojsonsf::sf_geojson(nc), file = dst_gjsf),
#     yygj = yyjsonr::to_json_file(nc, filename = dst_gjyy)
#   )
# }

 







