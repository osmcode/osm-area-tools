/*****************************************************************************

  OSM Area Tools - Sizes

  https://github.com/osmcode/osm-area-tools

*****************************************************************************/

#include "oat.hpp"

#include <osmium/area/assembler.hpp>

#include <iostream>

int main() {
    std::cout << "sizeof(osmium::area::detail::NodeRefSegment) = " << sizeof(osmium::area::detail::NodeRefSegment) << '\n';
    std::cout << "sizeof(osmium::area::detail::ProtoRing) = " << sizeof(osmium::area::detail::ProtoRing) << '\n';

    return exit_code_ok;
}

