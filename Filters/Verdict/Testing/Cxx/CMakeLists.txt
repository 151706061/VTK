vtk_add_test_cxx(vtkFiltersVerdictCxxTests tests
  NO_VALID
  CellSizeFilter.cxx
  CellSizeFilter2.cxx
  MeshQuality.cxx
  TestCellQuality.cxx
  TestBoundaryMeshQuality.cxx
  )
vtk_test_cxx_executable(vtkFiltersVerdictCxxTests tests)
