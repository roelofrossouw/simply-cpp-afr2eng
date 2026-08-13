if (APPLE)
    cmake_minimum_required(VERSION 4.2)
else ()
    cmake_minimum_required(VERSION 3.28)
endif ()
project(cd-af2eng)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

############################
#  Fetch Third Party Code  #
############################
set(SETUP_VERSION_FILE "${CMAKE_BINARY_DIR}/setup_a2e.version")
if (NOT EXISTS "${SETUP_VERSION_FILE}")
    execute_process(COMMAND git init -b main WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    execute_process(COMMAND git submodule add git@github.com:roelofrossouw/simply-cpp.git third_party/sc WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    execute_process(COMMAND git submodule add git@github.com:OpenNMT/CTranslate2.git third_party/ctranslate2 WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    execute_process(COMMAND git submodule add git@github.com:google/sentencepiece.git third_party/sentencepiece WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    execute_process(COMMAND git submodule update --init --recursive WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    if (APPLE)
        execute_process(COMMAND brew install gperftools WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    else ()
        execute_process(COMMAND apt -y install clang-tidy WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    endif ()
    file(WRITE "${SETUP_VERSION_FILE}" "1")
endif ()

######################
#   Local Includes   #
######################
add_subdirectory(third_party/sc sc EXCLUDE_FROM_ALL)
if (APPLE)
    set(OPENMP_RUNTIME "NONE" CACHE STRING "" FORCE)   # Don't use OpenMP
    set(WITH_MKL OFF CACHE BOOL "" FORCE)
    set(WITH_ACCELERATE ON CACHE BOOL "" FORCE)
    set(WITH_RUY ON CACHE BOOL "" FORCE)
endif ()

get_directory_property(SAVED_COMPILE_OPTIONS COMPILE_OPTIONS)

set(CMAKE_POLICY_DEFAULT_CMP0074 NEW)
add_compile_options(-Wno-deprecated-literal-operator)
add_compile_options(-Wno-deprecated-this-capture)
add_compile_options(-Wno-unused-parameter)
add_compile_options(-Wno-reorder-ctor)
add_compile_options(-Wno-unused-variable)
add_compile_options(-Wno-sign-compare)
add_subdirectory(third_party/ctranslate2 ctranslate2 EXCLUDE_FROM_ALL)

set(SPM_ENABLE_SHARED OFF CACHE BOOL "" FORCE)
set(SPM_ENABLE_TCMALLOC OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/sentencepiece sentencepiece EXCLUDE_FROM_ALL)

set_directory_properties(PROPERTIES COMPILE_OPTIONS "${SAVED_COMPILE_OPTIONS}")

project(afr2eng)
add_library(${PROJECT_NAME} STATIC src/af2eng.cpp include/af2eng.h)
target_include_directories(${PROJECT_NAME} PRIVATE third_party/sentencepiece/src)
target_include_directories(${PROJECT_NAME} PRIVATE third_party/sentencepiece)
target_include_directories(${PROJECT_NAME} PUBLIC include)
target_link_libraries(${PROJECT_NAME} PUBLIC sentencepiece-static ctranslate2)

project(test_af2eng)
add_executable(${PROJECT_NAME} test/af2eng.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE sc afr2eng)