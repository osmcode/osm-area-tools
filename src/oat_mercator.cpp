/*****************************************************************************

  OSM Area Tools - Build areas and project to Mercator

  https://github.com/osmcode/osm-area-tools

*****************************************************************************/

#include "oat.hpp"

#include <osmium/area/assembler.hpp>
#include <osmium/area/multipolygon_manager.hpp>
#include <osmium/area/problem_reporter_ogr.hpp>
#include <osmium/geom/mercator_projection.hpp>
#include <osmium/geom/ogr.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/index/map/dense_mem_array.hpp>
#include <osmium/index/map/dense_mmap_array.hpp>
#include <osmium/index/map/dummy.hpp>
#include <osmium/index/map/flex_mem.hpp>
#include <osmium/index/map/sparse_mem_array.hpp>
#include <osmium/index/map/sparse_mmap_array.hpp>
#include <osmium/index/node_locations_map.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/util/memory.hpp>
#include <osmium/util/verbose_output.hpp>
#include <osmium/visitor.hpp>

#include <gdalcpp.hpp>

#include <cstdlib>
#include <getopt.h>
#include <iostream>

using index_type = osmium::index::map::Map<osmium::unsigned_object_id_type, osmium::Location>;
using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;
using proj_type = osmium::geom::MercatorProjection;
using factory_type = osmium::geom::OGRFactory<proj_type>;

REGISTER_MAP(osmium::unsigned_object_id_type, osmium::Location, osmium::index::map::Dummy, none)

class OutputOGR : public osmium::handler::Handler {

    factory_type& m_factory;

    gdalcpp::Layer m_layer_multipolygons;

    bool m_only_invalid = false;

    static void print_area_error(const osmium::Area& area, const osmium::geometry_error& e) {
        std::cerr << "Ignoring illegal geometry for area "
                  << area.id()
                  << " created from "
                  << (area.from_way() ? "way" : "relation")
                  << " with id="
                  << area.orig_id() << " (" << e.what() << ").\n";
    }

public:

    OutputOGR(gdalcpp::Dataset& dataset, factory_type& factory) :
        m_factory(factory),
        m_layer_multipolygons(dataset, "areas", wkbMultiPolygon, {"SPATIAL_INDEX=NO"}) {
        m_layer_multipolygons.add_field("id", OFTInteger, 10);
        m_layer_multipolygons.add_field("valid", OFTInteger, 1);
        m_layer_multipolygons.add_field("source", OFTString, 1);
        m_layer_multipolygons.add_field("orig_id", OFTInteger, 10);
    }

    void set_only_invalid(bool only_invalid) noexcept {
        m_only_invalid = only_invalid;
    }

    void area(const osmium::Area& area) {
        try {
            auto geom = m_factory.create_multipolygon(area);
            bool const is_valid = geom->IsValid();

            if (m_only_invalid && is_valid) {
                return;
            }

            gdalcpp::Feature feature{m_layer_multipolygons, std::move(geom)};
            feature.set_field("id", static_cast<int32_t>(area.id()));
            feature.set_field("valid", is_valid);
            feature.set_field("source", area.from_way() ? "w" : "r");
            feature.set_field("orig_id", static_cast<int32_t>(area.orig_id()));
            feature.add_to_layer();
        } catch (const osmium::geometry_error& e) {
            print_area_error(area, e);
        }
    }

}; // class OutputOGR


void print_help() {
    std::cout << "oat_mercator [OPTIONS] OSMFILE\n\n"
              << "Read OSMFILE, build multipolygons from it and project to Mercator.\n"
              << "\nOptions:\n"
              << "  -d, --debug[=LEVEL]     Set area assembler debug level\n"
              << "  -f, --only-invalid      Filter out valid geometries\n"
              << "  -h, --help              This help message\n"
              << "  -i, --index=INDEX_TYPE  Set index type for location index (default: flex_mem)\n"
              << "  -I, --show-index-types  Show available index types for location index\n"
              << "  -o, --output=DBNAME     Database name\n"
              << "  -O, --overwrite         Overwrite existing database\n"
              << "  -p, --report-problems   Report problems to database\n"
              ;
}

using assembler_type = osmium::area::Assembler;
using mp_manager_type = osmium::area::MultipolygonManager<assembler_type>;

