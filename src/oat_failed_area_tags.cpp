/*****************************************************************************

  OSM Area Tools - Failed Area Tags

  https://github.com/osmcode/osm-area-tools

*****************************************************************************/

#include <cstdlib>
#include <getopt.h>
#include <iostream>

//#define WITH_OLD_STYLE_MP_SUPPORT

#ifdef WITH_OLD_STYLE_MP_SUPPORT
# include <osmium/area/assembler_legacy.hpp>
# include <osmium/area/multipolygon_manager_legacy.hpp>
#else
# include <osmium/area/assembler.hpp>
# include <osmium/area/multipolygon_manager.hpp>
#endif

#include <osmium/area/problem_reporter_ogr.hpp>
#include <osmium/geom/ogr.hpp>
#include <osmium/index/map/dense_mem_array.hpp>
#include <osmium/index/map/dense_mmap_array.hpp>
#include <osmium/index/map/dummy.hpp>
#include <osmium/index/map/flex_mem.hpp>
#include <osmium/index/map/sparse_mem_array.hpp>
#include <osmium/index/map/sparse_mmap_array.hpp>
#include <osmium/index/node_locations_map.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/io/any_output.hpp>
#include <osmium/util/memory.hpp>
#include <osmium/util/verbose_output.hpp>
#include <osmium/visitor.hpp>

#include "oat.hpp"

using index_type = osmium::index::map::Map<osmium::unsigned_object_id_type, osmium::Location>;
using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;

REGISTER_MAP(osmium::unsigned_object_id_type, osmium::Location, osmium::index::map::Dummy, none)

void print_help() {
    std::cout << "oat_failed_area_tags [OPTIONS] OSMFILE\n\n"
              << "Build areas from OSMFILE and count tags where area assembly failed.\n\n"
              << "Options:\n"
              << "  -h, --help                   This help message\n"
              << "  -i, --index=INDEX_TYPE       Set index type for location index (default: flex_mem)\n"
              << "  -I, --show-index-types       Show available index types for location index\n"
              ;
}

#ifdef WITH_OLD_STYLE_MP_SUPPORT
using assembler_type = osmium::area::AssemblerLegacy;
using mp_manager_type = osmium::area::MultipolygonManagerLegacy<assembler_type>;
#else
using assembler_type = osmium::area::Assembler;
using mp_manager_type = osmium::area::MultipolygonManager<assembler_type>;
#endif

osmium::osm_entity_bits::type entity_bits(const std::string& location_index_type) {
    if (location_index_type == "none") {
        return osmium::osm_entity_bits::way;
    } else {
        return osmium::osm_entity_bits::way | osmium::osm_entity_bits::node;
    }
}

struct tag_counter {
    std::size_t amenity  = 0;
    std::size_t boundary = 0;
    std::size_t building = 0;
    std::size_t landuse  = 0;
    std::size_t leisure  = 0;
    std::size_t natural  = 0;
    std::size_t place    = 0;
    std::size_t sport    = 0;
    std::size_t waterway = 0;

    std::size_t unknown  = 0;

    std::size_t name     = 0;
};

int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);

    static const struct option long_options[] = {
        {"help",       no_argument,       nullptr, 'h'},
        {"index",      required_argument, nullptr, 'i'},
        {"show-index", no_argument,       nullptr, 'I'},
        {nullptr, 0, nullptr, 0}
    };

    std::string location_index_type{"flex_mem"};
    const auto& map_factory = osmium::index::MapFactory<osmium::unsigned_object_id_type, osmium::Location>::instance();

    while (true) {
        int c = getopt_long(argc, argv, "hi:I", long_options, nullptr);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 'h':
                print_help();
                std::exit(exit_code_ok);
            case 'i':
                location_index_type = optarg;
                break;
            case 'I':
                std::cout << "Available index types:\n";
                for (const auto& map_type : map_factory.map_types()) {
                    std::cout << "  " << map_type;
                    if (map_type == location_index_type) {
                        std::cout << " (default)";
                    }
                    std::cout << '\n';
                }
                std::exit(exit_code_ok);
            default:
                std::exit(exit_code_cmdline_error);
        }
    }

    const int remaining_args = argc - optind;
    if (remaining_args != 1) {
        std::cerr << "Usage: " << argv[0] << " [OPTIONS] OSMFILE\n";
        std::exit(exit_code_cmdline_error);
    }

    auto location_index = map_factory.create_map(location_index_type);
    location_handler_type location_handler{*location_index};
    location_handler.ignore_errors(); // XXX

    const osmium::io::File input_file{argv[optind]};

    assembler_type::config_type assembler_config;
    assembler_config.create_empty_areas = true;

    mp_manager_type mp_manager{assembler_config};

    osmium::relations::read_relations(input_file, mp_manager);

    osmium::io::Reader reader2{input_file, entity_bits(location_index_type)};

    tag_counter counter;

    auto mp_manager_handler = mp_manager.handler([&counter](osmium::memory::Buffer&& buffer){
        for (const auto& area : buffer.select<osmium::Area>()) {
            if (area.num_rings().first == 0) {
                if (area.tags().has_key("name")) {
                    ++counter.name;
                }
                if (area.tags().has_key("building")) {
                    ++counter.building;
                } else if (area.tags().has_key("landuse")) {
                    ++counter.landuse;
                } else if (area.tags().has_key("natural")) {
                    ++counter.natural;
                } else if (area.tags().has_key("amenity")) {
                    ++counter.amenity;
                } else if (area.tags().has_key("boundary")) {
                    ++counter.boundary;
                } else if (area.tags().has_key("sport")) {
                    ++counter.sport;
                } else if (area.tags().has_key("leisure")) {
                    ++counter.leisure;
                } else if (area.tags().has_key("place")) {
                    ++counter.place;
                } else if (area.tags().has_key("waterway")) {
                    ++counter.waterway;
                } else {
                    ++counter.unknown;
                    for (const osmium::Tag& tag : area.tags()) {
                        std::cerr << area.id() << ' ' << tag.key() << ' ' << tag.value() << '\n';
                    }
                }
            }
        }
    });

    if (location_index_type == "none") {
        osmium::apply(reader2, mp_manager_handler);
    } else {
        osmium::apply(reader2, location_handler, mp_manager_handler);
    }

    reader2.close();

    std::cout << "amenity:   " << counter.amenity  << '\n';
    std::cout << "boundary:  " << counter.boundary << '\n';
    std::cout << "building:  " << counter.building << '\n';
    std::cout << "landuse:   " << counter.landuse  << '\n';
    std::cout << "leisure:   " << counter.leisure  << '\n';
    std::cout << "natural:   " << counter.natural  << '\n';
    std::cout << "place:     " << counter.place    << '\n';
    std::cout << "sport:     " << counter.sport    << '\n';
    std::cout << "waterway:  " << counter.waterway << '\n';

    std::cout << "unknown:   " << counter.unknown << '\n';

    std::cout << "with name: " << counter.name << '\n';

    return exit_code_ok;
}

