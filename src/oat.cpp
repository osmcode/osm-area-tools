/*****************************************************************************

  OSM Area Tools - Statistics

  https://github.com/osmcode/osm-area-tools

*****************************************************************************/

#include "oat.hpp"

osmium::osm_entity_bits::type entity_bits(const std::string& location_index_type) {
    if (location_index_type == "none") {
        return osmium::osm_entity_bits::way;
    }
    return osmium::osm_entity_bits::way | osmium::osm_entity_bits::node;
}

