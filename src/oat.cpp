/*****************************************************************************

  OSM Area Tools - Statistics

  https://github.com/osmcode/osm-area-tools

*****************************************************************************/

#include <iostream>

#include <osmium/index/map.hpp>
#include <osmium/osm/location.hpp>
#include <osmium/osm/types.hpp>

#include "oat.hpp"

osmium::osm_entity_bits::type entity_bits(const std::string& location_index_type) {
    if (location_index_type == "none") {
        return osmium::osm_entity_bits::way;
    }
    return osmium::osm_entity_bits::way | osmium::osm_entity_bits::node;
}

void show_index_types() {
    const auto& map_factory = osmium::index::MapFactory<osmium::unsigned_object_id_type, osmium::Location>::instance();
    std::cout << "Available index types:\n";
    for (const auto& map_type : map_factory.map_types()) {
        std::cout << "  " << map_type;
        if (map_type == "flex_mem") {
            std::cout << " (default)";
        }
        std::cout << '\n';
    }
}

