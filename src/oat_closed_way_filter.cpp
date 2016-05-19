/*****************************************************************************

  OSM Area Tools - Closed way filter

  https://github.com/osmcode/osm-area-tools

*****************************************************************************/

#include <algorithm>
#include <getopt.h>
#include <iostream>
#include <string>

#include <osmium/io/any_input.hpp>
#include <osmium/io/any_output.hpp>
#include <osmium/io/input_iterator.hpp>
#include <osmium/io/output_iterator.hpp>
#include <osmium/osm/entity_bits.hpp>
#include <osmium/osm/way.hpp>

void print_help() {
    std::cout << "oat_closed_way_filter [OPTIONS] OSMFILE\n"
              << "\nOptions:\n"
              << "  -h, --help           - This help message\n"
              << "  -o, --output=OSMFILE - Where to write output\n"
              << "  -O, --overwrite      - Allow overwriting of output file\n"
              ;
}

int main(int argc, char* argv[]) {
    std::string output_filename;
    auto overwrite = osmium::io::overwrite::no;

    static struct option long_options[] = {
        {"help",            no_argument, 0, 'h'},
        {"output",    required_argument, 0, 'o'},
        {"overwrite",       no_argument, 0, 'O'},
        {0, 0, 0, 0}
    };

    while (1) {
        int c = getopt_long(argc, argv, "ho:O", long_options, 0);
        if (c == -1)
            break;

        switch (c) {
            case 'h':
                print_help();
                exit(0);
            case 'o':
                output_filename = optarg;
                break;
            case 'O':
                overwrite = osmium::io::overwrite::allow;
                break;
            default:
                exit(2);
        }
    }

    if (output_filename.empty()) {
        std::cerr << "Missing -o/--output=OSMFILE option\n";
        exit(2);
    }

    if (optind != argc - 1) {
        std::cerr << "Usage: oat_closed_way_filter [OPTIONS] OSMFILE\n";
        exit(2);
    }

    const osmium::io::File infile(argv[optind]);

    osmium::io::Reader reader(infile, osmium::osm_entity_bits::way);

    osmium::io::Header header = reader.header();
    header.set("generator", "oat_closed_way_filter");

    osmium::io::Writer writer(output_filename, header, overwrite);
    auto output_it = osmium::io::make_output_iterator(writer);

    auto ways = osmium::io::make_input_iterator_range<const osmium::Way>(reader);

    std::copy_if(ways.cbegin(), ways.cend(), output_it, [](const osmium::Way& way) {
        return way.is_closed();
    });

    writer.close();
    reader.close();

    return 0;
}

