cgCompress
==========

Efficiently store Visual Novel cgs by using a multi-image format for storing all variations of one image in one file. This way, only the differences need to be stored which reduces the file size significantly.
cgCompress is an attempt to automatize this process, and reduce file size as much as possible by optimizing the configuration of differences.
*NOTE*: This will only work well when lossless compressed images are used as source, as lossy compression will add artefacts which are not necessarily consistent from image to image.

## Status

- Prof of concept
- A greedy algorithm have been implemented which is O( n^2 ) instead of O( n^n ), but it does not guaranty optimal solutions. It does however produce pretty good results, but it needs to be evaluated.
- Segmentation needs to be redone, and done with respect to file size.
- Some images contain the same image, but with color changes. Some success have been had with alternative composite methods, but still needs to be further investigated.
- The input images must currently not contain alpha, which means we can't optimize character sprites. Make it work!
- Check if the reconstructed images are correct. Lossless is 100%, not 99.999% because of programmer stupidity.
- A proper interface also needs to be made

## Output format

A custom image format is used for output. It is based on OpenRaster with the following modifications:
- Multiple images are supported, which is done by interpreting <stack> elements which are direct descendants of the root <image> element as separate images. When the specification for MultiplePages in OpenRaster appear, this format might be updated to reflect that. Thus, **this format might change!**
- Thumbnails may use other formats than PNG.
- mergedimage.png is not required. We don't want to waste space on that.

### Support

Qt5 plug-in for loading cgCompress images can be found here: https://github.com/spillerrec/qt5-cgcompress-plugin

I have made a Windows thumbnailer for support in Windows (file) explorer, but it is not release worthy in my opinion. If requested I can try to fix it up and release the code.

## Dependencies

- Qt (Qt5 have been used for development, Qt4 is untested)
- C++11 compiler support
- qmake (for building)
- zlib

*Recommended*:

A Qt plug-in with lossless webp output for higher compression. It is assumed that a quality of 100 will produce lossless compressed images. Qualities below 100 may produce lossy compressed images and this can be used for the thumbnails. If quality 100 *does not* produce lossless images, the result *will* be completely destroyed.

Plug-ins for writing WebP images which supports lossless can be found here:
- Qt4: https://github.com/stevenyao/webp
- Qt5: https://github.com/spillerrec/qt5-webp-plugin

