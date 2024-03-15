

# yyjsonr 0.1.18.9003 2024-03-13

* call `normalizePath()` on all file paths
* clarify authorship and copyright

# yyjsonr 0.1.18.9002 2024-03-07

* Increase maximum number of allowed data.frame columns (during parsing) to 2048

# yyjsonr 0.1.18.9001 2024-02-01

* Read JSON from '.gz' files in `read_json_file()`

# yyjsonr 0.1.18.9000 2024-01-25

* New `fast_numerics` flag when writing.
    * Default `FALSE`
    * If `TRUE` the user is guaranteeing that there are no NA, NaN or Inf values
      in the numeric and integer vectors, and thus a faster method for writing
      these vectors to JSON can be used.
* Changed writing of `raw` vectors to always use the `fast_numerics` method,
  as raw R vectors (by definition) will not have NA, NaN or Inf values.

# yyjsonr 0.1.18 2024-01-22

* Fixes for CRAN
    * Adjust pointer arithmetic when calling `output_verbose_error()` to
      avoid overflow of `size_t`

# yyjsonr 0.1.17 2024-01-20

* Fixes for CRAN
    * Fixed warnings when building with `MAKEVARS` `PKG_CFLAGS = -Wconversion` 
    * Fixed C error where raw buffer was passed to `strlen()` leading to 
      a check failure on cran with clang-ASAN

# yyjsonr 0.1.16 2024-01-17

* Fixes for CRAN
    * DESCRIPTION fix: Write `C` as `'C'`
    * Add link to original `yyjson` library
    * Fix name in LICENSE

# yyjsonr 0.1.15 2024-01-15

* Fixes for CRAN
    * Simplify example to remove `checkRd` NOTE
    * Platform specific handling of error location format string to fix WARNING

# yyjsonr 0.1.14 2024-01-13

* Add `int64 = "double"` option to `opts_read_json()`
* Preparations for CRAN

# yyjsonr 0.1.13 2024-01-05

* Remove NDJSON and GeoJSON code to simplify preparation for CRAN.  Will 
  re-introduce this code in future releases.
* Updated to YYJSON v0.8.0

# yyjsonr 0.1.12 2023-10-29

* Fix an off-by-one error when reporting line numbers in NDJSON handling.
* Increase buffer size when reading lines from NDJSON files.
    * MAX_LINE_LENGTH now 131072 (was 10000)

# yyjsonr 0.1.11 2023-10-27

* Writing to JSON objects now supports a `digits` argument for rounding floating 
  point values to the specified number of significant digits
    * `digits = -1` means don't do any rounding
    * `digits = 0` rounds floating point values to integers (and writes the 
      values as JSON integers)

# yyjsonr 0.1.10 2023-09-14

* Refactored options for simplification to data.frame
    * removed `vectors_to_df`
    * replaced with `obj_of_arrs_to_df`
    * added `arr_of_obs_to_df`
* length-1 vectors marked with class `AsIs` (using a call to `I()`) will
  never be unboxed i.e. will always be serialized as a JSON []-array with 
  one element.
* Added parse option `length1_array_asis`.  If `TRUE` then automatically add
  the class `AsIs` to the object object.

# yyjsonr 0.1.9 2023-09-13

* Added pre-calculation and caching of data.frame column types for faster
  serialization when outputting row-by-row.

# yyjsonr 0.1.8 2023-09-12

* Added `path.expand()` when handling filenames. Thanks to
    * https://github.com/shikokuchuo
    * https://github.com/hrbrmstr
* Added support for data.frames without column names to match 
  behaviour of `jsonlite` (when working within the `plotly` package)

# yyjsonr 0.1.7 2023-09-10

* Unifed naming scheme

# yyjsonr 0.1.6 2023-09-04

* FEATURE: Added `promote_num_to_string` in `opts_read_json()` to enable 
  forced promotion of numerics to string
* BUGFIX: fixes for handling of geometry collection when reading and writing.
* TESTING: More tests included for output to geojson
* TESTING: Refactored testing of 'sf' objects

# yyjsonr 0.1.5  2023-08-31

* Bug fix for checking attributes on list columns.  This surfaced more
  errors to do with setting XYZ geometry type.
* More extensive testing of `write_geojson_str()`

# yyjsonr 0.1.4  2023-08-27

* Initial **alpha** GeoJSON support:
    * `read_geojson_str()`, `read_geojson_file()`
        * Needs more testing for corner cases
    * `write_geojson_str()`, `write_geojson_file()`
        * At a very alpha stage.

# yyjsonr 0.1.3  2023-08-21

* Added `validate_json_file()` and `validate_json_str()`

# yyjsonr 0.1.2  2023-08-20

* Added `write_ndjson_file()` and `write_ndjson_str()`

# yyjsonr 0.1.1  2023-08-19

* Update to option setting to remove overhead
* Added `read_json_raw()` to parse JSON directly from a raw vector (which
  contains UTF-8 string data)

# yyjsonr 0.1.0  2023-08-16

* Initial release
