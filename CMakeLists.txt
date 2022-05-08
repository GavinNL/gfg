cmake_minimum_required(VERSION 3.10)



################################################################################
# The Project Name will the the name of the folder. Change it if you
# dont like it.
################################################################################
project( gfg  CXX )
################################################################################



# Extra targets.  adds coverage::coverage, warning::all, warning:error
SET(PROJECT_TARGETS_PREFIX ${PROJECT_NAME})

get_directory_property( gfg_is_sub_project PARENT_DIRECTORY)
if(gfg_is_sub_project)
    option(GFG_BUILD_EXAMPLES "Build Examples" FALSE)
else()
    option(GFG_BUILD_EXAMPLES "Build Examples" TRUE)
    #######################################################################################
    # Custom targets so that these files show up in QtCreator for easy editing
    #######################################################################################
    add_custom_target(gfg_otherfiles SOURCES conanfile.txt .gitignore README.md)
    #######################################################################################
endif()


################################################################################
# Build the Library
#
################################################################################
set(outName ${PROJECT_NAME})
add_library( gfg INTERFACE )
add_library( gfg::gfg ALIAS  gfg)

target_include_directories( ${outName}
                            INTERFACE
                               "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
 )

target_compile_features( ${outName}
                          INTERFACE
                              cxx_std_17)
################################################################################



if( ${GFG_BUILD_EXAMPLES})

    if( EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake )
        include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
        conan_basic_setup(NO_OUTPUT_DIRS TARGETS)
    endif()

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party_sym")
        add_subdirectory(third_party_sym/vkw)
        add_subdirectory(third_party_sym/gul)
        add_subdirectory(third_party_sym/gvu)
        add_subdirectory(third_party_sym/GLSLCompiler)
    else()
        # These are just needed for the examples
        add_subdirectory(third_party/gul)
        add_subdirectory(third_party/vkw)
        add_subdirectory(third_party/gvu)
        add_subdirectory(third_party/GLSLCompiler)
    endif()


    add_subdirectory(bin/frameGraphOpenGL)
    add_subdirectory(bin/frameGraphVulkan)

    add_subdirectory(test)

endif()

