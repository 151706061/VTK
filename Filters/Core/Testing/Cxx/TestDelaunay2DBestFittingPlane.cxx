// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// This test was created following the discovery that the computation of the
// best fitting plane for Delaunay2D failed when points were located exactly
// in the XZ (or YZ) plane.
//
// The test is the same as TestDelaunay2D.cxx except that the points
// are inserted into the XZ plane instead of the XY plane, and that
// VTK_BEST_FITTING_PLANE mode is used.

#include "vtkActor.h"
#include "vtkCellArray.h"
#include "vtkDelaunay2D.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkShrinkPolyData.h"

// #define WRITE_IMAGE

#ifdef WRITE_IMAGE
#include "vtkPNGWriter.h"
#include "vtkWindowToImageFilter.h"
#endif

int TestDelaunay2DBestFittingPlane(int argc, char* argv[])
{
  vtkPoints* newPts = vtkPoints::New();
  newPts->InsertNextPoint(1.5026018771810041, 0.0, 1.5026019428618222);
  newPts->InsertNextPoint(-1.5026020085426373, 0.0, 1.5026018115001829);
  newPts->InsertNextPoint(-1.5026018353814194, 0.0, -1.5026019846614038);
  newPts->InsertNextPoint(1.5026019189805875, 0.0, -1.5026019010622396);
  newPts->InsertNextPoint(5.2149123972752491, 0.0, 5.2149126252263240);
  newPts->InsertNextPoint(-5.2149128531773883, 0.0, 5.2149121693241645);
  newPts->InsertNextPoint(-5.2149122522061022, 0.0, -5.2149127702954603);
  newPts->InsertNextPoint(5.2149125423443916, 0.0, -5.2149124801571842);
  newPts->InsertNextPoint(8.9272229173694946, 0.0, 8.9272233075908254);
  newPts->InsertNextPoint(-8.9272236978121402, 0.0, 8.9272225271481460);
  newPts->InsertNextPoint(-8.9272226690307868, 0.0, -8.9272235559295172);
  newPts->InsertNextPoint(8.9272231657081953, 0.0, -8.9272230592521282);
  newPts->InsertNextPoint(12.639533437463740, 0.0, 12.639533989955329);
  newPts->InsertNextPoint(-12.639534542446890, 0.0, 12.639532884972127);
  newPts->InsertNextPoint(-12.639533085855469, 0.0, -12.639534341563573);
  newPts->InsertNextPoint(12.639533789072001, 0.0, -12.639533638347073);

  vtkIdType inNumPts = newPts->GetNumberOfPoints();
  cout << "input numPts= " << inNumPts << endl;

  vtkPolyData* pointCloud = vtkPolyData::New();
  pointCloud->SetPoints(newPts);
  newPts->Delete();

  vtkDelaunay2D* delaunay2D = vtkDelaunay2D::New();
  delaunay2D->SetInputData(pointCloud);
  delaunay2D->SetProjectionPlaneMode(VTK_BEST_FITTING_PLANE);
  pointCloud->Delete();
  delaunay2D->Update();

  vtkPolyData* triangulation = delaunay2D->GetOutput();

  vtkIdType outNumPts = triangulation->GetNumberOfPoints();
  vtkIdType outNumCells = triangulation->GetNumberOfCells();
  vtkIdType outNumPolys = triangulation->GetNumberOfPolys();
  vtkIdType outNumLines = triangulation->GetNumberOfLines();
  vtkIdType outNumVerts = triangulation->GetNumberOfVerts();

  cout << "output numPts= " << outNumPts << endl;
  cout << "output numCells= " << outNumCells << endl;
  cout << "output numPolys= " << outNumPolys << endl;
  cout << "output numLines= " << outNumLines << endl;
  cout << "output numVerts= " << outNumVerts << endl;

  if (outNumPts != inNumPts)
  {
    cout << "ERROR: output numPts " << outNumPts << " doesn't match input numPts=" << inNumPts
         << endl;
    delaunay2D->Delete();
    return EXIT_FAILURE;
  }

  if (!outNumCells)
  {
    cout << "ERROR: output numCells= " << outNumCells << endl;
    delaunay2D->Delete();
    return EXIT_FAILURE;
  }

  if (outNumPolys != outNumCells)
  {
    cout << "ERROR: output numPolys= " << outNumPolys
         << " doesn't match output numCells= " << outNumCells << endl;
    delaunay2D->Delete();
    return EXIT_FAILURE;
  }

  if (outNumLines)
  {
    cout << "ERROR: output numLines= " << outNumLines << endl;
    delaunay2D->Delete();
    return EXIT_FAILURE;
  }

  if (outNumVerts)
  {
    cout << "ERROR: output numVerts= " << outNumVerts << endl;
    delaunay2D->Delete();
    return EXIT_FAILURE;
  }

  // check that every point is connected
  triangulation->BuildLinks();

  vtkIdList* cellIds = vtkIdList::New();
  vtkIdType numUnconnectedPts = 0;

  for (vtkIdType ptId = 0; ptId < outNumPts; ptId++)
  {
    triangulation->GetPointCells(ptId, cellIds);
    if (!cellIds->GetNumberOfIds())
    {
      numUnconnectedPts++;
    }
  }

  cellIds->Delete();

  cout << "Triangulation has " << numUnconnectedPts << " unconnected points" << endl;

  if (numUnconnectedPts)
  {
    cout << "ERROR: Triangulation has " << numUnconnectedPts << " unconnected points" << endl;
    delaunay2D->Delete();
    return EXIT_FAILURE;
  }

  vtkShrinkPolyData* shrink = vtkShrinkPolyData::New();
  shrink->SetInputConnection(delaunay2D->GetOutputPort());

  vtkPolyDataMapper* mapper = vtkPolyDataMapper::New();
  mapper->SetInputConnection(shrink->GetOutputPort());

  vtkActor* actor = vtkActor::New();
  actor->SetMapper(mapper);

  vtkRenderer* ren = vtkRenderer::New();
  ren->AddActor(actor);

  vtkRenderWindow* renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren);

  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  iren->Initialize();

  renWin->Render();
#ifdef WRITE_IMAGE
  vtkWindowToImageFilter* windowToImage = vtkWindowToImageFilter::New();
  windowToImage->SetInput(renWin);

  vtkPNGWriter* PNGWriter = vtkPNGWriter::New();
  PNGWriter->SetInputConnection(windowToImage->GetOutputPort());
  windowToImage->Delete();
  PNGWriter->SetFileName("TestDelaunay2DBestFittingPlane.png");
  PNGWriter->Write();
  PNGWriter->Delete();
#endif
  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  // Clean up
  delaunay2D->Delete();
  shrink->Delete();
  mapper->Delete();
  actor->Delete();
  ren->Delete();
  renWin->Delete();
  iren->Delete();

  return !retVal;
}
