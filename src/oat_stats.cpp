/*****************************************************************************

  OSM Area Tools - Statistics

  https://github.com/osmcode/osm-area-tools

*****************************************************************************/

#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <unordered_map>

#include <sqlite.hpp>

#include <osmium/handler.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/util/verbose_output.hpp>
#include <osmium/visitor.hpp>

#include "oat.hpp"

class StatsHandler : public osmium::handler::Handler {

    uint32_t m_ways_all = 0;
    uint32_t m_ways_closed = 0;
    uint32_t m_relations_all = 0;
    uint32_t m_relations_type_multipolygon = 0;
    uint32_t m_relations_type_boundary = 0;
    uint32_t m_area_relations_without_tags = 0;
    uint32_t m_area_relations_without_members = 0;
    uint32_t m_area_relations_with_single_member = 0;
    uint32_t m_area_relations_with_single_member_and_few_nodes = 0;

    uint32_t m_member_nodes = 0;
    uint32_t m_member_ways = 0;
    uint32_t m_member_relations = 0;

    uint32_t m_roles_outer = 0;
    uint32_t m_roles_inner = 0;
    uint32_t m_roles_empty = 0;
    uint32_t m_roles_other = 0;

    uint32_t m_nodes_in_ways_histogram[2001];

    std::unordered_map<uint32_t, uint32_t> m_nodes_in_ways;

    std::vector<uint32_t> m_ways_in_relations;
    std::vector<uint32_t> m_nodes_in_relations;

    static void add_stat(Sqlite::Database& db, const char* key, uint64_t value) {
        static Sqlite::Statement statement{db, "INSERT INTO stats (key, value) VALUES (?, ?)"};
        statement.bind_text(key);
        statement.bind_int64(value);
        statement.execute();
    }

    void mp_relation(const osmium::Relation& relation) {
        if (relation.tags().size() == 1) {
            ++m_area_relations_without_tags;
        }
        if (relation.members().empty()) {
            ++m_area_relations_without_members;
        } else if (relation.members().size() == 1) {
            ++m_area_relations_with_single_member;
            if (m_nodes_in_ways[relation.members().begin()->ref()] < 500) {
                ++m_area_relations_with_single_member_and_few_nodes;
            }
        }

        uint32_t way_members = 0;
        uint32_t nodes_in_way_members = 0;

        for (const auto& member : relation.members()) {
            switch (member.type()) {
                case osmium::item_type::node:
                    ++m_member_nodes;
                    break;
                case osmium::item_type::way:
                    ++m_member_ways;
                    ++way_members;
                    if (!std::strcmp(member.role(), "outer")) {
                        ++m_roles_outer;
                    } else if (!std::strcmp(member.role(), "inner")) {
                        ++m_roles_inner;
                    } else if (!std::strcmp(member.role(), "")) {
                        ++m_roles_empty;
                    } else {
                        ++m_roles_other;
                    }
                    nodes_in_way_members += m_nodes_in_ways[member.ref()];
                    break;
                case osmium::item_type::relation:
                    ++m_member_relations;
                    break;
                default:
                    break;
            }
        }

        if (m_ways_in_relations.size() <= way_members) {
            m_ways_in_relations.resize(way_members + 1);
        }
        ++m_ways_in_relations[way_members];

        if (m_nodes_in_relations.size() <= nodes_in_way_members) {
            m_nodes_in_relations.resize(nodes_in_way_members + 1);
        }
        ++m_nodes_in_relations[nodes_in_way_members];
    }

public:

    StatsHandler() {
        std::fill(std::begin(m_nodes_in_ways_histogram), std::end(m_nodes_in_ways_histogram), 0);
    }

    void way(const osmium::Way& way) {
        ++m_ways_all;

        if (!way.is_closed()) {
            return;
        }

        ++m_ways_closed;
        assert(way.nodes().size() <= 2000);
        ++m_nodes_in_ways_histogram[way.nodes().size()];

        m_nodes_in_ways[uint32_t(way.id())] = uint32_t(way.nodes().size());
    }

