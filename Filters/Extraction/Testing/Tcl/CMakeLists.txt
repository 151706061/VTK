if(VTK_DATA_ROOT)
  add_test_tcl(extractRectGrid Graphics)
  add_test_tcl(extractUGrid Graphics)
  add_test_tcl(extractVectors Graphics)
endif()
add_test_tcl(ExtractEdgesQuadraticCells Graphics)
add_test_tcl(ExtractTensors Graphics)
add_test_tcl(extractPolyData Graphics)
