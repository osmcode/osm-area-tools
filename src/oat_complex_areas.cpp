/*****************************************************************************

  OSM Area Tools - Complex areas

  https://github.com/osmcode/osm-area-tools

*****************************************************************************/

#include <cstdlib>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <utility>

#include <osmium/area/multipolygon_manager.hpp>
#include <osmium/handler.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/util/memory.hpp>
#include <osmium/util/verbose_output.hpp>
#include <osmium/visitor.hpp>

#include "oat.hpp"

static bool count_okay(const std::vector<osmium::object_id_type> &ids) {
    if (ids.size() % 2 != 0) {
        return false;
    }

    auto it1 = ids.cbegin();
    auto it2 = std::next(it1);

    while (*it1 == *it2) {
        if (it1 + 2 != ids.cend() && *(it1 + 2) == *it1) {
            return false;
        }
        it1 += 2;
        if (it1 == ids.cend()) {
            return true;
        }
        it2 += 2;
    }

    return false;
}

static void add_node_ids(const osmium::WayNodeList& nodes, std::vector<osmium::object_id_type>& ids) {
    if (nodes.empty()) {
        return;
    }
    if (nodes.size() == 1) {
        ids.push_back(nodes.front().ref());
        return;
    }

    ids.push_back(nodes.front().ref());
    ids.push_back(nodes.back().ref());
    for (auto it = std::next(nodes.cbegin()); std::next(it) != nodes.cend(); ++it) {
        ids.push_back(it->ref());
        ids.push_back(it->ref());
    }
}

struct assembler_config_type {
    std::ofstream* stream_simple;
    std::ofstream* stream_complex;
};

static void write_id(std::ofstream *stream, char type, osmium::object_id_type id) {
    if (stream) {
        *(stream) << type << id << '\n';
    }
}

class Assembler {

    assembler_config_type m_config;
    osmium::area::area_stats m_stats;

public:

    using config_type = assembler_config_type;

    explicit Assembler(const assembler_config_type& config) : m_config(config) {
    }

    void operator()(const osmium::Way& /*way*/, osmium::memory::Buffer& /*out_buffer*/) {
#if 0
        if (way.nodes().size() < 4 || !way.is_closed()) {
            return;
        }

        std::vector<osmium::object_id_type> ids;
        add_node_ids(way.nodes(), ids);

        std::sort(ids.begin(), ids.end());

        auto* stream = count_okay(ids) ? m_config.stream_simple : m_config.stream_complex;
        write_id(stream, 'w', way.id());
#endif
    }

    void operator()(const osmium::Relation& relation, const std::vector<const osmium::Way*>& members, osmium::memory::Buffer& /*out_buffer*/) const {
        std::vector<osmium::object_id_type> ids;

        for (const auto* way : members) {
            add_node_ids(way->nodes(), ids);
        }

        std::sort(ids.begin(), ids.end());

        auto* stream = count_okay(ids) ? m_config.stream_simple : m_config.stream_complex;
        write_id(stream, 'r', relation.id());
    }

    const osmium::area::area_stats& stats() const noexcept {
        return m_stats;
    }

}; // class Assembler

class Handler : public osmium::handler::Handler {

    assembler_config_type m_config;

public:

    explicit Handler(const assembler_config_type& config) : m_config(config) {
    }

    void way(const osmium::Way& way) const {
        if (way.nodes().size() < 4 || !way.is_closed()) {
            return;
        }

        std::vector<osmium::object_id_type> ids;
        add_node_ids(way.nodes(), ids);

        std::sort(ids.begin(), ids.end());

        auto* stream = count_okay(ids) ? m_config.stream_simple : m_config.stream_complex;
        write_id(stream, 'w', way.id());
    }

}; // class Handler

static void print_help() {
    std::cout << "oat_complex_areas [OPTIONS] OSMFILE\n\n"
              << "Find complex areas in OSMFILE.\n\n"
              << "Options:\n"
              << "  -h, --help                 This help message\n"
              << "  -c, --output-complex=FILE  Where to write ids of complex areas (default: none)\n"
              << "  -s, --output-simple=FILE   Where to write ids of simple areas (default: none)\n"
              ;
}

int main(int argc, char* argv[]) {
    try {
        osmium::util::VerboseOutput vout{true};

        std::string filename_simple;
        std::string filename_complex;

        static const struct option long_options[] = {
            {"help",                 no_argument, nullptr, 'h'},
            {"output-complex", required_argument, nullptr, 'c'},
            {"output-simple",  required_argument, nullptr, 's'},
            {nullptr, 0, nullptr, 0}
        };

        while (true) {
            int c = getopt_long(argc, argv, "hc:s:", long_options, nullptr);
            if (c == -1) {
                break;
            }

            switch (c) {
                case 'h':
                    print_help();
                    return exit_code_ok;
                case 'c':
                    filename_complex = optarg;
                    break;
                case 's':
                    filename_simple = optarg;
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

        if (filename_simple.empty() && filename_complex.empty()) {
            std::cerr << "You have to set at least one of -c, --output-complex and -s, --output-simple.\n";
            return exit_code_cmdline_error;
        }

        std::unique_ptr<std::ofstream> out_simple;
        std::unique_ptr<std::ofstream> out_complex;

        if (!filename_simple.empty()) {
            out_simple = std::make_unique<std::ofstream>(filename_simple);
        }
        if (!filename_complex.empty()) {
            out_complex = std::make_unique<std::ofstream>(filename_complex);
        }

        assembler_config_type config{};
        config.stream_simple = out_simple.get();
        config.stream_complex = out_complex.get();

        osmium::area::MultipolygonManager<Assembler> mp_manager{config};
        Handler handler{config};

        const osmium::io::File input_file{argv[optind]};

        vout << "Starting first pass (reading relations)...\n";
        osmium::relations::read_relations(input_file, mp_manager);
        vout << "First pass done.\n";

        vout << "Starting second pass (reading ways)...\n";
        osmium::io::Reader reader{input_file, osmium::osm_entity_bits::way | osmium::osm_entity_bits::relation};
        osmium::apply(reader, handler, mp_manager.handler([](osmium::memory::Buffer&& /*buffer*/) {}));
        reader.close();
        vout << "Second pass done.\n";

        osmium::MemoryUsage mcheck;
        vout << "Actual memory usage:\n";
        vout << "  current: " << mcheck.current() << "MB\n";
        vout << "  peak:    " << mcheck.peak() << "MB\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return exit_code_error;
    }

    return exit_code_ok;
}

