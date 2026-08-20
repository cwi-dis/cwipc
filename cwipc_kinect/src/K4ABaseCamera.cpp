#include "turbojpeg.h"
#include "K4ABaseCamera.hpp"


k4a_image_t cwipc_k4a_uncompress_color_image(CwipcBaseCamera *cam, k4a_capture_t capture, k4a_image_t color_image) {
    assert(capture);
    assert(color_image);

    if (k4a_image_get_format(color_image) != K4A_IMAGE_FORMAT_COLOR_MJPG) {
        // Not the format we expect. Return as-is.
        if (k4a_image_get_format(color_image) != K4A_IMAGE_FORMAT_COLOR_BGRA32) {
            cam->_log_error("Color image format is not MJPG or BGRA32, cannot decompress");
        }
        return color_image;
    }

    k4a_image_t uncompressed_color_image = nullptr;

    int color_image_width_pixels = k4a_image_get_width_pixels(color_image);
    int color_image_height_pixels = k4a_image_get_height_pixels(color_image);

    //COLOR image is JPEG compressed. we need to convert the image to BGRA format.
    if (K4A_RESULT_SUCCEEDED != k4a_image_create(K4A_IMAGE_FORMAT_COLOR_BGRA32,
        color_image_width_pixels,
        color_image_height_pixels,
        color_image_width_pixels * 4 * (int)sizeof(uint8_t),
        &uncompressed_color_image))
    {
      cam->_log_error("Failed to create image buffer for image decompression");
      return color_image;
    }

    tjhandle tjHandle = tjInitDecompress();

    if (tjDecompress2(tjHandle,
        k4a_image_get_buffer(color_image),
        static_cast<unsigned long>(k4a_image_get_size(color_image)),
        k4a_image_get_buffer(uncompressed_color_image),
        color_image_width_pixels,
        0, // pitch
        color_image_height_pixels,
        TJPF_BGRA,
        TJFLAG_FASTDCT | TJFLAG_FASTUPSAMPLE) != 0)
    {
        cam->_log_error("Failed to decompress color frame");
    }

    if (tjDestroy(tjHandle)) {
        cam->_log_error("Failed to destroy turboJPEG handle");
    }

    assert(uncompressed_color_image);
    k4a_image_release(color_image);
    k4a_capture_set_color_image(capture, uncompressed_color_image);

    return uncompressed_color_image;
}

