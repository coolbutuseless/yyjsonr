

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
#' Re-introduce these tests after basics are working
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# json$`geometry-collection` <- NULL
# json$`standard-example`    <- NULL
# json[grepl("proper", names(json))] <- NULL

# json <- json['geometry-collection']
# i <- 1

test_that("Basic writing works for lots of different geojson", {
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  #' Using 'yyjsonr', round-trip some JSON 
  #' - from geojson string to R 'sf' object 
  #' - from R 'sf' object to geojson string
  #' - from geojson string to R 'sf' object
  #' 
  #' Then assert that the two 'sf' objects are equal 
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  for (i in seq_along(json)) {
    # cat(names(json)[i], "\n")
    # flush.console()
    
    # Take the JSON
    js1 <- json[[i]]
    
    # Load it into R as 'sf'
    geojson1 <- from_geojson_str(js1)

    # Write it out to JSON str
    js2 <- to_geojson_str(geojson1)
    
    # Re-load it into R as 'sf'
    geojson2 <- from_geojson_str(js2)
    
    # Test 'sf' representations are equal
    expect_identical(geojson1, geojson2, label = names(json)[i])
  }
})


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
#' Re-introduce these tests after basics are working
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
json$`geometry-collection` <- NULL
json$`standard-example`    <- NULL
json[grepl("proper", names(json))] <- NULL

# json <- json['coord-class-linestring-xyz']
# i <- 1

test_that("Basic writing works for lots of different geojson", {
  
  for (i in seq_along(json)) {
    # cat(names(json)[i], "\n")
    flush.console()
    js1 <- json[[i]]
    
    # geojson0 <- geojsonsf::geojson_sf(js1)
    geojson1 <- from_geojson_str(js1)

    js2 <- to_geojson_str(geojson1)
    geojson2 <- from_geojson_str(js2)
    
    # expect_equal(geojson1, geojson2, label = names(json)[i], ignore_attr = TRUE)
    expect_equal(geojson1, geojson2, label = names(json)[i], ignore_attr = FALSE)
  }
})







js1 <- r"(
{
    "type": "GeometryCollection",
    "geometries": [
        {
            "type": "Point",
            "coordinates": [
                -80.660805,
                35.049392
            ]
        },
        {
            "type": "Polygon",
            "coordinates": [
                [
                    [
                        -80.664582,
                        35.044965
                    ],
                    [
                        -80.663874,
                        35.04428
                    ],
                    [
                        -80.662586,
                        35.04558
                    ],
                    [
                        -80.663444,
                        35.046036
                    ],
                    [
                        -80.664582,
                        35.044965
                    ]
                ]
            ]
        },
        {
            "type": "LineString",
            "coordinates": [
                [
                    -80.662372,
                    35.059509
                ],
                [
                    -80.662693,
                    35.059263
                ],
                [
                    -80.662844,
                    35.05893
                ]
            ]
        }
    ]
} 
)"

test_that("geom collection isolated test", {
  # Load it into R as 'sf'
  geojson1 <- from_geojson_str(js1)

  # Write it out to JSON str
  js2 <- to_geojson_str(geojson1)
  # cat(js2)

  # Re-load it into R as 'sf'
  geojson2 <- from_geojson_str(js2)

  # Test 'sf' representations are equal
  expect_equal(geojson1, geojson2)
})

