# SOCOLES Backend

This directory contains the C++ grading engine for SOCOLES:  
it accepts SQL-query submissions evaluates them against the reference queries, generated tailored feedback and awards full or partial points based on correctness.

---

## Prerequisites

- **CMake** ≥ 3.10  
- **C++20** compiler (GCC 10+, Clang 11+, or MSVC 2019+)  
- **PostgreSQL** (client & server) and **libpqxx** development headers  
- **nlohmann_json**  
- **Crow** (C++ microframework)  
- **Boost** ≥ 1.74, with components:
  - unit_test_framework  
  - program_options  
  - system  
  - thread  
- **OpenSSL**  
- **Zlib**  
- **libpg_query** (PostgreSQL query parser library)  
- **DuckDB** (in-memory DBMS)

Install these via your OS package manager or from source.

---

## Building

```bash
cd backend
mkdir -p build
cd build

# Debug build (default)
cmake -DCMAKE_BUILD_TYPE=Debug ..
make

# Or for a Release build:
# cmake -DCMAKE_BUILD_TYPE=Release ..
# make
