#!/usr/bin/env python
from vtkmodules.vtkCommonTransforms import vtkTransform
from vtkmodules.vtkIOImage import vtkImageReader
from vtkmodules.vtkImagingCore import vtkImageReslice
from vtkmodules.vtkRenderingCore import (
    vtkActor2D,
    vtkImageMapper,
    vtkRenderWindow,
    vtkRenderer,
)
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# this script tests vtkImageReslice with different interpolation modes
# and with a rotation
# Image pipeline
reader = vtkImageReader()
reader.ReleaseDataFlagOff()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,63,0,63,1,93)
reader.SetDataSpacing(3.2,3.2,1.5)
reader.SetFilePrefix(VTK_DATA_ROOT + "/Data/headsq/quarter")
reader.SetDataMask(0x7fff)
transform = vtkTransform()
# rotate about the center of the image
transform.Translate(+100.8,+100.8,+69.0)
transform.RotateWXYZ(10,1,1,0)
transform.Translate(-100.8,-100.8,-69.0)
reslice1 = vtkImageReslice()
reslice1.SetInputConnection(reader.GetOutputPort())
reslice1.SetResliceTransform(transform)
reslice1.SetInterpolationModeToCubic()
reslice1.SetOutputSpacing(0.65,0.65,1.5)
reslice1.SetOutputOrigin(80,120,40)
reslice1.SetOutputExtent(0,63,0,63,0,0)
reslice2 = vtkImageReslice()
reslice2.SetInputConnection(reader.GetOutputPort())
reslice2.SetResliceTransform(transform)
reslice2.SetInterpolationModeToLinear()
reslice2.SetOutputSpacing(0.65,0.65,1.5)
reslice2.SetOutputOrigin(80,120,40)
reslice2.SetOutputExtent(0,63,0,63,0,0)
reslice3 = vtkImageReslice()
reslice3.SetInputConnection(reader.GetOutputPort())
reslice3.SetResliceTransform(transform)
reslice3.SetInterpolationModeToNearestNeighbor()
reslice3.SetOutputSpacing(0.65,0.65,1.5)
reslice3.SetOutputOrigin(80,120,40)
reslice3.SetOutputExtent(0,63,0,63,0,0)
reslice4 = vtkImageReslice()
reslice4.SetInputConnection(reader.GetOutputPort())
reslice4.SetResliceTransform(transform)
reslice4.SetInterpolationModeToLinear()
reslice4.SetOutputSpacing(3.2,3.2,1.5)
reslice4.SetOutputOrigin(0,0,40)
reslice4.SetOutputExtent(0,63,0,63,0,0)
mapper1 = vtkImageMapper()
mapper1.SetInputConnection(reslice1.GetOutputPort())
mapper1.SetColorWindow(2000)
mapper1.SetColorLevel(1000)
mapper1.SetZSlice(0)
mapper2 = vtkImageMapper()
mapper2.SetInputConnection(reslice2.GetOutputPort())
mapper2.SetColorWindow(2000)
mapper2.SetColorLevel(1000)
mapper2.SetZSlice(0)
mapper3 = vtkImageMapper()
mapper3.SetInputConnection(reslice3.GetOutputPort())
mapper3.SetColorWindow(2000)
mapper3.SetColorLevel(1000)
mapper3.SetZSlice(0)
mapper4 = vtkImageMapper()
mapper4.SetInputConnection(reslice4.GetOutputPort())
mapper4.SetColorWindow(2000)
mapper4.SetColorLevel(1000)
mapper4.SetZSlice(0)
actor1 = vtkActor2D()
actor1.SetMapper(mapper1)
actor2 = vtkActor2D()
actor2.SetMapper(mapper2)
actor3 = vtkActor2D()
actor3.SetMapper(mapper3)
actor4 = vtkActor2D()
actor4.SetMapper(mapper4)
imager1 = vtkRenderer()
imager1.AddViewProp(actor1)
imager1.SetViewport(0.5,0.0,1.0,0.5)
imager2 = vtkRenderer()
imager2.AddViewProp(actor2)
imager2.SetViewport(0.0,0.0,0.5,0.5)
imager3 = vtkRenderer()
imager3.AddViewProp(actor3)
imager3.SetViewport(0.5,0.5,1.0,1.0)
imager4 = vtkRenderer()
imager4.AddViewProp(actor4)
imager4.SetViewport(0.0,0.5,0.5,1.0)
imgWin = vtkRenderWindow()
imgWin.AddRenderer(imager1)
imgWin.AddRenderer(imager2)
imgWin.AddRenderer(imager3)
imgWin.AddRenderer(imager4)
imgWin.SetSize(150,128)
imgWin.Render()
# --- end of script --
