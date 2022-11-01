# external-process
#### _Framework for interaction with external Win32 processes._

### Navigation
- [/src](/src) - source code of the framework being developed
- [/test](/test) - everything you need to write and run tests
- [/examples](/examples) - examples of using the developed framework

### Functionality
- reading/writing the memory of external process
- allocation/deallocation of memory of a external process
- calling functions of a external process (currently functions using cdecl, stdcall, MSVC thiscall call conventions are supported)
- code injection in various ways
- search in external process memory by signature

### Usage
Add [/src/external_process.hpp](https://github.com/ep1h/external-process/blob/master/src/external_process.hpp), [/src/external_process.cpp](https://github.com/ep1h/external-process/blob/master/src/external_process.cpp) to your project. Inherit from the ExternalProcess class and implement the functionality you need. Examples of using the developed framework can be found here: [/examples](https://github.com/ep1h/external-process/blob/master/examples).

### Testing
Everything you need to write and run tests is in the [/test](/test) directory.
Since the main part of the developed functionality is based on the interaction between two Win32 processes, most of the tests require a victim application with which the developed framework will interact. For these purposes, the [/test/external_process_simulator](/test/external_process_simulator) simulator was written, which is compiled into a 32-bit PE file. The engine for writing tests, tests and auxiliary functionality for them are located in the [/test/unit_tests](test/unit_tests) directory. The tests are also compiled into a 32-bit PE file. When you run this file, it will launch the simulator and start executing tests by interacting with it.

To run the tests, you need to run the ```make run_tests``` command. As a result of executing this command, the external process simulator will first be built (the **external_process_simulator.exe** executable file will be located at the path *test/external_process_simulator/bin*). Then the tests will be assembled (the executable file **test.exe** along the path *test/unit_tests/bin*). Then the link to **external_process_simulator.exe** will be moved to *test/unit_tests/bin*. After that, test.exe will start and the tests will run. All the described logic is implemented in (Makefile)[/Makefile] in the root of the repository.

Test results (names of tests, statistics of passed/failed tests) will be displayed in the console. Also, upon completion, test.exe will return a value equal to the number of failed tests.

Also, it is possible to run the tests manually, not through the Makefile. To do this, you need to place **external_process_simulator.exe** and **test.exe** in the same directory and run **test.exe**.