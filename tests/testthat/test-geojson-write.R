

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' dput(head(sf::st_read(system.file("shape/nc.shp", package="sf")), 2))
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
nc2 <- structure(
  list(
    AREA = c(0.114, 0.061), 
    PERIMETER = c(1.442, 1.231
    ), 
    CNTY_ = c(1825, 1827), 
    CNTY_ID = c(1825, 1827), 
    NAME = c("Ashe", "Alleghany"), 
    FIPS = c("37009", "37005"), 
    FIPSNO = c(37009, 37005), 
    CRESS_ID = c(5L, 3L), 
    BIR74 = c(1091, 487), 
    SID74 = c(1, 0), 
    NWBIR74 = c(10, 10), 
    BIR79 = c(1364, 542), 
    SID79 = c(0, 3), 
    NWBIR79 = c(19, 12), 
    geometry = structure(list(structure(list(
      list(structure(c(
        -81.4727554321289, -81.5408401489258, 
        -81.5619812011719, -81.6330642700195, -81.7410736083984, 
        -81.6982803344727, -81.7027969360352, -81.6699981689453, 
        -81.3452987670898, -81.347541809082, -81.3247756958008, 
        -81.3133239746094, -81.2662353515625, -81.2628402709961, 
        -81.2406921386719, -81.2398910522461, -81.2642440795898, 
        -81.3289947509766, -81.3613739013672, -81.3656921386719, 
        -81.354133605957, -81.3674545288086, -81.4063873291016, 
        -81.4123306274414, -81.431037902832, -81.4528884887695, 
        -81.4727554321289, 36.2343559265137, 36.2725067138672, 
        36.2735939025879, 36.3406867980957, 36.3917846679688, 
        36.4717788696289, 36.5193405151367, 36.5896492004395, 
        36.5728645324707, 36.537914276123, 36.5136795043945, 
        36.4806976318359, 36.4372062683105, 36.4050407409668, 
        36.3794174194336, 36.365364074707, 36.3524131774902, 
        36.3635025024414, 36.3531608581543, 36.3390502929688, 
        36.2997169494629, 36.2786979675293, 36.2850532531738, 
        36.2672920227051, 36.2607192993164, 36.2395858764648, 
        36.2343559265137), dim = c(27L, 2L)))), 
      class = c("XY", "MULTIPOLYGON", "sfg")), 
      structure(list(list(structure(
        c(-81.2398910522461, 
          -81.2406921386719, -81.2628402709961, -81.2662353515625, 
          -81.3133239746094, -81.3247756958008, -81.347541809082, -81.3452987670898, 
          -80.9034423828125, -80.9335479736328, -80.9657745361328, 
          -80.9496688842773, -80.9563903808594, -80.9779510498047, 
          -80.9828414916992, -81.0027770996094, -81.0246429443359, 
          -81.0428009033203, -81.0842514038086, -81.0985641479492, 
          -81.1133117675781, -81.1293792724609, -81.1383972167969, 
          -81.1533660888672, -81.1766738891602, -81.2398910522461, 
          36.365364074707, 36.3794174194336, 36.4050407409668, 36.4372062683105, 
          36.4806976318359, 36.5136795043945, 36.537914276123, 36.5728645324707, 
          36.5652122497559, 36.4983139038086, 36.4672203063965, 36.4147338867188, 
          36.4037971496582, 36.3913764953613, 36.3718338012695, 36.3666801452637, 
          36.3778343200684, 36.4103355407715, 36.4299201965332, 36.43115234375, 
          36.4228515625, 36.4263305664062, 36.4176254272461, 36.4247398376465, 
          36.4154434204102, 36.365364074707), dim = c(26L, 2L)))), 
        class = c("XY", "MULTIPOLYGON", "sfg"))), 
      class = c("sfc_MULTIPOLYGON", "sfc"), 
      precision = 0, 
      bbox = structure(c(
        xmin = -81.7410736083984, 
        ymin = 36.2343559265137, 
        xmax = -80.9034423828125, 
        ymax = 36.5896492004395
      ), 
      class = "bbox"
      ), 
      crs = structure(list(
        input = "NAD27", 
        wkt = "GEOGCRS[\"NAD27\",\n    DATUM[\"North American Datum 1927\",\n        ELLIPSOID[\"Clarke 1866\",6378206.4,294.978698213898,\n            LENGTHUNIT[\"metre\",1]]],\n    PRIMEM[\"Greenwich\",0,\n        ANGLEUNIT[\"degree\",0.0174532925199433]],\n    CS[ellipsoidal,2],\n        AXIS[\"latitude\",north,\n            ORDER[1],\n            ANGLEUNIT[\"degree\",0.0174532925199433]],\n        AXIS[\"longitude\",east,\n            ORDER[2],\n            ANGLEUNIT[\"degree\",0.0174532925199433]],\n    ID[\"EPSG\",4267]]"), class = "crs"), n_empty = 0L)), 
  sf_column = "geometry", 
  agr = structure(c(
    AREA = NA_integer_, 
    PERIMETER = NA_integer_, CNTY_ = NA_integer_, CNTY_ID = NA_integer_, 
    NAME = NA_integer_, FIPS = NA_integer_, FIPSNO = NA_integer_, 
    CRESS_ID = NA_integer_, BIR74 = NA_integer_, SID74 = NA_integer_, 
    NWBIR74 = NA_integer_, BIR79 = NA_integer_, SID79 = NA_integer_, 
    NWBIR79 = NA_integer_), levels = c("constant", "aggregate", "identity"
    ), class = "factor"), row.names = 1:2, class = c("sf", "data.frame"
    ))



test_that("Basic sf-to-geojson works", {
  
  geojson_str <- to_geojson_str(nc2)
  nc2_test <- from_geojson_str(geojson_str)  
  
  expect_equal(
    nc2_test,
    nc2,
    ignore_attr = TRUE
  )
  
  tmp <- tempfile()
  to_geojson_file(nc2, tmp)
  nc2_test2 <- from_geojson_file(tmp)  
  expect_equal(
    nc2_test2,
    nc2,
    ignore_attr = TRUE
  )  
  
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






# if (FALSE) {
#   library(sf)
#   library(geojsonsf)
#   
#   nc <- st_read(system.file("shape/nc.shp", package="sf"))
#   nc2 <- head(nc, 2)
#   ref <- sf_geojson(nc2)
#   ref <- sfc_geojson(nc2$geometry)
#   
#   yy <- to_geojson_str(nc$geometry)
#   zz <- sfc_geojson(nc$geometry)  
#   
#   
#   yy <- to_geojson_str(nc2)
#   zz <- sf_geojson(nc2)  
#   cdiff(yy, zz)
#   
#   
#   bench::mark(
#     to_json_str(nc$geometry),
#     to_geojson_str(nc$geometry),
#     sfc_geojson(nc$geometry),
#     check = FALSE
#   )
#   
#   
#   bench::mark(
#     to_json_str(nc),
#     to_geojson_str(nc),
#     sf_geojson(nc),
#     check = FALSE
#   )
#   
# }



