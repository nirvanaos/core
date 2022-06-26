# Nirvana Core

## Content

This is a part of the Nirvana project.

This repository contains source code for the Nirvana core.

## Folders

### Source

Sources and headers for the CoreLib.
CoreLib is a library of different core classes that may be tested separately,
without the Core build.

### Exports

This folder contains sources which must be included directly into the core build.

### Test

Various tests.

## How to build
This repository does not contain any project files.
It must be included as a Git submodule in a Nirvana library project.
To build the library under Visual C++ use *nirvanaos/nirvana.vc* repository.
