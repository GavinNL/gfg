cmake_minimum_required(VERSION 3.10)



################################################################################
# The Project Name will the the name of the folder. Change it if you
# dont like it.
################################################################################
project( frameGraph  CXX )
################################################################################

if( EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake )
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup(NO_OUTPUT_DIRS TARGETS)
endif()

find_package(SDL2 REQUIRED)


################################################################################
# Build the Library
#
################################################################################
set(outName              ${PROJECT_NAME})   # name of the library
set(srcFiles             "main.cpp"     )          # all the source files for this library
set(PublicLinkedTargets  "CONAN_PKG::spdlog"             )
set(PrivateLinkedTargets  CONAN_PKG::spdlog)

#-------------------------------------------------------------------------------
#file(GLOB_RECURSE srcFiles ${srcFiles} )
add_library( ${outName} INTERFACE )

target_include_directories( ${outName}
                            INTERFACE
                               "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
 )

target_compile_features( ${outName}
                          INTERFACE
                              cxx_std_17)

target_compile_definitions( ${outName}
                                INTERFACE
                                TEST_DEFINE)

target_link_libraries( ${outName}  INTERFACE  ${PublicLinkedTargets}  )
#target_link_libraries( ${outName}  PRIVATE ${PrivateLinkedTargets} )

################################################################################

################################################################################
# Build the executable
#
################################################################################
set(outName              fgOpenGL)   # name of the library
set(srcFiles             "main.cpp"     )          # all the source files for this library
set(PublicLinkedTargets  ""             )
set(PrivateLinkedTargets  CONAN_PKG::spdlog SDL2::SDL2 CONAN_PKG::glbinding CONAN_PKG::glm frameGraph)

#-------------------------------------------------------------------------------
file(GLOB_RECURSE srcFiles ${srcFiles} )
add_executable( ${outName} ${srcFiles} )

target_include_directories( ${outName}
                            PUBLIC
                               "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
 )

target_compile_features( ${outName}
                          PUBLIC
                              cxx_std_17)

target_compile_definitions( ${outName}
                                PUBLIC
                                TEST_DEFINE)

target_link_libraries( ${outName}  PUBLIC  ${PublicLinkedTargets}  )
target_link_libraries( ${outName}  PRIVATE ${PrivateLinkedTargets} )

################################################################################

add_subdirectory(test)
