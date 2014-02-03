cgCompress
==========

Efficiently store Visual Novel cgs by using a multi-image format for storing all variations of one image in one file. This way, only the differences need to be stored which reduces the file size significantly.
cgCompress is an attempt to automatize this process, and reduce file size as much as possible by optimizing the configuration of differences.
NOTE: This will only work well when lossless compressed images are used as source, as lossy compression will add artefacts which are not necessarily consistent from image to image.

Output will be based on the OpenRaster format, but since the proposal to add MultiplePages to the format appears to be far from stabilized, a custom variation is used. As of such, the output format is UNSTABLE and not guaranteed to be supported in the future. For experimentation uses only (for now)!

Status
------

Prof of concept in progress