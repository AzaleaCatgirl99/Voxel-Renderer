# Copies the asset files to the project binary directory
file(COPY ${CMAKE_CURRENT_LIST_DIR}/../assets DESTINATION ${CMAKE_BINARY_DIR} PATTERN "shaders" EXCLUDE)
message(STATUS "-- Copied assets to build directory")

# Creates the shader directory
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/assets/shaders)
