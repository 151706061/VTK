// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2009 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-LANL-California-USGov

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCompositeDataGeometryFilter.h"
#include "vtkInformation.h"
#include "vtkLookupTable.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSLACParticleReader.h"
#include "vtkSLACReader.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTestUtilities.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include <sstream>

int SLACParticleReader(int argc, char* argv[])
{
  char* directoryName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SLAC/pic-example/");
  const std::string directory = directoryName;
  delete[] directoryName;
  const std::string meshFileName = directory + "mesh.ncdf";
  const std::string particleFileName = directory + "particles_5.ncdf";

  // Set up mesh reader.
  VTK_CREATE(vtkSLACReader, meshReader);
  meshReader->SetMeshFileName(meshFileName.c_str());

  size_t modeFileNameLength = directory.size() + 32;
  char* modeFileName = new char[modeFileNameLength];
  for (int i = 0; i < 9; i++)
  {
    snprintf(modeFileName, modeFileNameLength, "%sfields_%d.mod", directory.c_str(), i);
    meshReader->AddModeFileName(modeFileName);
  }
  delete[] modeFileName;

  meshReader->ReadInternalVolumeOn();
  meshReader->ReadExternalSurfaceOff();
  meshReader->ReadMidpointsOff();

  // Extract geometry that we can render.
  VTK_CREATE(vtkCompositeDataGeometryFilter, geometry);
  geometry->SetInputConnection(meshReader->GetOutputPort(vtkSLACReader::VOLUME_OUTPUT));

  // Set up particle reader.
  VTK_CREATE(vtkSLACParticleReader, particleReader);
  particleReader->SetFileName(particleFileName.c_str());

  // Set up rendering stuff.
  VTK_CREATE(vtkPolyDataMapper, meshMapper);
  meshMapper->SetInputConnection(geometry->GetOutputPort());
  meshMapper->SetScalarModeToUsePointFieldData();
  meshMapper->ColorByArrayComponent("efield", 2);
  meshMapper->UseLookupTableScalarRangeOff();
  meshMapper->SetScalarRange(1.0, 1e+05);

  VTK_CREATE(vtkLookupTable, lut);
  lut->SetHueRange(0.66667, 0.0);
  lut->SetScaleToLog10();
  meshMapper->SetLookupTable(lut);

  VTK_CREATE(vtkActor, meshActor);
  meshActor->SetMapper(meshMapper);
  meshActor->GetProperty()->FrontfaceCullingOn();

  VTK_CREATE(vtkPolyDataMapper, particleMapper);
  particleMapper->SetInputConnection(particleReader->GetOutputPort());
  particleMapper->ScalarVisibilityOff();

  VTK_CREATE(vtkActor, particleActor);
  particleActor->SetMapper(particleMapper);

  VTK_CREATE(vtkRenderer, renderer);
  renderer->AddActor(meshActor);
  renderer->AddActor(particleActor);
  vtkCamera* camera = renderer->GetActiveCamera();
  camera->SetPosition(-0.2, 0.05, 0.0);
  camera->SetFocalPoint(0.0, 0.05, 0.0);
  camera->SetViewUp(0.0, 1.0, 0.0);

  VTK_CREATE(vtkRenderWindow, renwin);
  renwin->SetSize(300, 200);
  renwin->AddRenderer(renderer);
  VTK_CREATE(vtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(renwin);
  renwin->Render();

  double time = particleReader->GetOutput()->GetInformation()->Get(vtkDataObject::DATA_TIME_STEP());
  cout << "Time in particle reader: " << time << endl;

  // Change the time to test the time step field load and to have the field
  // match the particles in time.
  geometry->UpdateInformation();
  geometry->GetOutputInformation(0)->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), time);
  renwin->Render();

  // Do the test comparison.
  int retVal = vtkRegressionTestImage(renwin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
    retVal = vtkRegressionTester::PASSED;
  }

  return (retVal == vtkRegressionTester::PASSED) ? 0 : 1;
}