    void relation(const osmium::Relation& relation) {
        ++m_relations_all;
        const char* type = relation.tags().get_value_by_key("type");
        if (!type) {
            return;
        }
        if (!strcmp(type, "multipolygon")) {
            ++m_relations_type_multipolygon;
            mp_relation(relation);
        } else if (!strcmp(type, "boundary")) {
            ++m_relations_type_boundary;
            mp_relation(relation);
        }
    }

    void write_stats_to_db(const std::string& database_name) const {
        unlink(database_name.c_str());
        Sqlite::Database db{database_name, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE};

        db.exec("CREATE TABLE stats (key VARCHAR, value INT64 DEFAULT 0);");
        db.exec("CREATE TABLE histogram_nodes_in_ways (value INTEGER, num INTEGER);");
        db.exec("CREATE TABLE histogram_ways_in_relations (value INTEGER, num INTEGER);");
        db.exec("CREATE TABLE histogram_nodes_in_relations (value INTEGER, num INTEGER);");

        db.begin_transaction();
        add_stat(db, "ways_all", m_ways_all);
        add_stat(db, "ways_closed", m_ways_closed);
        add_stat(db, "relations_all", m_relations_all);
        add_stat(db, "relations_type_multipolygon", m_relations_type_multipolygon);
        add_stat(db, "relations_type_boundary", m_relations_type_boundary);
        add_stat(db, "area_relations", m_relations_type_multipolygon + m_relations_type_boundary);
        add_stat(db, "area_relations_without_tags", m_area_relations_without_tags);
        add_stat(db, "area_relations_without_members", m_area_relations_without_members);
        add_stat(db, "area_relations_with_single_member", m_area_relations_with_single_member);
        add_stat(db, "area_relations_with_single_member_and_few_nodes", m_area_relations_with_single_member_and_few_nodes);
        add_stat(db, "member_nodes", m_member_nodes);
        add_stat(db, "member_ways", m_member_ways);
        add_stat(db, "member_relations", m_member_relations);
        add_stat(db, "roles_outer", m_roles_outer);
        add_stat(db, "roles_inner", m_roles_inner);
        add_stat(db, "roles_empty", m_roles_empty);
        add_stat(db, "roles_other", m_roles_other);

        {
            Sqlite::Statement statement{db, "INSERT INTO histogram_nodes_in_ways (value, num) VALUES (?, ?);"};
            for (int i = 0; i <= 2000; ++i) {
                if (m_nodes_in_ways_histogram[i]) {
                    statement.bind_int(i);
                    statement.bind_int(m_nodes_in_ways_histogram[i]);
                    statement.execute();
                }
            }
        }

        {
            Sqlite::Statement statement{db, "INSERT INTO histogram_ways_in_relations (value, num) VALUES (?, ?);"};
            for (size_t i = 0; i < m_ways_in_relations.size(); ++i) {
                if (m_ways_in_relations[i]) {
                    statement.bind_int(i);
                    statement.bind_int(m_ways_in_relations[i]);
                    statement.execute();
                }
            }
        }

        {
            Sqlite::Statement statement{db, "INSERT INTO histogram_nodes_in_relations (value, num) VALUES (?, ?);"};
            for (size_t i = 0; i < m_nodes_in_relations.size(); ++i) {
                if (m_nodes_in_relations[i]) {
                    statement.bind_int(i);
                    statement.bind_int(m_nodes_in_relations[i]);
                    statement.execute();
                }
            }
        }

        db.commit();
    }

}; // class StatsHandler

int main(int argc, char* argv[]) {
    osmium::util::VerboseOutput vout{true};

    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " OSMFILE\n";
        exit(exit_code_cmdline_error);
    }

    StatsHandler stats_handler;

    vout << "Reading OSM data...\n";

    const osmium::io::File infile(argv[1]);
    osmium::io::Reader reader(infile, osmium::osm_entity_bits::way | osmium::osm_entity_bits::relation);
    osmium::apply(reader, stats_handler);
    reader.close();

    vout << "Writing statistics to database...\n";
    stats_handler.write_stats_to_db("area-stats.db");

    vout << "Done.\n";

    return exit_code_ok;
}

