set(classes
  vtkAbstractInterpolatedVelocityField
  vtkAMRInterpolatedVelocityField
  vtkCompositeInterpolatedVelocityField
  vtkEvenlySpacedStreamlines2D
  vtkLagrangianBasicIntegrationModel
  vtkLagrangianMatidaIntegrationModel
  vtkLagrangianParticle
  vtkLagrangianParticleTracker
  vtkLinearTransformCellLocator
  vtkModifiedBSPTree
  vtkParallelVectors
  vtkParticlePathFilter
  vtkParticleTracer
  vtkParticleTracerBase
  vtkStreaklineFilter
  vtkStreamSurface
  vtkStreamTracer
  vtkTemporalInterpolatedVelocityField
  vtkVectorFieldTopology
  vtkVortexCore)

set(nowrap_headers
  vtkLagrangianThreadedData.h)

vtk_module_add_module(VTK::FiltersFlowPaths
  CLASSES ${classes}
  NOWRAP_HEADERS ${nowrap_headers})
vtk_add_test_mangling(VTK::FiltersFlowPaths)
