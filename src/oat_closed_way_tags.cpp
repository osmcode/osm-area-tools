/*****************************************************************************

  OSM Area Tools - Closed way tags

  https://github.com/osmcode/osm-area-tools

*****************************************************************************/

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <iostream>
#include <memory>
#include <string>

#include <osmium/io/any_input.hpp>
#include <osmium/io/any_output.hpp>
#include <osmium/io/input_iterator.hpp>
#include <osmium/io/output_iterator.hpp>
#include <osmium/osm/entity_bits.hpp>
#include <osmium/osm/tag.hpp>
#include <osmium/osm/way.hpp>
#include <osmium/tags/filter.hpp>
#include <osmium/tags/taglist.hpp>

#include "oat.hpp"

void print_help() {
    std::cout << "oat_closed_way_tags [OPTIONS] OSMFILE\n\n"
              << "Split up closed ways in OSMFILE according to polygon/non-polygon tags.\n\n"
              << "Options:\n"
              << "  -h, --help                 - This help message\n"
              << "  -o, --output-prefix=PREFIX - Prefix for output files\n"
              << "  -O, --overwrite            - Allow overwriting of output files\n"
              ;
}

enum category {
    unknown    = 0,
    notags     = 1,
    linestring = 2,
    polygon    = 3,
    both       = 4
};

static osmium::tags::KeyValueFilter linestring_tags{false};
static osmium::tags::KeyValueFilter polygon_tags{false};
static osmium::tags::KeyFilter uninteresting_tags{true};

static category classify(const osmium::TagList& tags) {
    if (tags.empty()) {
        return category::notags;
    }

    osmium::tags::KeyFilter::iterator fi_begin(uninteresting_tags, tags.cbegin(), tags.cend());
    osmium::tags::KeyFilter::iterator fi_end(uninteresting_tags, tags.cend(), tags.cend());
    if (std::distance(fi_begin, fi_end) == 0) {
        return category::notags;
    }

    const char* area = tags.get_value_by_key("area");
    if (area) {
        if (!std::strcmp(area, "yes")) {
            return category::polygon;
        } else if (!std::strcmp(area, "no")) {
            return category::linestring;
        }
    }

    const bool any_l = osmium::tags::match_any_of(tags, linestring_tags);
    const bool any_p = osmium::tags::match_any_of(tags, polygon_tags);

    if (any_l) {
        if (any_p) {
            return category::both;
        }
        return category::linestring;
    }

    if (any_p) {
        return category::polygon;
    }

    return category::unknown;
}

static void init_taglists() {
    linestring_tags.add(true, "highway");
    linestring_tags.add(true, "leisure", "slipway");
    linestring_tags.add(false, "waterway", "riverbank");
    linestring_tags.add(false, "waterway", "dock");
    linestring_tags.add(true, "waterway");

    polygon_tags.add(true, "building");
    polygon_tags.add(true, "building:part");
    polygon_tags.add(true, "landuse");
    polygon_tags.add(true, "natural");
    polygon_tags.add(true, "waterway", "dock");
    polygon_tags.add(true, "waterway", "riverbank");
    polygon_tags.add(false, "leisure", "slipway");
    polygon_tags.add(true, "leisure");
    polygon_tags.add(true, "amenity", "parking");
    polygon_tags.add(true, "amenity", "bicycle_parking");
    polygon_tags.add(true, "aeroway", "apron");

    uninteresting_tags.add(false, "created_by");
    uninteresting_tags.add(false, "source");
    uninteresting_tags.add(false, "note");
    uninteresting_tags.add(false, "name");
}

int main(int argc, char* argv[]) {
    init_taglists();

    std::string output_prefix{"close-way-tags"};
    auto overwrite = osmium::io::overwrite::no;

    static const struct option long_options[] = {
        {"help",                no_argument, nullptr, 'h'},
        {"output-prefix", required_argument, nullptr, 'o'},
        {"overwrite",           no_argument, nullptr, 'O'},
        {nullptr, 0, nullptr, 0}
    };

    while (1) {
        int c = getopt_long(argc, argv, "ho:O", long_options, nullptr);
        if (c == -1)
            break;

        switch (c) {
            case 'h':
                print_help();
                std::exit(exit_code_ok);
            case 'o':
                output_prefix = optarg;
                break;
            case 'O':
                overwrite = osmium::io::overwrite::allow;
                break;
            default:
                std::exit(exit_code_cmdline_error);
        }
    }

    if (optind != argc - 1) {
        std::cerr << "Usage: " << argv[0] << " [OPTIONS] OSMFILE\n";
        std::exit(exit_code_cmdline_error);
    }

    const osmium::io::File infile{argv[optind]};

    osmium::io::Reader reader{infile, osmium::osm_entity_bits::way};
    osmium::io::Header header = reader.header();
    header.set("generator", "oat_closed_way_tags");

    std::array<uint32_t, 5> counter = {{0, 0, 0, 0, 0}};
    std::array<std::unique_ptr<osmium::io::Writer>, 5> writers {{
        std::unique_ptr<osmium::io::Writer>(new osmium::io::Writer{output_prefix + "-unknown.osm.pbf",    header, overwrite}),
        std::unique_ptr<osmium::io::Writer>(new osmium::io::Writer{output_prefix + "-notags.osm.pbf",     header, overwrite}),
        std::unique_ptr<osmium::io::Writer>(new osmium::io::Writer{output_prefix + "-linestring.osm.pbf", header, overwrite}),
        std::unique_ptr<osmium::io::Writer>(new osmium::io::Writer{output_prefix + "-polygon.osm.pbf",    header, overwrite}),
        std::unique_ptr<osmium::io::Writer>(new osmium::io::Writer{output_prefix + "-both.osm.pbf",       header, overwrite})
    }};

    const auto ways = osmium::io::make_input_iterator_range<const osmium::Way>(reader);

    for (const osmium::Way& way : ways) {
        if (way.is_closed()) {
            const auto way_category = classify(way.tags());
            ++counter[way_category];
            (*writers[way_category])(way);
        }
    }

    for (auto& writer : writers) {
        writer->close();
    }

    reader.close();

    std::cout << "unknown:    " << counter[category::unknown] << '\n';
    std::cout << "no tags:    " << counter[category::notags] << '\n';
    std::cout << "linestring: " << counter[category::linestring] << '\n';
    std::cout << "polygon:    " << counter[category::polygon] << '\n';
    std::cout << "both:       " << counter[category::both] << '\n';

    return exit_code_ok;
}

