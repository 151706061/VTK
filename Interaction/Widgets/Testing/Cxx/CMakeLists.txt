set(TestOrientedGlyphContour_ARGS "--PlaybackFile" "DATA{../Data/Input/TestOrientedGlyphContourEventLog.txt}")
set(TerrainPolylineEditor_ARGS -ProjectionMode 2 -HeightOffset 100)
vtk_add_test_cxx(vtkInteractionWidgetsCxxTests tests
  BoxWidget.cxx
  BoxWidget2.cxx
  ImagePlaneWidget.cxx
  ImagePlaneWidgetOriented.cxx
  TerrainPolylineEditor.cxx
  TestAffineWidget.cxx
  TestAngleWidget2D.cxx
  TestAngleWidget3D.cxx
  TestBalloonWidget.cxx
  TestBiDimensionalWidget.cxx,LOOSE_VALID
  TestBorderWidget.cxx
  TestBrokenLineWidget.cxx
  TestCamera3DWidget.cxx
  TestCameraWidget.cxx
  TestCaptionWidget.cxx
  TestCellCentersPointPlacer.cxx
  TestCenteredSliderWidget2D.cxx
  TestCheckerboardWidget.cxx
  TestCoincidentHandleWidgets.cxx
  TestCompassWidget.cxx
  TestConstrainedHandleWidget.cxx
  TestContourWidget2.cxx
  TestCoordinateFrameWidget.cxx
  TestDijkstraImageGeodesicPath.cxx
  TestDisplaySizedImplicitPlaneWidget.cxx
  TestDistanceWidget.cxx,LOOSE_VALID
  TestDistanceWidget3D.cxx
  TestFinitePlaneWidget.cxx
  TestFixedSizeHandleRepresentation3D.cxx
  TestFocalPlaneContour.cxx
  TestHandleWidget.cxx
  TestHandleWidget2D.cxx
  TestImageActorContourWidget.cxx
  TestImplicitAnnulusRadius.cxx
  TestImplicitAnnulusWidget.cxx
  TestImplicitConeWidget.cxx
  TestImplicitCylinderWidget.cxx
  TestImplicitCylinderWidget2.cxx
  TestImplicitFrustumWidget.cxx
  TestImplicitPlaneWidget.cxx
  TestImplicitPlaneWidget2.cxx
  TestImplicitPlaneWidget2LockNormalToCamera.cxx
  TestImplicitPlaneWidget2b.cxx
  TestImplicitPlaneWidget3.cxx
  TestLightWidget.cxx
  TestLogoWidget.cxx
  TestLogoWidgetAlphaBlending.cxx
  TestLogoWidgetDepthPeeling.cxx
  TestLineWidget2b.cxx,NO_VALID
  TestMeasurementCubeHandleRepresentation3D.cxx
  TestMultipleViewports.cxx
  TestOrientationMarkerWidget.cxx
  TestOrientationMarkerWidget2.cxx
  TestOrientationMarkerWidgetRepeatedAdd.cxx
  TestOrientationWidget.cxx
  TestOrthoPlanes.cxx
  TestParallelopipedWidget.cxx
  TestPickingManagerSeedWidget.cxx
  TestPickingManagerSeedWidget2.cxx
  TestPlaybackWidget.cxx
  TestPointHandleRepresentation3D.cxx
  TestPolylineWidget.cxx
  TestProgrammaticPlacement.cxx
  TestProgressBarWidget.cxx
  TestProp3DButtonRepresentation.cxx,NO_VALID
  TestRectilinearWipeWidget.cxx
  TestResliceCursorWidget2.cxx
  TestResliceCursorWidget3.cxx
  TestScalarBarWidget.cxx,LOOSE_VALID
  TestSeedWidget.cxx
  TestSeedWidget2.cxx
  TestSeedWidgetNonUniformRepresentations.cxx
  TestSliderWidget.cxx
  TestSliderWidget2D.cxx
  TestSliderWidgetMultipleViewports.cxx
  TestSphereWidget2CenterCursor.cxx
  TestSphereWidgetZoomInOut.cxx
  TestSplineWidget.cxx
  TestTextWidget.cxx
  TestTextRepresentationWithBorders.cxx
  TestTextWidgetBackgroundInteractive.cxx
  )
