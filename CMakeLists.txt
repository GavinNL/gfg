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

option(FGG_BUILD_EXAMPLES ON)

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



if( FGG_BUILD_EXAMPLES)
    # These are just needed for the examples
    add_subdirectory(third_party/gul)
    add_subdirectory(third_party/vkw)
    add_subdirectory(third_party/vkb)
    add_subdirectory(third_party/GLSLCompiler)

    add_subdirectory(bin/frameGraphOpenGL)
    add_subdirectory(bin/frameGraphVulkan)

endif()

add_subdirectory(test)
