#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
from vtkmodules.vtkCommonCore import (
    vtkFloatArray,
    vtkLookupTable,
)
from vtkmodules.vtkCommonDataModel import vtkTable
from vtkmodules.vtkChartsCore import vtkChartParallelCoordinates
from vtkmodules.vtkRenderingCore import vtkColorTransferFunction
from vtkmodules.vtkViewsContext2D import vtkContextView
import vtkmodules.vtkRenderingContextOpenGL2
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
import vtkmodules.test.Testing
import math

class TestParallelCoordinatesColors(vtkmodules.test.Testing.vtkTest):
    def testLinePlot(self):
        "Test if colored parallel coordinates plots can be built with python"

        # Set up a 2D scene, add a PC chart to it
        view = vtkContextView()
        view.GetRenderer().SetBackground(1.0, 1.0, 1.0)
        view.GetRenderWindow().SetSize(600,300)

        chart = vtkChartParallelCoordinates()
        view.GetScene().AddItem(chart)

        # Create a table with some points in it
        arrX = vtkFloatArray()
        arrX.SetName("XAxis")

        arrC = vtkFloatArray()
        arrC.SetName("Cosine")

        arrS = vtkFloatArray()
        arrS.SetName("Sine")

        arrS2 = vtkFloatArray()
        arrS2.SetName("Tan")

        numPoints = 200
        inc = 7.5 / (numPoints-1)

        for i in range(numPoints):
            arrX.InsertNextValue(i * inc)
            arrC.InsertNextValue(math.cos(i * inc) + 0.0)
            arrS.InsertNextValue(math.sin(i * inc) + 0.0)
            arrS2.InsertNextValue(math.tan(i * inc) + 0.5)

        table = vtkTable()
        table.AddColumn(arrX)
        table.AddColumn(arrC)
        table.AddColumn(arrS)
        table.AddColumn(arrS2)

        # Create blue to gray to red lookup table
        lut = vtkLookupTable()
        lutNum = 256
        lut.SetNumberOfTableValues(lutNum)
        lut.Build()

        ctf = vtkColorTransferFunction()
        ctf.SetColorSpaceToDiverging()
        cl = []
        # Variant of Colorbrewer RdBu 5
        cl.append([float(cc)/255.0 for cc in [202, 0, 32]])
        cl.append([float(cc)/255.0 for cc in [244, 165, 130]])
        cl.append([float(cc)/255.0 for cc in [140, 140, 140]])
        cl.append([float(cc)/255.0 for cc in [146, 197, 222]])
        cl.append([float(cc)/255.0 for cc in [5, 113, 176]])
        vv = [float(xx)/float(len(cl)-1) for xx in range(len(cl))]
        vv.reverse()
        for pt,color in zip(vv,cl):
            ctf.AddRGBPoint(pt, color[0], color[1], color[2])

        for ii,ss in enumerate([float(xx)/float(lutNum) for xx in range(lutNum)]):
            cc = ctf.GetColor(ss)
            lut.SetTableValue(ii,cc[0],cc[1],cc[2],1.0)

        lut.SetAlpha(0.25)
        lut.SetRange(-1, 1)

        chart.GetPlot(0).SetInputData(table)
        chart.GetPlot(0).SetScalarVisibility(1)
        chart.GetPlot(0).SetLookupTable(lut)
        chart.GetPlot(0).SelectColorArray("Cosine")

        view.GetRenderWindow().SetMultiSamples(0)
        view.GetRenderWindow().Render()

        img_file = "TestParallelCoordinatesColors.png"
        vtkmodules.test.Testing.compareImage(view.GetRenderWindow(),vtkmodules.test.Testing.getAbsImagePath(img_file))
        vtkmodules.test.Testing.interact()

if __name__ == "__main__":
    vtkmodules.test.Testing.main([(TestParallelCoordinatesColors, 'test')])
