

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Parse an Dataset-NDJSON file to a data.frame
#' 
#' This is quite a strict parser and a colspec \emph{must} be given and match 
#' the data stream.
#' 
#' @inheritParams read_json_str
#' @param nskip Number of records to skip before starting to read. Default: 0 
#'        (skip no data)
#' @param colspec data.frame of column spec. Must have a row for each data item
#'        expected in the dataset.  Must include a 'name' and 'dataType' column
#'
#' 
#' @family JSON Parsers
#' @return Dataset-NDJSON data read into R as data.frame 
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
read_dataset_ndjson_str <- function(str, nskip = 1, colspec = NULL, opts = list(), ...) {
  
    .Call(
      parse_dataset_ndjson_str_as_df_,
      str, 
      colspec,
      nskip,
      modify_list(opts, list(...))
    )
}


if (FALSE) {
  dsjs <- 
    r"({"datasetJSONCreationDateTime": "2023-06-28T15:38:43", "datasetJSONVersion": "1.1.0", "fileOID": "www.sponsor.xyz.org.project123.final", "dbLastModifiedDateTime": "2023-05-31T00:00:00", "originator": "Sponsor XYZ", "sourceSystem": {"name": "Software ABC", "version": "1.0.0"}, "studyOID": "cdisc.com.CDISCPILOT01", "metaDataVersionOID": "MDV.MSGv2.0.SDTMIG.3.3.SDTM.1.7", "metaDataRef": "https://metadata.location.org/CDISCPILOT01/define.xml", "itemGroupOID": "IG.DM", "records": 18, "name": "DM", "label": "Demographics", "columns": [{"itemOID": "IT.DM.STUDYID", "name": "STUDYID", "label": "Study Identifier", "dataType": "string", "length": 12, "keySequence": 1}, {"itemOID": "IT.DM.DOMAIN", "name": "DOMAIN", "label": "Domain Abbreviation", "dataType": "string", "length": 2},  {"itemOID": "IT.DM.USUBJID", "name": "USUBJID", "label": "Unique Subject Identifier", "dataType": "string", "length": 8, "keySequence": 2}, {"itemOID": "IT.DM.AGE", "name": "AGE", "label": "Age", "dataType": "integer"}, {"itemOID": "IT.DM.AGEU", "name": "AGEU", "label": "Age Units", "dataType": "string", "length": 5}]}
["CDISCPILOT01", "DM", "CDISC001", 84, "YEARS"]
["CDISCPILOT01", "DM", "CDISC002", 76, "YEARS"]
["CDISCPILOT01", "DM", "CDISC003", 61, "YEARS"])"
  
  zz <- read_json_str(dsjs, yyjson_read_flag = yyjson_read_flag$YYJSON_READ_STOP_WHEN_DONE)
  zz$columns
  read_dataset_ndjson_str(dsjs, colspec = zz$columns)
  
  
  read_ndjson_str(dsjs, nread = 1)
  
  read_ndjson_str(dsjs, nskip = 1, type = 'list')
  
  
}


