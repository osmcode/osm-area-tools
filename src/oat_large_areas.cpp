/*****************************************************************************

  OSM Area Tools - Large areas

  https://github.com/osmcode/osm-area-tools

*****************************************************************************/

#include <cstdlib>
#include <getopt.h>
#include <iostream>
#include <unordered_map>
#include <utility>

#include <sqlite.hpp>

#include <osmium/handler.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/io/pbf_output.hpp>
#include <osmium/util/memory.hpp>
#include <osmium/visitor.hpp>

#include "oat.hpp"

class LargeAreasHandler : public osmium::handler::Handler {

    std::unordered_map<uint32_t, uint32_t> m_nodes_in_ways;

    osmium::io::Writer& m_writer;

    Sqlite::Statement& m_insert_into_areas;

    std::size_t m_min_ways;
    std::size_t m_min_nodes;

    static std::pair<const char*, const char*> get_subtype(const osmium::TagList& tags) {
        for (const osmium::Tag& tag : tags) {
            if (!std::strcmp(tag.key(), "boundary")  ||
                !std::strcmp(tag.key(), "land_area") ||
                !std::strcmp(tag.key(), "landuse")   ||
                !std::strcmp(tag.key(), "leisure")   ||
                !std::strcmp(tag.key(), "natural")   ||
                !std::strcmp(tag.key(), "place")     ||
                !std::strcmp(tag.key(), "waterway")) {
                return std::make_pair(tag.key(), tag.value());
            }
        }
        return std::make_pair("", "");
    }

public:

    LargeAreasHandler(osmium::io::Writer& writer, Sqlite::Statement& insert_into_areas, std::size_t min_ways, std::size_t min_nodes) :
        m_writer(writer),
        m_insert_into_areas(insert_into_areas),
        m_min_ways(min_ways),
        m_min_nodes(min_nodes) {
    }

    void way(const osmium::Way& way) {
        m_nodes_in_ways[uint32_t(way.id())] = uint32_t(way.nodes().size());
    }

    void relation(const osmium::Relation& relation) {
        const char* type = relation.tags().get_value_by_key("type");
        if (!type) {
            return;
        }
        if (!std::strcmp(type, "multipolygon") || !std::strcmp(type, "boundary")) {
            std::size_t num_ways = 0;
            std::size_t num_nodes = 0;
            for (const auto& member : relation.members()) {
                if (member.type() == osmium::item_type::way) {
                    ++num_ways;
                    num_nodes += m_nodes_in_ways[member.ref()];
                }
            }

            if (num_ways >= m_min_ways || num_nodes >= m_min_nodes) {
                m_writer(relation);

                auto subtype = get_subtype(relation.tags());
                const char* name = relation.tags().get_value_by_key("name");
                const char* name_en = relation.tags().get_value_by_key("name:en");

                m_insert_into_areas.bind_int(relation.id());
                m_insert_into_areas.bind_int(num_ways);
                m_insert_into_areas.bind_int(num_nodes);
                m_insert_into_areas.bind_int(relation.tags().size());
                m_insert_into_areas.bind_text(type);
                m_insert_into_areas.bind_text(subtype.first);
                m_insert_into_areas.bind_text(subtype.second);
                m_insert_into_areas.bind_text(name ? name : "");
                m_insert_into_areas.bind_text(name_en ? name_en : "");
                m_insert_into_areas.execute();
            }
        }
    }

}; // class LargeAreasHandler

static void print_help() {
    std::cout << "oat_large_areas [OPTIONS] OSMFILE\n\n"
              << "Find largest area relations in OSMFILE.\n\n"
              << "Options:\n"
              << "  -h, --help           This help message\n"
              << "  -n, --min-nodes=NUM  Minimum number of nodes (default: 100000)\n"
              << "  -o, --output=FILE    File name prefix for output files (default: 'large_areas')\n"
              << "  -w, --min-ways=NUM   Minimum number of ways (default: 1000)\n"
              ;
}

int main(int argc, char* argv[]) {
    static const struct option long_options[] = {
        {"help",      no_argument,       nullptr, 'h'},
        {"min-nodes", required_argument, nullptr, 'n'},
        {"min-ways",  required_argument, nullptr, 'w'},
        {"output",    required_argument, nullptr, 'o'},
        {nullptr, 0, nullptr, 0}
    };

    std::size_t min_ways = 1000;
    std::size_t min_nodes = 100000;
    std::string output{"large_areas"};
    while (true) {
        int c = getopt_long(argc, argv, "hn:o:w:", long_options, nullptr);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 'h':
                print_help();
                std::exit(exit_code_ok);
            case 'n':
                min_nodes = std::atoi(optarg);
                break;
            case 'w':
                min_ways = std::atoi(optarg);
                break;
            case 'o':
                output = optarg;
                break;
            default:
                std::exit(exit_code_cmdline_error);
        }
    }

    const int remaining_args = argc - optind;
    if (remaining_args != 1) {
        std::cerr << "Usage: " << argv[0] << " [OPTIONS] OSMFILE\n";
        std::exit(exit_code_cmdline_error);
    }

    osmium::io::Writer writer{output + ".osm.pbf"};

    Sqlite::Database db{output + ".db", SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE}; // NOLINT(hicpp-signed-bitwise)
    db.exec("CREATE TABLE areas (relation_id INTEGER, num_ways INTEGER, num_nodes INTEGER, num_tags INTEGER, type VARCHAR, key VARCHAR, value VARCHAR, name VARCHAR, name_en VARCHAR);");
    Sqlite::Statement insert_into_areas{db, "INSERT INTO areas (relation_id, num_ways, num_nodes, num_tags, type, key, value, name, name_en) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)"};

    LargeAreasHandler handler{writer, insert_into_areas, min_ways, min_nodes};

    const osmium::io::File infile{argv[optind]};
    osmium::io::Reader reader{infile, osmium::osm_entity_bits::way | osmium::osm_entity_bits::relation};
    osmium::apply(reader, handler);
    reader.close();

    writer.close();

    osmium::MemoryUsage mcheck;
    std::cerr << "Actual memory usage:\n"
              << "  current: " << mcheck.current() << "MB\n"
              << "  peak:    " << mcheck.peak() << "MB\n";

    return exit_code_ok;
}

