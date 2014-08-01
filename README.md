# libdvid-utils [![Picture](https://raw.github.com/janelia-flyem/janelia-flyem.github.com/master/images/gray_janelia_logo.png)](http://janelia.org/)
## C++ tools that work with DVID

Utilities that peform operations on data stored in [DVID](http://github.com/janelia-flyem/dvid).
It uses the [libdvid](http://github.com/janelia-flyem/libdvid-cpp) C++ library.

## Installation Instructions

libdvid-utils is currently only supported on linux operating systems.  It probably
would be relatively straightforward to build on a MacOS.

libdvid-utils supports two mechanisms for installation:

### Buildem (preferred)

The most-supported and most push-button approach involves using the build system
called [buildem](https://github.com/janelia-flyem/buildem).  The advantage
of using buildem is that it will automatically build all of the dependencies
and put it in a separate environment so as not to conflict with any other
builds.  The disadvantage of buildem is that it will
install a lot of package dependencies.  The initial build of all of the dependencies
could take on the order of an hour to complete.

To build libdvid-utils using buildem

    % mkdir build; cd build;
    % cmake .. -DBUILDEM_DIR=/user-defined/path/to/build/directory
    % make -j num_processors

This will automatically load the binaries into BUILDEM_DIR/bin.  To run
the test regressions, add this directory into your PATH environment and
run <i>make test</i> in the <i>build</i> directory.

### Package only (untested) 

libdvid-utils can be built without Buildem with the following:

    % mkdir build; cd build;
    % cmake ..
    % make

To successfully compile this code, you will need to install the following
dependencies and make sure their headers and libraries can be found in
the common linux search paths:

* jsoncpp
* boost
* libdvid-cpp

For more information on the supported versions of these dependencies, please
consult buildem.

To run <i>make test</i>, the installed libraries will need to be in the default
library search path or set in LD_LIBRARY_PATH. The
binaries produced will be placed in the libdvid-utils <i>bin</i> directory.