int main(int argc, char* argv[]) {
    try {
        osmium::util::VerboseOutput vout{true};

        static const struct option long_options[] = {
            {"debug",           optional_argument, nullptr, 'd'},
            {"only-invalid",    no_argument,       nullptr, 'f'},
            {"help",            no_argument,       nullptr, 'h'},
            {"index",           required_argument, nullptr, 'i'},
            {"show-index",      no_argument,       nullptr, 'I'},
            {"output",          required_argument, nullptr, 'o'},
            {"overwrite",       no_argument,       nullptr, 'O'},
            {"report-problems", no_argument,       nullptr, 'p'},
            {nullptr, 0, nullptr, 0}
        };

        std::string database_name;

        std::string location_index_type{"flex_mem"};
        const auto& map_factory = osmium::index::MapFactory<osmium::unsigned_object_id_type, osmium::Location>::instance();

        bool overwrite = false;
        bool report_problems = false;
        bool only_invalid = false;

        assembler_type::config_type assembler_config;
        assembler_config.create_empty_areas = false;

        while (true) {
            const int c = getopt_long(argc, argv, "d::fhi:Io:Op", long_options, nullptr);
            if (c == -1) {
                break;
            }

            switch (c) {
                case 'd':
                    assembler_config.debug_level = optarg ? std::atoi(optarg) : 1;
                    break;
                case 'f':
                    only_invalid = true;
                    break;
                case 'h':
                    print_help();
                    return exit_code_ok;
                case 'i':
                    location_index_type = optarg;
                    break;
                case 'I':
                    show_index_types();
                    return exit_code_ok;
                case 'o':
                    database_name = optarg;
                    break;
                case 'O':
                    overwrite = true;
                    break;
                case 'p':
                    report_problems = true;
                    break;
                default:
                    return exit_code_cmdline_error;
            }
        }

        const int remaining_args = argc - optind;
        if (remaining_args != 1) {
            std::cerr << "Usage: " << argv[0] << " [OPTIONS] OSMFILE\n";
            return exit_code_cmdline_error;
        }

        auto location_index = map_factory.create_map(location_index_type);
        location_handler_type location_handler{*location_index};
        location_handler.ignore_errors(); // XXX

        const osmium::io::File input_file{argv[optind]};

        const bool need_locations = location_index_type != "none";

        if (overwrite) {
            unlink(database_name.c_str());
        }

        CPLSetConfigOption("OGR_SQLITE_SYNCHRONOUS", "OFF");
        factory_type factory;

        gdalcpp::Dataset dataset{"SQLite", database_name, gdalcpp::SRS{factory.proj_string()}, { "SPATIALITE=TRUE", "INIT_WITH_EPSG=NO" }};
        dataset.enable_auto_transactions();

        dataset.exec("PRAGMA journal_mode = OFF;");

        OutputOGR output{dataset, factory};
        output.set_only_invalid(only_invalid);

        std::unique_ptr<osmium::area::ProblemReporterOGR> reporter;

        if (report_problems) {
            reporter = std::make_unique<osmium::area::ProblemReporterOGR>(dataset);
        }

        assembler_config.problem_reporter = reporter.get();
        mp_manager_type mp_manager{assembler_config};

        vout << "Starting first pass (reading relations)...\n";
        osmium::relations::read_relations(input_file, mp_manager);
        vout << "First pass done.\n";

        vout << "Memory:\n";
        osmium::relations::print_used_memory(vout, mp_manager.used_memory());

        vout << "Starting second pass (reading nodes and ways and assembling areas)...\n";
        osmium::io::Reader reader{input_file, entity_bits(location_index_type)};

        if (need_locations) {
            osmium::apply(reader, location_handler, mp_manager.handler([&output](osmium::memory::Buffer&& buffer) {
                osmium::apply(buffer, output);
            }));
        } else {
            osmium::apply(reader, mp_manager.handler([&output](osmium::memory::Buffer&& buffer) {
                osmium::apply(buffer, output);
            }));
        }

        reader.close();
        vout << "Second pass done\n";

        reporter.reset();
        osmium::relations::print_used_memory(vout, mp_manager.used_memory());

        vout << "Stats:" << mp_manager.stats() << '\n';

        vout << "Estimated memory usage:\n";
        vout << "  location index: " << (location_index->used_memory() / 1024) << "kB\n";

        osmium::MemoryUsage mcheck;
        vout << "Actual memory usage:\n"
            << "  current: " << mcheck.current() << "MB\n"
            << "  peak:    " << mcheck.peak() << "MB\n";

        vout << "Done.\n";

    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return exit_code_error;
    }

    return exit_code_ok;
}

