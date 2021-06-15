cmake_minimum_required(VERSION 3.10)
project(MiniSQL)

set(CMAKE_CXX_STANDARD 20)

set(MINISQL_SRC "${PROJECT_SOURCE_DIR}/src")
set(API_SRC "${MINISQL_SRC}/API")
set(BUFFER_SRC "${MINISQL_SRC}/BufferManager")
set(CATALOG_SRC "${MINISQL_SRC}/CatalogManager")
set(INDEX_SRC "${MINISQL_SRC}/IndexManager")
set(INTERPRETER_SRC "${MINISQL_SRC}/Interpreter")
set(RECORD_SRC "${MINISQL_SRC}/RecordManager")

include_directories("${MINISQL_SRC}/Include")

file(GLOB API_SRC_FILE ${API_SRC}/*.cpp)
file(GLOB BUFFER_SRC_FILE ${BUFFER_SRC}/*.cpp)
file(GLOB CATALOG_SRC_FILE ${CATALOG_SRC}/*.cpp)
file(GLOB INDEX_SRC_FILE ${INDEX_SRC}/*.cpp)
file(GLOB INTERPRETER_SRC_FILE ${INTERPRETER_SRC}/*.cpp)
file(GLOB RECORD_SRC_FILE ${RECORD_SRC}/*.cpp)

add_library(API ${API_SRC_FILE})
add_library(Buffer ${BUFFER_SRC_FILE})
add_library(Catalog ${CATALOG_SRC_FILE})
add_library(Index ${INDEX_SRC_FILE})
add_library(Interpreter ${INTERPRETER_SRC_FILE})
add_library(Record ${RECORD_SRC_FILE})

add_executable(${PROJECT_NAME}
    ${MINISQL_SRC}/testBM.cpp
)

target_link_libraries(${PROJECT_NAME}
    Buffer Catalog Index Record API Interpreter
)