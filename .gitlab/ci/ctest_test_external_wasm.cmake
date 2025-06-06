include("${CMAKE_CURRENT_LIST_DIR}/gitlab_ci.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/ctest_annotation.cmake")

set(annotation_report_file "${CTEST_BINARY_DIRECTORY}/annotations.json")
set(annotation_report)

# This is the path to a wasm execution engine. At the moment, VTK tests run in chrome.
if ("$ENV{VTK_TESTING_WASM_ENGINE}" STREQUAL "")
  message(FATAL_ERROR "The environment variable VTK_TESTING_WASM_ENGINE is not initilaized!")
else ()
  set(VTK_TESTING_WASM_ENGINE "$ENV{VTK_TESTING_WASM_ENGINE}")
endif ()

# Support wasm64 architecture.
set(VTK_WEBASSEMBLY_64_BIT OFF)
if ("$ENV{WASM_ARCHITECTURE}" MATCHES "wasm64")
  set(VTK_WEBASSEMBLY_64_BIT ON)
endif ()

# Use Emscripten toolchain
get_filename_component(emsdk_dir "${CMAKE_CURRENT_LIST_DIR}/../emsdk" ABSOLUTE)
file(TO_CMAKE_PATH "${emsdk_dir}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake" toolchain_file)

# set emdawnwebgpu dir
get_filename_component(_emdawnwebgpu_DIR "${CMAKE_CURRENT_LIST_DIR}/../dawn" ABSOLUTE)

set(CTEST_SOURCE_DIRECTORY "${CTEST_SOURCE_DIRECTORY}/Testing/ExternalWasm")
set(cmake_args
  "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${toolchain_file}"
  "-DCMAKE_BUILD_TYPE:STRING=Release"
  "-DCMAKE_PREFIX_PATH:PATH=${CTEST_BINARY_DIRECTORY}/install"
  "-DVTK_USE_LARGE_DATA:BOOL=ON"
  "-DVTK_BUILD_TESTING:BOOL=ON"
  "-DVTK_WEBASSEMBLY_64_BIT:BOOL=${VTK_WEBASSEMBLY_64_BIT}"
  "-DVTK_TESTING_WASM_ENGINE=${VTK_TESTING_WASM_ENGINE}"
  "-Demdawnwebgpu_DIR=${_emdawnwebgpu_DIR}/lib/cmake/emdawnwebgpu"
  "--no-warn-unused-cli")

# Create an entry in CDash.
ctest_start(Experimental TRACK "${ctest_track}")

# Gather update information.
find_package(Git)
set(CTEST_UPDATE_VERSION_ONLY ON)
set(CTEST_UPDATE_COMMAND "${GIT_EXECUTABLE}")
ctest_update()

# Configure the project.
ctest_configure(
  OPTIONS "${cmake_args}"
  RETURN_VALUE configure_result)

# Read the files from the build directory.
ctest_read_custom_files("${CTEST_BINARY_DIRECTORY}")

# We can now submit because we've configured. This is a cmb-superbuild-ism.
ctest_submit(PARTS Update)
ctest_submit(PARTS Configure
  BUILD_ID build_id)

if (DEFINED build_id)
  list(APPEND annotation_report
    "Build Summary" "https://open.cdash.org/build/${build_id}"
    "Update" "https://open.cdash.org/build/${build_id}/update"
    "Configure" "https://open.cdash.org/build/${build_id}/configure"
  )
endif ()

if (configure_result)
  ctest_annotation_report("${annotation_report_file}" ${annotation_report})
  ctest_submit(PARTS Done)
  message(FATAL_ERROR
    "Failed to configure")
endif ()

include(ProcessorCount)
ProcessorCount(nproc)
if (NOT "$ENV{CTEST_MAX_PARALLELISM}" STREQUAL "")
  if (nproc GREATER "$ENV{CTEST_MAX_PARALLELISM}")
    set(nproc "$ENV{CTEST_MAX_PARALLELISM}")
  endif ()
endif ()

if (CTEST_CMAKE_GENERATOR STREQUAL "Unix Makefiles")
  set(CTEST_BUILD_FLAGS "-j${nproc} -l${nproc}")
elseif (CTEST_CMAKE_GENERATOR MATCHES "Ninja")
  set(CTEST_BUILD_FLAGS "-l${nproc}")
endif ()

if (CTEST_CMAKE_GENERATOR MATCHES "Make")
  # Drop the `-i` flag without remorse.
  set(CTEST_BUILD_COMMAND "make ${CTEST_BUILD_FLAGS}")
endif ()

ctest_build(
  NUMBER_ERRORS num_errors
  NUMBER_WARNINGS num_warnings
  RETURN_VALUE    build_result
  ${build_args})

ctest_submit(PARTS Build)

if (DEFINED build_id)
  list(APPEND annotation_report
      "Build Errors (${num_errors})" "https://open.cdash.org/viewBuildError.php?buildid=${build_id}"
      "Build Warnings (${num_warnings})" "https://open.cdash.org/viewBuildError.php?type=1&buildid=${build_id}"
  )
endif ()

if (build_result)
  file(GLOB logs
    "${CTEST_SOURCE_DIRECTORY}/compile_output.log")
  if (logs)
    list(APPEND CTEST_NOTES_FILES ${logs})
    ctest_submit(PARTS Notes)
  endif ()
endif ()

if (build_result)
  ctest_annotation_report("${annotation_report_file}" ${annotation_report})
  ctest_submit(PARTS Done)
  message(FATAL_ERROR
    "Failed to build")
endif ()

file(WRITE "${CTEST_SOURCE_DIRECTORY}/compile_num_warnings.log" "${num_warnings}")

# Default to a reasonable test timeout.
set(CTEST_TEST_TIMEOUT 100)

include("${CMAKE_CURRENT_LIST_DIR}/ctest_exclusions.cmake")
ctest_test(APPEND
  PARALLEL_LEVEL "${nproc}"
  TEST_LOAD "${nproc}"
  RETURN_VALUE test_result
  EXCLUDE "${test_exclusions}"
  OUTPUT_JUNIT "${CTEST_BINARY_DIRECTORY}/junit.xml"
  REPEAT UNTIL_PASS:3)
ctest_submit(PARTS Test)

if (DEFINED build_id)
  list(APPEND annotation_report
    "All Tests"     "https://open.cdash.org/viewTest.php?buildid=${build_id}"
    "Test Failures" "https://open.cdash.org/viewTest.php?onlyfailed&buildid=${build_id}"
    "Tests Not Run" "https://open.cdash.org/viewTest.php?onlynotrun&buildid=${build_id}"
    "Test Passes"   "https://open.cdash.org/viewTest.php?onlypassed&buildid=${build_id}"
  )
endif ()

if (test_result)
  ctest_annotation_report("${annotation_report_file}" ${annotation_report})
  ctest_submit(PARTS Done)
  message(FATAL_ERROR
    "Failed to test")
endif ()

ctest_annotation_report("${annotation_report_file}" ${annotation_report})
ctest_submit(PARTS Done)
