// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include <iostream>
#include <string>

#include "vtkAMREnzoReader.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkOverlappingAMR.h"
#include "vtkSetGet.h"
#include "vtkTestUtilities.h"
#include "vtkUniformGrid.h"
#include "vtkUniformGridAMRIterator.h"
namespace EnzoReaderTest
{

//------------------------------------------------------------------------------
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

static int ComputeMaxNonEmptyLevel(vtkOverlappingAMR* amr)
{
  vtkUniformGridAMRIterator* iter = vtkUniformGridAMRIterator::SafeDownCast(amr->NewIterator());
  iter->SetSkipEmptyNodes(true);
  int maxLevel(-1);
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    int level = iter->GetCurrentLevel();
    if (level > maxLevel)
    {
      maxLevel = level;
    }
  }
  iter->Delete();
  return maxLevel + 1;
}

static int ComputeNumberOfVisibleCells(vtkOverlappingAMR* amr)
{
  int numVisibleCells(0);
  vtkCompositeDataIterator* iter = amr->NewIterator();
  iter->SkipEmptyNodesOn();
  for (iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    vtkUniformGrid* grid = vtkUniformGrid::SafeDownCast(iter->GetCurrentDataObject());
    vtkIdType num = grid->GetNumberOfCells();
    for (vtkIdType i = 0; i < num; i++)
    {
      if (grid->IsCellVisible(i))
      {
        numVisibleCells++;
      }
    }
  }
  iter->Delete();
  return numVisibleCells;
}

int TestEnzoReader(int argc, char* argv[])
{
  int rc = 0;
  int NumBlocksPerLevel[] = { 1, 3, 1, 1, 1, 1, 1, 1 };
  int numVisibleCells[] = { 4096, 6406, 13406, 20406, 23990, 25502, 26377, 27077 };
  vtkAMREnzoReader* myEnzoReader = vtkAMREnzoReader::New();
  char* fileName =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/AMR/Enzo/DD0010/moving7_0010.hierarchy");
  std::cout << "Filename: " << fileName << std::endl;
  std::cout.flush();

  vtkOverlappingAMR* amr = nullptr;
  myEnzoReader->SetFileName(fileName);
  for (int level = 0; level < myEnzoReader->GetNumberOfLevels(); ++level)
  {
    myEnzoReader->SetMaxLevel(level);
    myEnzoReader->Update();
    rc += EnzoReaderTest::CheckValue("LEVEL", myEnzoReader->GetNumberOfLevels(), 8);
    rc += EnzoReaderTest::CheckValue("BLOCKS", myEnzoReader->GetNumberOfBlocks(), 10);

    amr = myEnzoReader->GetOutput();
    if (amr != nullptr)
    {
      if (!amr->CheckValidity())
      {
        std::cerr << "ERROR: output AMR dataset is not valid!";
        return 1;
      }

      rc += EnzoReaderTest::CheckValue("OUTPUT LEVELS", ComputeMaxNonEmptyLevel(amr), level + 1);
      rc += EnzoReaderTest::CheckValue("NUMBER OF BLOCKS AT LEVEL",
        static_cast<int>(amr->GetNumberOfBlocks(level)), NumBlocksPerLevel[level]);
      rc += EnzoReaderTest::CheckValue(
        "Number of Visible cells ", ComputeNumberOfVisibleCells(amr), numVisibleCells[level]);
    }
    else
    {
      std::cerr << "ERROR: output AMR dataset is nullptr!";
      return 1;
    }
  } // END for all levels

  myEnzoReader->Delete();
  delete[] fileName;
  return (rc);
}
