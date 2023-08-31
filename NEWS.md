

# yyjsonr 0.1.5  2023-08-31

* Bug fix for checking attributes on list columns.  This surfaced more
  errors to do with setting XYZ geometry type.
* More extensive testing of `to_geojson_str()`

# yyjsonr 0.1.4  2023-08-27

* Initial **alpha** GeoJSON support:
    * `from_geojson_str()`, `from_geojson_file()`
        * Needs more testing for corner cases
    * `to_geojson_str()`, `to_geojson_file()`
        * At a very alpha stage.

# yyjsonr 0.1.3  2023-08-21

* Added `validate_json_file()` and `validate_json_str()`

# yyjsonr 0.1.2  2023-08-20

* Added `to_ndjson_file()` and `to_ndjson_str()`

# yyjsonr 0.1.1  2023-08-19

* Update to option setting to remove overhead
* Added `from_json_raw()` to parse JSON directly from a raw vector (which
  contains UTF-8 string data)

# yyjsonr 0.1.0  2023-08-16

* Initial release
