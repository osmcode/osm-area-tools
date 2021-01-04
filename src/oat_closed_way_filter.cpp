/*****************************************************************************

  OSM Area Tools - Closed way filter

  https://github.com/osmcode/osm-area-tools

*****************************************************************************/

#include <algorithm>
#include <cstdlib>
#include <getopt.h>
#include <iostream>
#include <string>

#include <osmium/io/any_input.hpp>
#include <osmium/io/any_output.hpp>
#include <osmium/io/input_iterator.hpp>
#include <osmium/io/output_iterator.hpp>
#include <osmium/osm/entity_bits.hpp>
#include <osmium/osm/way.hpp>

#include "oat.hpp"

static void print_help() {
    std::cout << "oat_closed_way_filter [OPTIONS] OSMFILE -o OUTPUT\n\n"
              << "Copy closed ways from OSMFILE to OUTPUT.\n\n"
              << "Options:\n"
              << "  -h, --help           - This help message\n"
              << "  -o, --output=OSMFILE - Where to write output\n"
              << "  -O, --overwrite      - Allow overwriting of output file\n"
              ;
}

int main(int argc, char* argv[]) {
    try {
        std::string output_filename;
        auto overwrite = osmium::io::overwrite::no;

        static const struct option long_options[] = {
            {"help",            no_argument, nullptr, 'h'},
            {"output",    required_argument, nullptr, 'o'},
            {"overwrite",       no_argument, nullptr, 'O'},
            {nullptr, 0, nullptr, 0}
        };

        while (true) {
            const int c = getopt_long(argc, argv, "ho:O", long_options, nullptr);
            if (c == -1) {
                break;
            }

            switch (c) {
                case 'h':
                    print_help();
                    return exit_code_ok;
                case 'o':
                    output_filename = optarg;
                    break;
                case 'O':
                    overwrite = osmium::io::overwrite::allow;
                    break;
                default:
                    return exit_code_cmdline_error;
            }
        }

        if (output_filename.empty() || optind != argc - 1) {
            std::cerr << "Usage: " << argv[0] << " [OPTIONS] OSMFILE -o OUTPUT\n";
            return exit_code_cmdline_error;
        }

        const osmium::io::File infile{argv[optind]};

        osmium::io::Reader reader{infile, osmium::osm_entity_bits::way};

        osmium::io::Header header = reader.header();
        header.set("generator", "oat_closed_way_filter");

        osmium::io::Writer writer{output_filename, header, overwrite};
        auto output_it = osmium::io::make_output_iterator(writer);

        const auto ways = osmium::io::make_input_iterator_range<const osmium::Way>(reader);

        std::copy_if(ways.cbegin(), ways.cend(), output_it, [](const osmium::Way& way) {
            return way.is_closed();
        });

        writer.close();
        reader.close();
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return exit_code_error;
    }

    return exit_code_ok;
}

