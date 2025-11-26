#include "util/BufferedFile.h"

#include "util/Logger.h"
#include <cstddef>
#include <fstream>
#include <ios>
#include <vector>

Logger BufferedFile::sLogger = Logger("BufferedFile");

BufferedFile BufferedFile::Read(const std::string& path, bool binary) {
    BufferedFile file;

    // Sets the open mode for the file stream.
    std::ios_base::openmode mode = std::ios::ate | std::ios::in;
    if (binary)
        mode |= std::ios::binary;

    // Creates the file stream and reads it at the end of the file.
    std::fstream stream(path, mode);
    if (!stream.is_open())
        throw sLogger.RuntimeError("Failed to read file '", path, "'! Could not open stream.");

    // Allocates the file data.
    size_t size = stream.tellg();
    file.m_data = std::vector<char>(size);

    // Sets the stream back to the beginning of the file.
    stream.seekg(0);

    // Sets the file data.
    stream.read(file.m_data.data(), size);

    // Closes the stream.
    stream.close();

    return file;
}