vtk_add_test_cxx(vtkInteractionWidgetsCxxTests no_valid_tests
  NO_VALID
  TestEqualizerContextItem.cxx
  vtkAngleRepresentation2DTest1.cxx
  vtkAngleRepresentation3DTest1.cxx
  vtkAngleWidgetTest1.cxx
  vtkBalloonRepresentationTest1.cxx
  vtkBalloonWidgetTest1.cxx
  vtkBiDimensionalRepresentation2DTest1.cxx
  vtkBiDimensionalWidgetTest1.cxx
  vtkBorderWidgetTest1.cxx
  vtkCameraPathRepresentationTest1.cxx
  vtkCaptionRepresentationTest1.cxx
  vtkCaptionWidgetTest1.cxx
  vtkConstrainedPointHandleRepresentationTest1.cxx
  vtkHoverWidgetTest1.cxx
  vtkImplicitPlaneRepresentationTest1.cxx
  vtkImplicitPlaneWidget2Test1.cxx
  vtkLineRepresentationTest1.cxx
  vtkLineWidget2Test1.cxx
  vtkPointHandleRepresentation2DTest1.cxx
  # vtkPointHandleRepresentation3DTest1.cxx disabled
  vtkPolyLineRepresentationTest1.cxx
  vtkPolyLineWidgetTest1.cxx
  vtkResliceCursorRepresentationTest1.cxx
  vtkSeedRepresentationTest1.cxx
  vtkSeedWidgetTest1.cxx
  # vtkSphereHandleRepresentationTest1.cxx disabled
  vtkSplineRepresentationTest1.cxx
  vtkSplineWidget2Test1.cxx
  vtkTextRepresentationTest1.cxx
  vtkTextWidgetTest1.cxx
  )

# Tests with data
vtk_add_test_cxx(vtkInteractionWidgetsCxxTests data_tests
  TestButtonWidget.cxx
  TestButtonWidgetMultipleViewports.cxx
  TestButtonWidgetPlacement.cxx
  TestDijkstraGraphGeodesicPath.cxx
  TestLineWidget.cxx
  TestLineWidget2.cxx
  TestOrientedGlyphContour.cxx
  TestPlaneWidget.cxx
  TestPointWidget.cxx
  TestPolyPlane.cxx,NO_VALID
  TestPolygonalHandleRepresentations.cxx
  TestPolygonalRepresentationHandleWidget.cxx
  TestPolygonalSurfaceConstrainedDistanceWidget.cxx
  TestSphereHandleWidget.cxx
  TestSurfaceConstrainedHandleWidget.cxx
  TestTensorProbeWidget.cxx
  TestImageTracerWidget.cxx
  )

vtk_add_test_cxx(vtkInteractionWidgetsCxxTests ogl_tests
  TestPickingManagerWidgets.cxx
  )

set(all_tests
  ${tests}
  ${no_valid_tests}
  ${data_tests}
  ${ogl_tests}
  )
vtk_test_cxx_executable(vtkInteractionWidgetsCxxTests all_tests
  DISABLE_FLOATING_POINT_EXCEPTIONS
  RENDERING_FACTORY
  )

# Allow local customization of timeout for this test.
if(NOT VTK_TEST_TIMEOUT_TerrainPolylineEditor)
  set(VTK_TEST_TIMEOUT_TerrainPolylineEditor 350)
endif()
set_tests_properties(VTK::InteractionWidgetsCxx-TerrainPolylineEditor
  PROPERTIES TIMEOUT ${VTK_TEST_TIMEOUT_TerrainPolylineEditor})

# Allow local customization of timeout for this test.
if(NOT VTK_TEST_TIMEOUT_TestPickingManagerSeedWidget)
  set(VTK_TEST_TIMEOUT_TestPickingManagerSeedWidget 400)
endif()
set_tests_properties(VTK::InteractionWidgetsCxx-TestPickingManagerSeedWidget
  PROPERTIES TIMEOUT ${VTK_TEST_TIMEOUT_TestPickingManagerSeedWidget})

# On some machines this test takes ~220 seconds to complete, allow for some
# extra time
if(NOT VTK_TEST_TIMEOUT_TestSphereHandleWidget)
  set(VTK_TEST_TIMEOUT_TestSphereHandleWidget 300)
endif()
set_tests_properties(VTK::InteractionWidgetsCxx-TestSphereHandleWidget
    PROPERTIES TIMEOUT ${VTK_TEST_TIMEOUT_TestSphereHandleWidget}
               RUN_SERIAL ON)

# TestOrthoPlanes can take a long time
if(NOT VTK_TEST_TIMEOUT_TestOrthoPlanes)
  set(VTK_TEST_TIMEOUT_TestOrthoPlanes 300)
endif()
set_tests_properties(VTK::InteractionWidgetsCxx-TestOrthoPlanes
    PROPERTIES TIMEOUT ${VTK_TEST_TIMEOUT_TestOrthoPlanes}
               RUN_SERIAL ON)
