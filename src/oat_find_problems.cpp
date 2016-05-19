/*****************************************************************************

  OSM Area Tools - Find problems

  https://github.com/osmcode/osm-area-tools

*****************************************************************************/

#include <iostream>

#include <getopt.h>

#include <osmium/handler.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/io/any_output.hpp>
#include <osmium/util/memory.hpp>

#include "oat.hpp"

bool check_relation(const osmium::Relation& relation, char mptype, int& error_count) {
    bool okay = true;

    for (const auto& member : relation.members()) {

        // relations of type=multipolygon should not have anything but way members
        if (member.type() != osmium::item_type::way && mptype == 'm') {
            std::cout << 'r' << relation.id() << " non-way member " << osmium::item_type_to_char(member.type()) << member.ref() << " (role='" << member.role() << "')\n";
            ++error_count;
            okay = false;
        }

        // way members should have role "outer" or "inner" or be empty
        if (member.type() == osmium::item_type::way) {
            const char* r = member.role();
            if (*r != '\0' && std::strcmp(r, "outer") && std::strcmp(r, "inner")) {
                std::cout << 'r' << relation.id() << " wrong role '" << r << "'\n";
                ++error_count;
                okay = false;
            }
        }
    }

    return okay;
}

void print_help() {
    std::cout << "oat_find_problems [OPTIONS] OSMFILE\n\n"
              << "Find problems in area relations in OSMFILE.\n\n"
              << "Options:\n"
              << "  -h, --help                  This help message\n"
              << "  -f, --output-format=FORMAT  Format of output file\n"
              << "  -o, --output=FILE           Output file\n"
              ;
}

char mp_type(const char* type) {
    char mptype = ' ';

    if (!std::strcmp(type, "multipolygon")) {
        mptype = 'm';
    } else if (!std::strcmp(type, "boundary")) {
        mptype = 'b';
    }

    return mptype;
}

int main(int argc, char* argv[]) {
    static const struct option long_options[] = {
        {"help",          no_argument,       0, 'h'},
        {"output-format", required_argument, 0, 'f'},
        {"output",        required_argument, 0, 'o'},
        {0, 0, 0, 0}
    };

    std::string output;
    std::string output_format;
    while (true) {
        int c = getopt_long(argc, argv, "hf:o:", long_options, 0);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 'h':
                print_help();
                exit(exit_code_ok);
            case 'f':
                output_format = optarg;
                break;
            case 'o':
                output = optarg;
                break;
            default:
                exit(exit_code_cmdline_error);
        }
    }

    int remaining_args = argc - optind;
    if (remaining_args != 1) {
        std::cerr << "Usage: " << argv[0] << " [OPTIONS] OSMFILE\n";
        exit(exit_code_cmdline_error);
    }


    const osmium::io::File output_file{output, output_format};
    osmium::io::Writer writer{output_file};

    osmium::io::File input_file{argv[optind]};
    osmium::io::Reader reader(input_file, osmium::osm_entity_bits::relation);

    int error_count = 0;
    while (const osmium::memory::Buffer buffer = reader.read()) {
        for (auto it = buffer.begin<osmium::Relation>(); it != buffer.end<osmium::Relation>(); ++it) {
            const char* type = it->tags().get_value_by_key("type");
            if (type) {
                char mptype = mp_type(type);
                if (mptype != ' ' && !check_relation(*it, mptype, error_count)) {
                    writer(*it);
                }
            }
        }
    }
    reader.close();

    writer.close();

    osmium::MemoryUsage mcheck;
    if (mcheck.peak()) {
        std::cerr << "Peak memory usage: " << mcheck.peak() << "MB\n";
    }

    std::cerr << "Found " << error_count << " errors\n";

    return error_count > 0 ? exit_code_error : exit_code_ok;
}

