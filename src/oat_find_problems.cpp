/*****************************************************************************

  OSM Area Tools - Find problems

  https://github.com/osmcode/osm-area-tools

*****************************************************************************/

#include "oat.hpp"

#include <osmium/io/any_input.hpp>
#include <osmium/io/any_output.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/osm/entity_bits.hpp>
#include <osmium/osm/item_type.hpp>
#include <osmium/osm/relation.hpp>
#include <osmium/util/memory.hpp>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <iostream>
#include <string>
#include <vector>

static bool check_relation(const osmium::Relation& relation, char mptype, int& error_count) {
    bool okay = true;
    std::vector<osmium::object_id_type> ids;
    ids.reserve(relation.members().size());

    for (const auto& member : relation.members()) {
        if (member.type() == osmium::item_type::way) {
            ids.push_back(member.ref());
        }

        // relations of type=multipolygon should not have anything but way members
        if (member.type() != osmium::item_type::way && mptype == 'm') {
            std::cout << 'r' << relation.id() << " non-way member " << osmium::item_type_to_char(member.type()) << member.ref() << " (role='" << member.role() << "')\n";
            ++error_count;
            okay = false;
        }

        // way members should have role "outer" or "inner" or be empty
        if (member.type() == osmium::item_type::way) {
            const char* r = member.role();
            if (*r != '\0' && (std::strcmp(r, "outer") != 0) && (std::strcmp(r, "inner") != 0)) {
                std::cout << 'r' << relation.id() << " wrong role '" << r << "'\n";
                ++error_count;
                okay = false;
            }
        }
    }

    std::sort(ids.begin(), ids.end());

    auto it = ids.cbegin();
    while ((it = std::adjacent_find(it, ids.cend())) != ids.cend()) {
        okay = false;
        std::cout << 'r' << relation.id() << " has duplicate member way " << *it << '\n';
        ++it;
        ++it;
    }

    return okay;
}

static void print_help() {
    std::cout << "oat_find_problems [OPTIONS] OSMFILE\n\n"
              << "Find problems in area relations in OSMFILE.\n\n"
              << "Options:\n"
              << "  -h, --help                  This help message\n"
              << "  -f, --output-format=FORMAT  Format of output file\n"
              << "  -o, --output=FILE           Output file\n"
              ;
}

static char mp_type(const char* type) {
    char mptype = ' ';

    if (!std::strcmp(type, "multipolygon")) {
        mptype = 'm';
    } else if (!std::strcmp(type, "boundary")) {
        mptype = 'b';
    }

    return mptype;
}

int main(int argc, char* argv[]) {
    int error_count = 0;

    try {
        static const struct option long_options[] = {
            {"help",          no_argument,       nullptr, 'h'},
            {"output-format", required_argument, nullptr, 'f'},
            {"output",        required_argument, nullptr, 'o'},
            {nullptr, 0, nullptr, 0}
        };

        std::string output;
        std::string output_format;
        while (true) {
            const int c = getopt_long(argc, argv, "hf:o:", long_options, nullptr);
            if (c == -1) {
                break;
            }

            switch (c) {
                case 'h':
                    print_help();
                    std::exit(exit_code_ok);
                case 'f':
                    output_format = optarg;
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


        const osmium::io::File output_file{output, output_format};
        osmium::io::Writer writer{output_file};

        const osmium::io::File input_file{argv[optind]};
        osmium::io::Reader reader{input_file, osmium::osm_entity_bits::relation};

        while (const osmium::memory::Buffer buffer = reader.read()) {
            for (const auto &relation : buffer.select<osmium::Relation>()) {
                const char* type = relation.tags().get_value_by_key("type");
                if (type) {
                    const char mptype = mp_type(type);
                    if (mptype != ' ' && !check_relation(relation, mptype, error_count)) {
                        writer(relation);
                    }
                }
            }
        }
        reader.close();

        writer.close();

    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return exit_code_error;
    }

    const osmium::MemoryUsage mcheck;
    if (mcheck.peak()) {
        std::cerr << "Peak memory usage: " << mcheck.peak() << "MB\n";
    }

    std::cerr << "Found " << error_count << " errors\n";

    return error_count > 0 ? exit_code_error : exit_code_ok;
}

