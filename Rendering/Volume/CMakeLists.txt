set(classes
  vtkDirectionEncoder
  vtkEncodedGradientEstimator
  vtkEncodedGradientShader
  vtkFiniteDifferenceGradientEstimator
  vtkFixedPointRayCastImage
  vtkFixedPointVolumeRayCastCompositeGOHelper
  vtkFixedPointVolumeRayCastCompositeGOShadeHelper
  vtkFixedPointVolumeRayCastCompositeHelper
  vtkFixedPointVolumeRayCastCompositeShadeHelper
  vtkFixedPointVolumeRayCastHelper
  vtkFixedPointVolumeRayCastMapper
  vtkFixedPointVolumeRayCastMIPHelper
  vtkGPUVolumeRayCastMapper
  vtkMultiVolume
  vtkOSPRayVolumeInterface
  vtkAnariVolumeInterface
  vtkProjectedTetrahedraMapper
  vtkRayCastImageDisplayHelper
  vtkRecursiveSphereDirectionEncoder
  vtkSphericalDirectionEncoder
  vtkUnstructuredGridBunykRayCastFunction
  vtkUnstructuredGridHomogeneousRayIntegrator
  vtkUnstructuredGridLinearRayIntegrator
  vtkUnstructuredGridPartialPreIntegration
  vtkUnstructuredGridPreIntegration
  vtkUnstructuredGridVolumeMapper
  vtkUnstructuredGridVolumeRayCastFunction
  vtkUnstructuredGridVolumeRayCastIterator
  vtkUnstructuredGridVolumeRayCastMapper
  vtkUnstructuredGridVolumeRayIntegrator
  vtkUnstructuredGridVolumeZSweepMapper
  vtkVolumeMapper
  vtkVolumeOutlineSource
  vtkVolumePicker
  vtkVolumeRayCastSpaceLeapingImageFilter)

set(headers
  vtkBlockSortHelper.h)

vtk_module_add_module(VTK::RenderingVolume
  CLASSES ${classes}
  HEADERS ${headers})
vtk_add_test_mangling(VTK::RenderingVolume)
