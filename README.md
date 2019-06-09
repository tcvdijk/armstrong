# Armstrong

_Thomas C. van Dijk and Andre Löffler_

## Description

Topologically-safe rounding of geographic networks.

## Running Armstrong

See ``armstrong --help``.

## Building Armstrong

Armstrong has been tested on Windows (Visual Studio 2019) and Linux (```gcc-8.3.0```), but no general build script is provided at this moment.
It should suffice to compile and link all ```.cpp``` files together, excluding ```docopt.cpp```.
Note that support for C++17 is required.

This repository includes convenience copies of the following libraries.

* Shapelib for reading shapefiles.
* Docopt for parsing command line options.

We recommmend ```vcpkg``` to handle the following dependencies.

* CGAL (```cgal```) for geometry and constrained Delaunay triangulations.
* Eigen (```eigen3```) for sparse linear algebra.
* spdlog and {fmt} (```spdlog```) for output.