cmake_minimum_required(VERSION 3.17)

set(TARGET openai-server)
project(${TARGET})

option(OPENAI_SERVER_LLAMA "Use llama.cpp backend" ON)
option(OPENAI_SERVER_STABLEDIFFUSION "Use StableDiffusion backend" OFF)

include(FetchContent)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#add_compile_options(
#    -Wno-zero-as-null-pointer-constant 
#    -Wno-extra-semi-stmt
#    -Wno-shadow
#    -Wno-suggest-override
#    -Wno-suggest-destructor-override
#)

#get cxxopts
FetchContent_Declare(
    cxxopts
    GIT_REPOSITORY "https://github.com/jarro2783/cxxopts.git"
    GIT_TAG v3.1.1
    GIT_SHALLOW ON
    SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/deps/cxxopts"
)
FetchContent_MakeAvailable(cxxopts)

#get rapidjson
set(RAPIDJSON_BUILD_DOC OFF CACHE BOOL "Build rapidjson documentation.")
set(RAPIDJSON_BUILD_EXAMPLES OFF CACHE BOOL "Build rapidjson examples.")
set(RAPIDJSON_BUILD_TESTS OFF CACHE BOOL "Build rapidjson perftests and unittests.")
FetchContent_Declare(
    rapidjson
    GIT_REPOSITORY "https://github.com/Tencent/rapidjson.git"
    GIT_TAG v1.1.0
    GIT_SHALLOW ON
    SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/deps/rapidjson"
)
FetchContent_MakeAvailable(rapidjson)

#get stduuid
FetchContent_Declare(
    stduuid
    GIT_REPOSITORY "https://github.com/mariusbancila/stduuid.git"
    GIT_TAG v1.2.3
    GIT_SHALLOW ON
    SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/deps/stduuid"
)
FetchContent_MakeAvailable(stduuid)

#get cpp-httplib
FetchContent_Declare(
    cpp-httplib
    GIT_REPOSITORY "https://github.com/yhirose/cpp-httplib.git"
    GIT_TAG v0.12.5
    GIT_SHALLOW ON
    SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/deps/cpp-httplib"
)
FetchContent_MakeAvailable(cpp-httplib)

if(OPENAI_SERVER_LLAMA)
    #get llama.cpp
    FetchContent_Declare(
        llama
        GIT_REPOSITORY "https://github.com/caseymcc/llama.cpp.git"
        GIT_TAG rocm
        GIT_SHALLOW ON
        SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/deps/llama"
    )
    FetchContent_MakeAvailable(llama)
endif()

set(${TARGET}_source
    oais/interfaces/llamaModel.cpp
    oais/json/builder.cpp
    oais/modelDefinition.cpp
    oais/modelServer.cpp
    oais/main.cpp
    oais/serverOptions.cpp
)

add_executable(${TARGET} ${${TARGET}_source})

#set_property(TARGET ${TARGET} PROPERTY CXX_STANDARD 17)
target_include_directories(${TARGET} PUBLIC 
    $<BUILD_INTERFACE:
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/rapidjson/include
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/llama
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/uuid/include
    >
)
target_compile_features(llama PUBLIC cxx_std_17) # don't bump
target_link_libraries(${TARGET} PRIVATE 
    cxxopts 
    httplib 
    stduuid
    llama 
    ${CMAKE_THREAD_LIBS_INIT}
)
