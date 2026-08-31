# TinyDistro AppBuilder

TinyDistro AppBuilder is the application development toolkit for TinyDistro v2.3.0.

It contains two companion tools:

- tmakegen
- tsdkgen

tmakegen generates Makefiles from TinyDistro configuration files.

tsdkgen generates tinysdk.h for TinyDistro applications.

## Version

TinyDistro AppBuilder v2.3.0

TinyDistro v2.3.0

## Project Structure

TinyDistro-AppBuilder/
├── src/
│   ├── tmakegen.c
│   ├── tsdkgen.c
│   └── common.c
├── include/
│   └── common.h
├── templates/
│   └── tinysdk.h
├── build/
│   └── generated applications
├── Makefile
├── README.md
└── LICENSE

## tmakegen

tmakegen generates Makefiles for C and C++ TinyDistro applications.

A basic application can contain:

main.c
tinysdk.h
tinydistro.conf

Example configuration:

DEFINE: CC = C
DEFINE: HD = HEADER

BUILD: main.c $ CC
HEADER: tinysdk.h $ HD

!END.

tmakegen reads the configuration and generates a Makefile according to the configured build rules.

## tsdkgen

tsdkgen generates tinysdk.h.

The generated SDK header provides the interface required by TinyDistro applications.

The SDK can provide access to:

- Application functions
- Console functions
- Filesystem functions
- Process functions
- Memory functions
- System functions
- Network functions
- Device functions
- Time functions

## Shared Code

tmakegen and tsdkgen are separate executables in the same project.

Shared functionality is implemented in:

src/common.c
include/common.h

The shared code prevents duplicated utility implementations between the two tools.

## Generated Applications

Generated applications are stored inside:

build/

The AppBuilder development files remain separate from generated application output.

Example:

build/
└── hello/
    ├── main.c
    ├── tinysdk.h
    ├── tinydistro.conf
    ├── Makefile
    └── hello

## Application Workflow

Create an application directory:

mkdir -p build/hello

Enter the directory:

cd build/hello

Create the source:

nano main.c

Generate the SDK:

../../build/tsdkgen

Generate the Makefile:

../../build/tmakegen

Build the application:

make

## Example Application

#include "tinysdk.h"

int main(int argc, char **argv)
{
    TSDK_UNUSED_ARG(argc);
    TSDK_UNUSED_ARG(argv);

    tsdk_init();

    tsdk_puts("Hello from TinyDistro!");

    tsdk_shutdown();

    return 0;
}

## Building AppBuilder

Build both tools with:

make

The resulting executables are:

build/tmakegen
build/tsdkgen

Both tools are linked with the shared common implementation.

## Manual Compilation

Build tmakegen:

gcc -std=c11 -Wall -Wextra -Wpedantic -Iinclude src/tmakegen.c src/common.c -o build/tmakegen

Build tsdkgen:

gcc -std=c11 -Wall -Wextra -Wpedantic -Iinclude src/tsdkgen.c src/common.c -o build/tsdkgen

## SDK Template

The SDK template is stored at:

templates/tinysdk.h

tsdkgen uses the template as the basis for generated TinyDistro SDK headers.

The default generated header is:

build/tinysdk.h

## Configuration

The TinyDistro configuration file is:

tinydistro.conf

Example:

DEFINE: CC = C
DEFINE: HD = HEADER
DEFINE: DISTRO = TinyDistro
DEFINE: DISTRO_VERSION = 2.3.0
DEFINE: SDK_VERSION = 2.3.0

BUILD: main.c $ CC
HEADER: tinysdk.h $ HD

!END.

## C Support

tmakegen supports C source files:

.c

## C++ Support

tmakegen supports C++ source files:

.cpp
.cc
.cxx

## Header Support

Header files can use:

.h
.hh
.hpp
.hxx

## Design Goals

### Small

Generated applications should only contain the components required by the application.

The complete AppBuilder does not need to be installed inside the final TinyDistro filesystem.

### Simple

The configuration format is intentionally small.

DEFINE
BUILD
HEADER
!END.

### Native

Applications are compiled using normal C and C++ toolchains.

### Extensible

Additional configuration directives and SDK APIs can be added in future releases.

### Shared

tmakegen and tsdkgen share common functionality through common.c and common.h.

## TinyDistro Filesystem

The final TinyDistro filesystem does not need to contain:

tmakegen
tsdkgen
templates/
src/
include/

These are development components.

Only the generated application and its required runtime components need to be included in the final filesystem.

## Architecture

TinyDistro configuration
        |
        +-------------------+
        |                   |
        v                   v
    tmakegen             tsdkgen
        |                   |
        v                   v
    Makefile            tinysdk.h
        |                   |
        +---------+---------+
                  |
                  v
             C or C++ app
                  |
                  v
               build/

## TinyDistro v2.3.0

TinyDistro AppBuilder is introduced with TinyDistro v2.3.0.

The AppBuilder provides a lightweight development workflow for building applications specifically for TinyDistro.

Current tools:

tmakegen
tsdkgen

Current release:

2.3.0

## License

See LICENSE for licensing information.

## Status

TinyDistro AppBuilder v2.3.0 is under active development.
