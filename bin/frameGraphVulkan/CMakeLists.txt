cmake_minimum_required(VERSION 3.10)

find_package(SDL2 REQUIRED)


################################################################################
# Build the executable
#
################################################################################
set(outName              fgVk)   # name of the library
set(srcFiles             "main.cpp"     )          # all the source files for this library
set(PublicLinkedTargets  ""             )
set(PrivateLinkedTargets  CONAN_PKG::spdlog SDL2::SDL2 CONAN_PKG::glm frameGraph gul::gul vkw::vkw CONAN_PKG::vulkan-memory-allocator CONAN_PKG::vulkan-loader CONAN_PKG::vulkan-headers)
#-------------------------------------------------------------------------------
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

