// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAMRVelodyneReader.h"
#include "vtkOverlappingAMR.h"
#include "vtkSetGet.h"
#include "vtkTestUtilities.h"
#include "vtkUniformGrid.h"
#include <iostream>
#include <string>
namespace VelodyneReaderTest
{

//-----------------------------------------------------------------------------
template <class T>

int CheckValue(const std::string& name, T actualValue, T expectedValue)
{
  if (actualValue != expectedValue)
  {
    std::cerr << "ERROR: " << name << " value mismatch! ";
    std::cerr << "Expected: " << expectedValue << " Actual: " << actualValue;
    std::cerr << std::endl;
    return 1;
  }
  return 0;
}

} // END namespace
int TestVelodyneReader(int argc, char* argv[])
{
  std::cout << "Running Velodyne Reader Test\n";
  vtkAMRVelodyneReader* myVelodyneReader = vtkAMRVelodyneReader::New();
  char* fileName =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/AMR/Velodyne/TestAMR.xamr");
  myVelodyneReader->SetFileName(fileName);
  int result = VelodyneReaderTest::CheckValue("LEVELS", 6, myVelodyneReader->GetNumberOfLevels());
  if (result != 0)
  {
    return VTK_ERROR;
  }
  result = VelodyneReaderTest::CheckValue("BLOCKS", 2559, myVelodyneReader->GetNumberOfBlocks());
  if (result != 0)
  {
    return VTK_ERROR;
  }
  vtkOverlappingAMR* amr = nullptr;
  amr = myVelodyneReader->GetOutput();
  if (amr == nullptr)
  {
    return VTK_ERROR;
  }
  if (!amr->CheckValidity())
  {
    return VTK_ERROR;
  }
  myVelodyneReader->Delete();
  delete[] fileName;
  return EXIT_SUCCESS;
}
