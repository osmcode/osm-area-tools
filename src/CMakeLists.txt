#-----------------------------------------------------------------------------
#
#  CMake Config
#
#  OSM Area Tools - Source
#
#-----------------------------------------------------------------------------

add_executable(oat_create_areas oat_create_areas.cpp)
target_link_libraries(oat_create_areas ${OSMIUM_LIBRARIES})
install(TARGETS oat_create_areas DESTINATION bin)

add_executable(oat_find_problems oat_find_problems.cpp)
target_link_libraries(oat_find_problems ${OSMIUM_IO_LIBRARIES})
install(TARGETS oat_find_problems DESTINATION bin)

add_executable(oat_large_areas oat_large_areas.cpp)
target_link_libraries(oat_large_areas ${OSMIUM_IO_LIBRARIES} sqlite3)
install(TARGETS oat_large_areas DESTINATION bin)

add_executable(oat_sizes oat_sizes.cpp)
target_link_libraries(oat_sizes ${OSMIUM_IO_LIBRARIES})
install(TARGETS oat_sizes DESTINATION bin)

add_executable(oat_stats oat_stats.cpp)
target_link_libraries(oat_stats ${OSMIUM_IO_LIBRARIES} sqlite3)
install(TARGETS oat_stats DESTINATION bin)

