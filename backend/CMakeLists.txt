cmake_minimum_required(VERSION 3.10)
project(socoles)
configure_file(config.h.in config.h)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set the build type to Debug
set(CMAKE_BUILD_TYPE Debug)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX   REQUIRED libpqxx)
find_package(PostgreSQL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(Crow REQUIRED)
find_package(Boost 1.74 REQUIRED COMPONENTS unit_test_framework program_options system thread)
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)


find_library(PG_QUERY_LIBRARY pg_query)
if(NOT PG_QUERY_LIBRARY)
    message(FATAL_ERROR "libpg_query not found")
endif()

find_library(DUCKDB_LIBRARY NAMES duckdb PATHS duckdb/ OPTIONAL_DIRS lib)
if(NOT DUCKDB_LIBRARY)
    message(FATAL_ERROR "libduckdb not found")
endif()

# Test executable target
add_executable(test_socoles  test.cpp utils.cpp tree_edit_distance.cpp grader.cpp sql_keywords.cpp abstract_syntax_tree.cpp model_query.cpp student_query.cpp admin.cpp my_duckdb.cpp process_queries.cpp #uery_generator.cpp
                my_evosql.cpp goals.cpp thread_pool.cpp clauses/select/from_clause.cpp clauses/select/where_clause.cpp clauses/select/select_clause.cpp clauses/insert_clause.cpp clauses/common.cpp
                clauses/select/group_by_clause.cpp clauses/select/having_clause.cpp clauses/select/order_by_clause.cpp
                clauses/assertion_clause.cpp clauses/create_clause.cpp clauses/delete_clause.cpp clauses/update_clause.cpp clauses/create_view.cpp clauses/alter_clause.cpp
                my_postgresql.cpp query_engine.cpp)       
target_include_directories(test_socoles  PRIVATE ${PostgreSQL_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})
target_link_libraries(test_socoles  PRIVATE ${PostgreSQL_LIBRARIES} ${PQXX_LIBRARIES} ${PG_QUERY_LIBRARY} nlohmann_json::nlohmann_json ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${DUCKDB_LIBRARY} -lpthread ${Boost_LIBRARIES} Crow::Crow OpenSSL::SSL OpenSSL::Crypto) 
target_compile_definitions(test_socoles  PRIVATE BOOST_TEST_DYN_LINK)

# Server executable target
add_executable(socoles_server server.cpp utils.cpp tree_edit_distance.cpp grader.cpp sql_keywords.cpp abstract_syntax_tree.cpp model_query.cpp student_query.cpp admin.cpp my_duckdb.cpp process_queries.cpp #query_generator.cpp
                my_evosql.cpp goals.cpp thread_pool.cpp clauses/select/from_clause.cpp clauses/select/where_clause.cpp clauses/select/select_clause.cpp clauses/insert_clause.cpp clauses/common.cpp
                clauses/select/group_by_clause.cpp clauses/select/having_clause.cpp clauses/select/order_by_clause.cpp
                clauses/assertion_clause.cpp clauses/create_clause.cpp clauses/delete_clause.cpp clauses/update_clause.cpp clauses/create_view.cpp clauses/alter_clause.cpp
                my_postgresql.cpp query_engine.cpp)
target_include_directories(socoles_server PRIVATE ${PostgreSQL_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIRS})
target_link_libraries(socoles_server PRIVATE ${PostgreSQL_LIBRARIES} ${PQXX_LIBRARIES} ${PG_QUERY_LIBRARY} nlohmann_json::nlohmann_json ${DUCKDB_LIBRARY} -lpthread ${Boost_LIBRARIES} Crow::Crow OpenSSL::SSL OpenSSL::Crypto ${ZLIB_LIBRARIES})
