
dsjs <- 
  r"({"datasetJSONCreationDateTime": "2023-06-28T15:38:43", "datasetJSONVersion": "1.1.0", "fileOID": "www.sponsor.xyz.org.project123.final", "dbLastModifiedDateTime": "2023-05-31T00:00:00", "originator": "Sponsor XYZ", "sourceSystem": {"name": "Software ABC", "version": "1.0.0"}, "studyOID": "cdisc.com.CDISCPILOT01", "metaDataVersionOID": "MDV.MSGv2.0.SDTMIG.3.3.SDTM.1.7", "metaDataRef": "https://metadata.location.org/CDISCPILOT01/define.xml", "itemGroupOID": "IG.DM", "records": 18, "name": "DM", "label": "Demographics", "columns": [{"itemOID": "IT.DM.STUDYID", "name": "STUDYID", "label": "Study Identifier", "dataType": "string", "length": 12, "keySequence": 1}, {"itemOID": "IT.DM.DOMAIN", "name": "DOMAIN", "label": "Domain Abbreviation", "dataType": "string", "length": 2},  {"itemOID": "IT.DM.USUBJID", "name": "USUBJID", "label": "Unique Subject Identifier", "dataType": "string", "length": 8, "keySequence": 2}, {"itemOID": "IT.DM.AGE", "name": "AGE", "label": "Age", "dataType": "integer"}, {"itemOID": "IT.DM.AGEU", "name": "AGEU", "label": "Age Units", "dataType": "string", "length": 5}]}
["CDISCPILOT01", "DM", "CDISC001", 84, "YEARS"]
["CDISCPILOT01", "DM", "CDISC002", 76, "YEARS"]
["CDISCPILOT01", "DM", "CDISC003", 61, "YEARS"])"

dsjs_raw <- dsjs |> utf8ToInt() |> as.raw()


test_that("parse dataset ndjson string works", {
  
  zz <- read_json_str(dsjs, yyjson_read_flag = yyjson_read_flag$YYJSON_READ_STOP_WHEN_DONE)
  zz$columns
  
  res <- read_dataset_ndjson_str(dsjs, colspec = zz$columns)
  expect_true(is.data.frame(res))
  
  expect_identical(colnames(res), c('STUDYID', 'DOMAIN', 'USUBJID', 'AGE', 'AGEU'))
  expect_equal(nrow(res), 3)
  expect_true(is.character(res[[1]]))
  expect_true(is.character(res[[2]]))
  expect_true(is.character(res[[3]]))
  expect_true(is.character(res[[5]]))
  
  expect_true(is.integer(res[[4]]))
  
})

test_that("parse dataset ndjson raw works", {
  
  zz <- read_json_str(dsjs, yyjson_read_flag = yyjson_read_flag$YYJSON_READ_STOP_WHEN_DONE)
  zz$columns
  
  res <- read_dataset_ndjson_raw(dsjs_raw, colspec = zz$columns)
  expect_true(is.data.frame(res))
  
  expect_identical(colnames(res), c('STUDYID', 'DOMAIN', 'USUBJID', 'AGE', 'AGEU'))
  expect_equal(nrow(res), 3)
  expect_true(is.character(res[[1]]))
  expect_true(is.character(res[[2]]))
  expect_true(is.character(res[[3]]))
  expect_true(is.character(res[[5]]))
  
  expect_true(is.integer(res[[4]]))
  
})



test_that("parse dataset ndjson from file works", {
  
  f <- testthat::test_path("dataset-ndjson/ae.ndjson")
  zz <- read_json_file(f, yyjson_read_flag = yyjson_read_flag$YYJSON_READ_STOP_WHEN_DONE)
  zz$columns
  res <- read_dataset_ndjson_file(f, colspec = zz$columns)
  expect_true(is.data.frame(res))
  expect_identical(dim(res), c(74L, 37L))
  expect_true(all(res$DOMAIN == 'AE'))
  
})


test_that("parse dataset ndjson from file works", {
  
  f <- testthat::test_path("dataset-ndjson/ae.ndjson")
  zz <- read_json_file(f, yyjson_read_flag = yyjson_read_flag$YYJSON_READ_STOP_WHEN_DONE)
  zz$columns
  res <- read_dataset_ndjson_file(f, colspec = zz$columns)
  expect_true(is.data.frame(res))
  expect_identical(dim(res), c(74L, 37L))
  expect_true(all(res$DOMAIN == 'AE'))
  
})

