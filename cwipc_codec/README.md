> Copyright (c) 2017-2026, Stichting Centrum Wiskunde en Informatica (CWI).

This repository is a compression library for `cwipc` pointcloud objects. It is part of the `cwipc` suite, <https://github.com/cwi-dis/cwipc>, and should generally be installed or built as part of that suite. 

See [the cwipc readme file](../readme.md) for details.

# cwipc_codec

This distribution contains a compressor and decompressor for pointclouds. It uses the _cwipc_ abstract object to represent pointclouds.

It is a modified version of the cwi-pcl-codec distribution, from <http://github.com/cwi-dis/cwi-pcl-codec>. Both distributions share the same codec, described in the following journal paper:

> _(R. Mekuria, K. Blom, and P. Cesar, "Design, Implementation and Evaluation of a Point Cloud Codec for Tele-Immersive Video," IEEE Transactions on Circuits and Systems for Video Technology, 27(4): 828 -842, 2017_

When cwipc_codec is built from source (as part of the _cwipc_ suite) it is possible to use the codec using the same API as specified in that paper and in the _cwi-pcl-codec_ repository. Simply add the include (and possibly source) directory to your project.
