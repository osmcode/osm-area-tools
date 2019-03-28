/*****************************************************************************

  OSM Area Tools - Problem Report

  https://github.com/osmcode/osm-area-tools

*****************************************************************************/

#include <cstdlib>
#include <getopt.h>
#include <iostream>

#include <gdalcpp.hpp>

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
#include <osmium/index/map/sparse_mem_array.hpp>
#include <osmium/index/map/sparse_mmap_array.hpp>
#include <osmium/index/node_locations_map.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/util/memory.hpp>
#include <osmium/util/verbose_output.hpp>
#include <osmium/visitor.hpp>

#include "oat.hpp"

using index_type = osmium::index::map::Map<osmium::unsigned_object_id_type, osmium::Location>;
using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;

REGISTER_MAP(osmium::unsigned_object_id_type, osmium::Location, osmium::index::map::Dummy, none)

static void print_help() {
    std::cout << "oat_problem_report [OPTIONS] OSMFILE\n\n"
              << "Build multipolygons from OSMFILE and report problems in shapefiles.\n\n"
              << "Options:\n"
              << "  -h, --help                   This help message\n"
              << "  -i, --index=INDEX_TYPE       Set index type for location index (default: flex_mem)\n"
              << "  -I, --show-index-types       Show available index types for location index\n";
}

#ifdef WITH_OLD_STYLE_MP_SUPPORT
using assembler_type = osmium::area::AssemblerLegacy;
using mp_manager_type = osmium::area::MultipolygonManagerLegacy<assembler_type>;
#else
using assembler_type = osmium::area::Assembler;
using mp_manager_type = osmium::area::MultipolygonManager<assembler_type>;
#endif

static osmium::osm_entity_bits::type entity_bits(const std::string& location_index_type) {
    if (location_index_type == "none") {
        return osmium::osm_entity_bits::way;
    } else {
        return osmium::osm_entity_bits::way | osmium::osm_entity_bits::node;
    }
}

int main(int argc, char* argv[]) {
    osmium::util::VerboseOutput vout{true};

    static const struct option long_options[] = {
        {"help",       no_argument,       0, 'h'},
        {"index",      required_argument, 0, 'i'},
        {"show-index", no_argument,       0, 'I'},
        {0, 0, 0, 0}
    };

    std::string database_name{"area_problems"};

    std::string location_index_type{"sparse_mmap_array"};
    const auto& map_factory = osmium::index::MapFactory<osmium::unsigned_object_id_type, osmium::Location>::instance();

    while (true) {
        int c = getopt_long(argc, argv, "hi:I", long_options, 0);
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
    location_handler_type location_handler(*location_index);
    location_handler.ignore_errors(); // XXX

    const osmium::io::File input_file{argv[optind]};

    assembler_type::config_type assembler_config;
    assembler_config.check_roles = true;

    osmium::geom::OGRFactory<> factory;

    gdalcpp::Dataset dataset{"ESRI Shapefile", database_name, gdalcpp::SRS{factory.proj_string()}};
    osmium::area::ProblemReporterOGR problem_reporter{dataset};
    assembler_config.problem_reporter = &problem_reporter;
    mp_manager_type mp_manager{assembler_config};

    vout << "Starting first pass (reading relations)...\n";
    osmium::relations::read_relations(input_file, mp_manager);
    vout << "First pass done.\n";

    vout << "Memory:\n";
    osmium::relations::print_used_memory(vout, mp_manager.used_memory());

    vout << "Starting second pass (reading nodes and ways and assembling areas)...\n";
    osmium::io::Reader reader2{input_file, entity_bits(location_index_type)};

    if (location_index_type == "none") {
        osmium::apply(reader2, mp_manager.handler([](osmium::memory::Buffer&&){}));
    } else {
        osmium::apply(reader2, location_handler, mp_manager.handler([](osmium::memory::Buffer&&){}));
    }

    reader2.close();
    vout << "Second pass done\n";

    osmium::relations::print_used_memory(vout, mp_manager.used_memory());

    vout << "Stats:" << mp_manager.stats() << '\n';

    vout << "Estimated memory usage:\n";
    vout << "  location index: " << (location_index->used_memory() / (1024 * 1024)) << "MB\n";

    osmium::MemoryUsage mcheck;
    vout << "Actual memory usage:\n"
         << "  current: " << mcheck.current() << "MB\n"
         << "  peak:    " << mcheck.peak() << "MB\n";

    vout << "Done.\n";

    return exit_code_ok;
}

