cgCompress
==========

Efficiently store Visual Novel cgs by using a multi-image format for storing all variations of one image in one file. This way, only the differences need to be stored which reduces the file size significantly.
cgCompress is an attempt to automatize this process, and reduce file size as much as possible by optimizing the configuration of differences.
NOTE: This will only work well when lossless compressed images are used as source, as lossy compression will add artefacts which are not necessarily consistent from image to image.

Output is based on the OpenRaster format, but since the proposal to add MultiplePages to the format appears to be far from stabilized, a custom variation is used. As of such, the output format is UNSTABLE and not guaranteed to be supported in the future. For experimentation uses only (for now)!

Status
------

Prof of concept

Performance needs to be greatly improved to be practical. The optimization problem is exponential, and while several short-cuts are already implemented, performance is wacky. Currently some cgs might just not be possible to do in practical time, while others only takes a few seconds.

Work still needs to be done on segmentation. Segmenting out to very small areas can help file compression, however it kills performance because of the algorithmic complexity. However not segmenting enough gives makes it fail to perform some cross-frame optimizations. We need to separate this into two, segmenting for differentiating unique features, and segmenting for file size optimization.

A proper interface also needs to be made

Dependencies
------------

Qt5 + qmake

zlib

Recommended:
A Qt5 plug-in with lossless webp output for higher compression.