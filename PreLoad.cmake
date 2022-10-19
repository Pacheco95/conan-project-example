set(CONAN_PROFILE conanprofile.ini)

configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/${CONAN_PROFILE}
        ${CMAKE_BINARY_DIR}/${CONAN_PROFILE}
)

execute_process(
        COMMAND conan install .. --profile=${CONAN_PROFILE}
)

set(CMAKE_GENERATOR "Visual Studio 16 2019" CACHE INTERNAL "" FORCE)
