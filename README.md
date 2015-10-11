cgCompress
==========

Efficiently store Visual Novel cgs by using a multi-image format for storing all variations of one image in one file. This way, only the differences need to be stored which reduces the file size significantly.
cgCompress is an attempt to automatize this process, and reduce file size as much as possible by optimizing the configuration of differences.
*NOTE*: This will only work well when lossless compressed images are used as source, as lossy compression will add artefacts which are not necessarily consistent from image to image.

## Usage

    cgCompress [options] [files]

### Option - modifiers

    --format=XXX
Changes the image compression format. Replace XXX with the file extension of the wanted format, e.g. "png".

    --quality=X
Provides a file-size/compression-time trade-off. A value of 0 means maximum compression, while higher values will be faster, at the cost of potentially higher file-sizes. Currently 1 is the fastest value.

### Option - operations

    --help
Display quick help and exit

    --auto
Automatically tries to split the input files into several cgCompress files based on visual difference. Note that it will expand folders. It only compares files next to each other, so it requires the files to be in order. See `--combined` to manually fix those files which were split incorrectly.

    --extract
Extract the images in cgCompress files to their original state. Use `--format` to change output file format. Currently requires qt5-cgcompress-plugin to be installed.

    --recompress
Extract and recompress cgCompress files. Useful for optimizing files which were created in an older version of cgCompress. Currently requires qt5-cgcompress-plugin to be installed.

    --combined
Extract and combines several cgCompress files into one file. Allows ordinary image files as well. Useful when `--auto` fails to combine files.

    --pack
Create a cgCompress file from a directory containing an un-zipped cgCompress file. OpenRaster sets some requirements on the structure of the zip archive such as file order and compression settings, use this option to get it correct.

    --noalpha
Ignore the alpha channel in the input images. Some CG collections have a small amount of transparency around the edges for no good apparent reason.

    --discard-transparent
Removes any color value for fully transparent pixels and sets it to black. This can improve compression a bit, but is only visually lossless.

## Status

- Prof of concept
- A greedy algorithm have been implemented which is O( n^2 ) instead of O( n^n ), but it does not guaranty optimal solutions. It does however produce pretty good results, but it needs to be evaluated.
- Segmentation needs to be redone, and done with respect to file size.
- Some images contain the same image, but with color changes. Some success have been had with alternative composite methods, but still needs to be further investigated.

## Output format

A custom image format is used for output. It is based on OpenRaster with the following modifications:
- Multiple images are supported, which is done by interpreting <stack> elements which are direct descendants of the root <image> element as separate images. When the specification for MultiplePages in OpenRaster appear, this format might be updated to reflect that. Thus, **this format might change!**
- Thumbnails may use other formats than PNG.
- mergedimage.png is not required. We don't want to waste space on that.

### Support

Qt5 plug-in for loading cgCompress images can be found here: https://github.com/spillerrec/qt5-cgcompress-plugin

For thumbnail support in Windows, you can use the following extension: https://github.com/spillerrec/shell-ora-extension

Notice that not any OpenRaster thumbnailer will work, as cgCompress allows for thumbnails in any format, while the OpenRaster specification requires the use of PNG.

## Dependencies

- Qt (Qt5 have been used for development, Qt4 is untested)
- C++11 compiler support
- qmake (for building)
- zlib
- Boost (headers only)

*Recommended*:

A Qt plug-in with lossless webp output for higher compression. It is assumed that a quality of 100 will produce lossless compressed images. Qualities below 100 may produce lossy compressed images and this can be used for the thumbnails. If quality 100 *does not* produce lossless images, the result *will* be completely destroyed.

Plug-ins for writing WebP images which supports lossless can be found here:
- Qt4: https://github.com/stevenyao/webp
- Qt5: https://github.com/spillerrec/qt5-webp-plugin

