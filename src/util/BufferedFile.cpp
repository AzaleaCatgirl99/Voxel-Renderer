#include "util/BufferedFile.h"

#include "util/Logger.h"
#include <SDL3/SDL_iostream.h>
#include <cstddef>

Logger BufferedFile::sLogger = Logger("BufferedFile");

BufferedFile BufferedFile::Read(const std::string& path, bool binary) {
    size_t size = 0;
    const char* data = static_cast<const char*>(SDL_LoadFile(path.c_str(), &size));

    return BufferedFile(data, size);
}
