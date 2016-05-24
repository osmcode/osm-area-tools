
# `create_areas`

Assembles areas from their parts and optionally checks them for validity.

## Options

-c, --check
:   Check created multipolygon geometries using the GEOS `IsValid()` function.

-C, --collect-only
:   Only collect the data needed to create the multipolygons but do not
    actually create them. This allows checking how much memory is used for
    collection alone. If this is set the options --check, --only-invalid,
    --output, --overwrite, and --show-incomplete are ignored.

-f, --only-invalid
:   Do not write

-d, --debug[=LEVEL]
:   Set area assembler debug level

-D, --dump[=FILENAME]
:   Dump out the created areas to FILENAME (or `stdout` if no filename was
    given).

-e, --empty-areas
:   Create "empty" areas without rings for multipolygons with broken
    geometries. Without this option they are simply ignored.

-h, --help
:   Show short usage info. All other options are ignored and the program ends
    immediately.

-i, --index=INDEX_TYPE
:   Set index type for location index. See below.

-I, --show-index-types
:   Show available index types for location index. All other options are
    ignored and the program ends immediately.

-o, --output=DBNAME
:   Set the name of the output database. If not set, the multipolygons are
    generated and then discarded.

-O, --overwrite
:   Allow overwriting of existing database. Removes any existing file with
    the name given with the `--output` option. Without this, the program will
    not overwrite an existing database.

-p, --report-problems[=FILENAME]
:   Report problems when building areas to FILENAME (or `stdout` if no filename
    was given). Without this option problems are not reported or, if the
    `--output` option is used, written to the database.

-r, --show-incomplete
:   Show IDs of area relations that could not be completed, because some ways
    were missing in the input file.

-R, --check-roles
:   Tell the assembler to check that the roles on members are correct, ie
    all ways that are outer rings are tagged "outer" and all ways that are
    inner rings are tagged "inner".

-s, --no-new-style
:   Do not output new style multipolygons.

-S, --no-old-style
:   Do not output old style multipolygons.

-t, --keep-type-tag
:   Keep the type tag from multipolygon relations and put it on the assembled
    area. Default is false, the type tag will be removed.

-w, --no-way-polygons
:   Do not output areas created from ways.

-x, --no-areas
:   Do not output any areas at all (same as `-s -S -w`).


## Index types

The following index types are supported:

* `sparse_mmap_array`: Use for small and medium sized extracts (default).
* `sparse_mem_array`: Use for small and medium sized extracts.
* `dense_mmap_array`: Use for very large extracts and planet files.
* `dense_mem_array`: Use for very large extracts and planet files.
* `none`: Not index used. You can use this only if the ways already have the
  node location information stored in them. See [this blog
  post](https://blog.jochentopf.com/2016-04-20-node-locations-on-ways.html)
  for information on how this is done.

The `*_mmap_*` versions are only available on Linux, use them if possible. On
other systems use the `*_mem_*` versions.


## Viewing in QGIS

You can view the result of this program in [QGIS](http://www.qgis.org/) using
the `area-errors.qgs` QGIS project file. You have to name the output
`areas.db` and it has to be in the directory you start QGIS in.


