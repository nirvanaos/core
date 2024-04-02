# Nirvana Core

## Content

This is a part of the [Nirvana project](https://github.com/nirvanaos/home).

This repository contains source code for the Nirvana core.

## Folders

### Source

Sources and headers for the LibCore.
LibCore is a library of various core classes that may be tested separately,
before the Core binary build.

Requires boost/preprocessor library installed.

### Exports

This folder contains sources which must be included directly into the core binary build.

### Test

CoreLib unit tests.

### DecCalc

Decimal calculations module.

Requires [DecNumber library](https://github.com/nirvanaos/decNumber).

### SFloat

Software floating point module.

Uses as a submodule: [Berkley SoftFloat IEEE Floating-Point Arithmetic
Package, by John R. Hauser](https://github.com/ucb-bar/berkeley-softfloat-3).

### InterfaceRepository

Interface repository module.

### dbc

Database connectivity module.

### SQLite

SQLite database driver module.

## How to build
The Nirvana core executable builded from the 3 parts:
* LibCore
* C++ sources from the Exports folder
* Port library for the corresponding host

This repository does not contain any project files.
It must be included as a Git submodule in a Nirvana library project.
To build the library under Visual C++ use *nirvanaos/nirvana.vc* repository.
