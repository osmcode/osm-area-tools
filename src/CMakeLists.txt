#-----------------------------------------------------------------------------
#
#  CMake Config
#
#  OSM Area Tools - Source
#
#-----------------------------------------------------------------------------

add_executable(oat_closed_way_filter oat_closed_way_filter.cpp)
target_link_libraries(oat_closed_way_filter ${OSMIUM_LIBRARIES})
set_pthread_on_target(oat_closed_way_filter)
install(TARGETS oat_closed_way_filter DESTINATION bin)

add_executable(oat_closed_way_tags oat_closed_way_tags.cpp)
target_link_libraries(oat_closed_way_tags ${OSMIUM_LIBRARIES})
set_pthread_on_target(oat_closed_way_tags)
install(TARGETS oat_closed_way_tags DESTINATION bin)

add_executable(oat_complex_areas oat_complex_areas.cpp)
target_link_libraries(oat_complex_areas ${OSMIUM_IO_LIBRARIES})
set_pthread_on_target(oat_complex_areas)
install(TARGETS oat_complex_areas DESTINATION bin)

add_executable(oat_create_areas oat_create_areas.cpp oat.cpp)
target_link_libraries(oat_create_areas ${OSMIUM_LIBRARIES})
set_pthread_on_target(oat_create_areas)
install(TARGETS oat_create_areas DESTINATION bin)

add_executable(oat_failed_area_tags oat_failed_area_tags.cpp oat.cpp)
target_link_libraries(oat_failed_area_tags ${OSMIUM_LIBRARIES})
set_pthread_on_target(oat_failed_area_tags)
install(TARGETS oat_failed_area_tags DESTINATION bin)

add_executable(oat_find_problems oat_find_problems.cpp)
target_link_libraries(oat_find_problems ${OSMIUM_IO_LIBRARIES})
set_pthread_on_target(oat_find_problems)
install(TARGETS oat_find_problems DESTINATION bin)

add_executable(oat_large_areas oat_large_areas.cpp)
target_link_libraries(oat_large_areas ${OSMIUM_IO_LIBRARIES} sqlite3)
set_pthread_on_target(oat_large_areas)
install(TARGETS oat_large_areas DESTINATION bin)

add_executable(oat_mercator oat_mercator.cpp oat.cpp)
target_link_libraries(oat_mercator ${OSMIUM_LIBRARIES})
set_pthread_on_target(oat_mercator)
install(TARGETS oat_mercator DESTINATION bin)

add_executable(oat_problem_report oat_problem_report.cpp oat.cpp)
target_link_libraries(oat_problem_report ${OSMIUM_LIBRARIES})
set_pthread_on_target(oat_problem_report)
install(TARGETS oat_problem_report DESTINATION bin)

add_executable(oat_sizes oat_sizes.cpp)
install(TARGETS oat_sizes DESTINATION bin)

add_executable(oat_stats oat_stats.cpp)
target_link_libraries(oat_stats ${OSMIUM_IO_LIBRARIES} sqlite3)
set_pthread_on_target(oat_stats)
install(TARGETS oat_stats DESTINATION bin)

