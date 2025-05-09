// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Test case of empty input for vtkGlyph3DMapper. Refer to MR!1529.
#include "vtkCubeSource.h"
#include "vtkGlyph3DMapper.h"
#include "vtkMath.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"

int TestGlyph3DMapperBounds(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  // create empty input data
  vtkSmartPointer<vtkPolyData> polydata = vtkSmartPointer<vtkPolyData>::New();

  vtkSmartPointer<vtkCubeSource> cubeSource = vtkSmartPointer<vtkCubeSource>::New();

  vtkSmartPointer<vtkGlyph3DMapper> glyph3Dmapper = vtkSmartPointer<vtkGlyph3DMapper>::New();
  glyph3Dmapper->SetSourceConnection(cubeSource->GetOutputPort());
  glyph3Dmapper->SetInputData(polydata);
  glyph3Dmapper->Update();

  double boundsAnswer[6];
  vtkMath::UninitializeBounds(boundsAnswer);
  // since there is nothing inside the scene, the boundsResult should be an
  // uninitializeBounds
  const double* boundsResult = glyph3Dmapper->GetBounds();
  for (int i = 0; i < 6; ++i)
  {
    if (boundsResult[i] != boundsAnswer[i])
      return -1;
  }
  return 0;
}
