
# OSM Area Tools

OpenStreetMap doesn't have anything like an *Area*, *Polygon* or *MultiPolygon*
basic data type. Instead, areas are modelled as closed ways or using a relation
tagged with `type=multipolygon` or `type=boundary`. This makes the data hard to
edit and use, but it is the way it is.

This repository contains several programs based on the
[libosmium](https://github.com/osmcode/libsomium) library that help with
understanding what areas there are in OSM by creating statistics or other
information from the OSM data. Some programs assemble areas from the
nodes, ways, and relations.

The programs in this repository are not meant to be used by end users, they are
for developers or mappers who want to understand what's in the OSM data and
maybe fix (some of) it.


## Naming Conventions

Osmium/OSM Area Tools uses the following naming conventions:

* Area: A Polygon or MultiPolygon with tags created from a closed way or
  an Area Relation.
* Area Relation: Any kind of relation that can be turned into an area,
  a Multipolygon Relation or a Boundary Relation.
* Multipolygon Relation: A relation tagged `type=multipolygon`.
* Boundary Relation: A relation tagged `type=boundary`.
* Assembling: The process of creating an Area from the basic OSM objects.
* Polygon: A type of geometry (as defined by the Simple Feature Definition)
  with one outer ring and zero or more inner rings.
* MultiPolygon: A collection of polygons (with one or more outer rings and
  zero or more inner rings).


## Programs

[![Build Status](https://github.com/osmcode/osm-area-tools/workflows/CI/badge.svg?branch=master)](https://github.com/osmcode/osm-area-tools/actions)


### `oat_closed_way_filter`

Copy only closed ways from input file to output file.

### `oat_create_areas`

Assembles areas from their parts and optionally checks them for validity. Can
write the areas to a Spatialite database including all the problems encountered
on the way.

### `oat_failed_area_tags`

Creates areas but only looks at those areas which could not be built due to
geometry problems. Creates some tag statistics on stdout.

### `oat_find_problems`

Find problematic data with respect to area creation. This program does not try
to actually build the areas, it just looks at the data and flags some obvious
problems.

### `oat_large_areas`

Look at the largest area relations in the input OSM file in terms of the number
of ways or nodes they contain. Creates a Sqlite database with information about
those relations and an OSM file containing those relations.

### `oat_problem_report`

Create areas and report all problems encountered into shapefiles. The areas
themselves are not kept.

### `oat_sizes`

Prints sizes of some C++ structures used in assembling areas from their parts.
This is only interesting for C++ developers optimizing the code.

### `oat_stats`

Create some statistics from an OSM file related to areas. This program will
not actually assemble the areas, just look at the objects potentially making
up the areas. The results are stored in an Sqlite database.


## Prerequisites

You need a C++11 compliant compiler. GCC 4.8 and later as well as clang 3.4 and
later are known to work. You also need the following libraries:

    Osmium Library
        Need at least version 2.12.2
        http://osmcode.org/libosmium

    Protozero
        Need at least version 1.5.1
        https://github.com/mapbox/protozero
        Debian/Ubuntu: libprotozero-dev

    Utfcpp
        http://utfcpp.sourceforge.net/
        Debian/Ubuntu: libutfcpp-dev
        Also included in the libosmium repository.

    gdalcpp
        https://github.com/joto/gdalcpp
        Also included in the libosmium repository.

    bz2lib (for reading and writing bzipped files)
        http://www.bzip.org/
        Debian/Ubuntu: libbz2-dev

    CMake (for building)
        http://www.cmake.org/
        Debian/Ubuntu: cmake

    Expat (for parsing XML files)
        http://expat.sourceforge.net/
        Debian/Ubuntu: libexpat1-dev
        openSUSE: libexpat-devel

    GDAL/OGR
        http://gdal.org/
        Debian/Ubuntu: libgdal-dev

    GEOS
        http://trac.osgeo.org/geos/
        Debian/Ubuntu: libgeos++-dev

    Sqlite
        http://sqlite.org/
        Debian/Ubuntu: libsqlite3-dev

    zlib (for PBF support)
        http://www.zlib.net/
        Debian/Ubuntu: zlib1g-dev
        openSUSE: zlib-devel


## Building

Build like any CMake program by creating a build directory and running `cmake`
and then `make` as follows:

    mkdir build
    cd build
    cmake ..
    make

To set the build type call cmake with `-DCMAKE_BUILD_TYPE=type`. Possible
values are empty, Debug, Release, RelWithDebInfo, MinSizeRel, and Dev. The
defaults is RelWithDebInfo.


## License

Copyright (C) 2016-2019  Jochen Topf <jochen@topf.org>

This program is available under the GNU GENERAL PUBLIC LICENSE Version 3.
See the file LICENSE.txt for the complete text of the license.


## Authors

This program was written and is maintained by Jochen Topf <jochen@topf.org>.


