#include "util/BufferedImage.h"

#include "util/Logger.h"

// These need to be placed and defined within a source file because STB is a header-only library.
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

Logger BufferedImage::sLogger = Logger("BufferedImage");
