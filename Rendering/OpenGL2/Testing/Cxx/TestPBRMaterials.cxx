// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This test covers the PBR Interpolation shading
// It renders spheres with different materials using a skybox as image based lighting

#include "vtkActor.h"
#include "vtkActorCollection.h"
#include "vtkImageFlip.h"
#include "vtkJPEGReader.h"
#include "vtkLight.h"
#include "vtkLookupTable.h"
#include "vtkNew.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLSkybox.h"
#include "vtkOpenGLTexture.h"
#include "vtkPBRIrradianceTexture.h"
#include "vtkPBRLUTTexture.h"
#include "vtkPBRPrefilterTexture.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"

//------------------------------------------------------------------------------
int TestPBRMaterials(int argc, char* argv[])
{
  vtkNew<vtkOpenGLRenderer> renderer;

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(600, 600);
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkOpenGLSkybox> skybox;

  vtkSmartPointer<vtkPBRIrradianceTexture> irradiance = renderer->GetEnvMapIrradiance();
  irradiance->SetIrradianceStep(0.3);

  vtkNew<vtkOpenGLTexture> textureCubemap;
  textureCubemap->CubeMapOn();

  std::string pathSkybox[6] = { "Data/skybox/posx.jpg", "Data/skybox/negx.jpg",
    "Data/skybox/posy.jpg", "Data/skybox/negy.jpg", "Data/skybox/posz.jpg",
    "Data/skybox/negz.jpg" };

  for (int i = 0; i < 6; i++)
  {
    vtkNew<vtkJPEGReader> jpg;
    char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, pathSkybox[i].c_str());
    jpg->SetFileName(fname);
    delete[] fname;
    vtkNew<vtkImageFlip> flip;
    flip->SetInputConnection(jpg->GetOutputPort());
    flip->SetFilteredAxis(1); // flip y axis
    textureCubemap->SetInputConnection(i, flip->GetOutputPort());
  }

  vtkNew<vtkLight> l1;
  l1->SetPositional(true);
  renderer->AddLight(l1);
  vtkNew<vtkLight> l2;
  l2->SetPosition(2, 1, 1);
  l2->SetFocalPoint(2, 1, 0);
  l2->SetColor(1.0, 0.6, 1.0);
  l2->SetPositional(false);
  renderer->AddLight(l2);
  renderer->SetEnvironmentTexture(textureCubemap, true);
  renderer->UseImageBasedLightingOn();
  renderer->UseSphericalHarmonicsOff();

  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(100);
  sphere->SetPhiResolution(100);

  vtkNew<vtkPolyDataMapper> pdSphere;
  pdSphere->SetInputConnection(sphere->GetOutputPort());

  vtkNew<vtkLookupTable> lut;
  lut->SetIndexedLookup(true);
  lut->SetNumberOfColors(5);
  lut->SetTableValue(0, 1.0, 1.0, 1.0);
  lut->SetTableValue(1, 0.72, 0.45, 0.2);
  lut->SetTableValue(2, 0.0, 0.0, 0.0);
  lut->SetTableValue(3, 0.0, 1.0, 1.0);
  lut->SetTableValue(4, 1.0, 0.0, 0.0);

  for (int j = 0; j < 5; ++j)
  {
    for (int i = 0; i < 6; i++)
    {
      vtkNew<vtkActor> actorSphere;
      actorSphere->SetPosition(i, j * 1.0, 0.0);
      actorSphere->SetMapper(pdSphere);
      actorSphere->GetProperty()->SetInterpolationToPBR();
      actorSphere->GetProperty()->SetColor(lut->GetTableValue(j));
      actorSphere->GetProperty()->SetMetallic(j < 2 ? 1.0 : 0.0);
      actorSphere->GetProperty()->SetRoughness(i / 5.0);
      renderer->AddActor(actorSphere);
    }
  }

  skybox->SetTexture(textureCubemap);
  renderer->AddActor(skybox);

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
