# external-process
_A framework for interacting with external Win32 processes._

## Table of Contents
- [Repository structure](#Repository-structure)
- [Features](#Features)
- [Usage](#Usage)
- [Testing](#Testing)

## Repository Structure
- [/src](/src) - Source code of the framework
- [/test](/test) - Resources for writing and running tests
- [/examples](/examples) - Examples of using the framework

## Features
- Read and write to an external process memory
- Allocate and deallocate external process memory
- Call function of an external process (currently supports cdecl, stdcall, and MSVC thiscall calling conventions)
- Code injection using various methods
- Search for byte sequences in external process memory using signatures

## Usage
Add [/src/external_process.hpp](/src/external_process.hpp), [/src/external_process.cpp](/src/external_process.cpp) to your project. Inherit from the '__ExternalProcess__' class and implement the functionality you need.
You can find examples of how to use the framework in the [/examples](/examples) directory.

## Testing
The /test directory contains everything you need to write and run tests. As the primary functionality of the framework is based on the interaction between two Win32 processes, most tests require a "victim" application with which the framework will interact. For this purpose, the [/test/external_process_simulator](/test/external_process_simulator) simulator is provided, which compiles into a 32-bit PE file. The test engine, tests, and auxiliary functionality are located in the [/test/unit_tests](test/unit_tests) directory. Tests are also compiled into a 32-bit PE file. Running this file will launch the simulator and start executing tests through interaction with it.

To run the tests, execute the ```make run_tests``` command. This will build the external process simulator (output as **external_process_simulator.exe** in *test/external_process_simulator/bin*). Next, a link to **external_process_simulator.exe** will be moved to *test/unit_tests/bin*. Afterward, **test.exe** will start and the tests will run. The entire process is implemented in the (Makefile)[/Makefile] located in the repository root.

Test results, including test names and passed/failed test statistics, will be displayed in the console. Additionally, upon completion, **test.exe** will return a value equal to the number of failed tests.

You can also run tests manually without using the Makefile. To do this, place **external_process_simulator.exe** and **test.exe** in the same directory and execute **test.exe**

## Examples
<details>
  <summary>FlatOut 2 trainer</summary>
  https://user-images.githubusercontent.com/46194184/233863589-82d6642b-f4a2-4b80-a6d3-7517e2dcdec8.mp4
</details>

<details>
  <summary>GTA San Andreas trainer</summary>
  POV hack:
  https://user-images.githubusercontent.com/46194184/233863892-846cd69c-14e4-4293-ac94-a97a6f2ba99c.mp4
</details>
