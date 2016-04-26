/*****************************************************************************

  OSM Area Tools - Sizes

  https://github.com/osmcode/osm-area-tools

*****************************************************************************/

#include <iostream>

#include <osmium/area/assembler.hpp>

int main() {
    std::cout << "sizeof(osmium::area::detail::NodeRefSegment) = " << sizeof(osmium::area::detail::NodeRefSegment) << "\n";
    std::cout << "sizeof(osmium::area::detail::ProtoRing) = " << sizeof(osmium::area::detail::ProtoRing) << "\n";
}

