# VTK WASM regression test suite architecture

The VTK Emscripten test utilities utilize a custom protocol designed to address the file I/O requirements of WebAssembly (WASM) unit tests. This protocol allows C++ unit tests to dump files from the sandbox to the server hosting the WASM module.

## Fixtures

The test suite requires a HTTP server to run in the background. Additionally, the tests expect to find a "vtkhttp.lock" file in the `${CMAKE_BINARY_DIR}/Testing/Temporary` directory.

This lock file must contain the URL of a HTTP server that the tests may connect to using web browser APIs. Right now, the connection is only made when a unit test needs to dump a file (like difference images)
to server filesystem.

A new fixture called "HTTP" is used to coordinate the startup and shutdown of the HTTP server.
The special unit test `HTTPServerStart` is run before all the unit tests that have "HTTP" in their `FIXTURES_REQUIRED`
property and finally the `HTTPServerStop` is run at the end to shutdown the HTTP server.

## Arguments

The command line for a unit test is set by the CMake functions in vtkModuleTesting.cmake file. It looks like
"cmake" -D... "CMake/vtkWasmTestRunner.cmake" "/path/to/vtkModuleNameCxxTests.js_file" "TestName" "arg1" "arg2" ... "argn"

## Routes:

1. `/dump?file=/path/to/file.ext`:

    **Request description**: wasm unit test initiates a POST with the full path of a file in its query parameters and sends the entire contents of the output file in the POST content.

    **Expected response**: No response needed if parameters are valid. 'Invalid query for /dump' if query parameters are invalid.

## Reference Client-side implementation in VTK:

1. [vtkEmscriptenTestUtilities.h](https://gitlab.kitware.com/vtk/vtk/-/blob/master/Testing/Core/vtkEmscriptenTestUtilities.h)
2. [vtkEmscriptenTestUtilities.cxx](https://gitlab.kitware.com/vtk/vtk/-/blob/master/Testing/Core/vtkEmscriptenTestUtilities.cxx)
3. [vtkEmscriptenTestUtilities.js](https://gitlab.kitware.com/vtk/vtk/-/blob/master/Testing/Core/vtkEmscriptenTestUtilities.js)

## Reference Server-side implementation in VTK:

The `server.js` implements the `/dump` request.
