vtk_module_test_data(
  Data/clt.vtm
  Data/clt/,REGEX:.*
  Data/ParticleTracers/pathline.vtp
  Data/ParticleTracers/streakline.vtp
  Data/ParticleTracers/tracer.vtp)

if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif ()

if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/combq.bin
    Data/combxyz.bin
  )
  add_subdirectory(Python)
endif ()
