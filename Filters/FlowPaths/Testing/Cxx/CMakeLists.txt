vtk_add_test_cxx(vtkFiltersFlowPathsCxxTests tests
  TestBSPTree.cxx
  TestBSPTreeWithGhostArrays.cxx
  TestCellLocatorsLinearTransform.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  TestEvenlySpacedStreamlines2D.cxx
  TestStreamTracer.cxx,NO_VALID
# TestStreamTracerSurface.cxx #19221
  TestStreamSurface.cxx
  TestAMRInterpolatedVelocityField.cxx,NO_VALID
  TestParallelVectors.cxx
  TestParticleTracers.cxx,NO_VALID
  TestLagrangianIntegrationModel.cxx,NO_VALID
  TestLagrangianParticle.cxx,NO_VALID
  TestLagrangianParticleTracker.cxx
  TestLagrangianParticleTrackerWithGravity.cxx,NO_VALID
  TestStreamTracerImplicitArray.cxx,NO_VALID
  TestVortexCore.cxx,NO_VALID
  TestVectorFieldTopology.cxx
  TestVectorFieldTopologyAMR.cxx
  TestVectorFieldTopologyNoIterativeSeeding.cxx
  )

vtk_test_cxx_executable(vtkFiltersFlowPathsCxxTests tests
  DISABLE_FLOATING_POINT_EXCEPTIONS
  RENDERING_FACTORY
  )
