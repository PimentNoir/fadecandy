Fadecandy Server
================

The Fadecandy Server is a background process that handles the USB communications with one or more Fadecandy Controller boards.

You can send pixel data to a Fadecandy Server over the Open Pixel Control protocol, or from a web app via WebSockets. See the 'doc' directory for details on all protocols supported.

The Fadecandy Server optionally takes configuration options in the form of a JSON config file. Configuration files allow you to do things like:

* Support multiple Fadecandy boards
* Mix Fadecandy and DMX lighting devices
* Listen on an alternate TCP port
* Listen for connections from the network, not just from local programs

The configuration file format is documented in the **doc** directory.

When you run the Fadecandy Server, it will provide a simple web interface. By default, the Fadecandy server runs at [http://localhost:7890](http://localhost:7890).

Build
-----

Pre-built binaries are included in the **bin** directory, but you can also build it yourself. All required libraries are included as git submodules.

It can build on Windows, Mac OS, or Linux using Make and other command line tools. On Windows, the build uses MinGW and gcc.


Getting Started
---------------

In order to build the binary from source you need to run the following commands inside of the **server** directory:

```bash
$ make submodules
$ make
```

The compiled binary will be created in the same **server** directory

If you want to remove the compiled binary and source files run:

```bash
$ make clean
```


Build using CMake
-----------------

The CMake project supports building a Debian package including a SystemD service. Run the following commands in the **server** directory to build using it.

To build the binaries and get a Debian package:


```bash
$ make submodules # Fetches submodule git repositories
$ mkdir build
$ cd build
$ cmake ..
$ make          # Builds the binaries.

$ cpack -G DEB  # Generates the debian package.
```

To list CMake options:

```bash
$ cmake -LH ..
```

You can also install on the system without doing it via a Debian package:
```bash
$ make install
```
