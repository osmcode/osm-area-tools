#ifndef OAT_HPP
#define OAT_HPP

#include <string>

#include <osmium/osm/entity_bits.hpp>

enum exit_codes {
    exit_code_ok            = 0,
    exit_code_error         = 1,
    exit_code_cmdline_error = 2
};

osmium::osm_entity_bits::type entity_bits(const std::string& location_index_type);

void show_index_types();

#endif // OAT_HPP
