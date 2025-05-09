// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME AMRCommon.h -- Encapsulates common functionality for AMR data.
//
// .SECTION Description
// This header encapsulates some common functionality for AMR data to
// simplify and expedite the development of examples.

#ifndef AMRCOMMON_H_
#define AMRCOMMON_H_

#include <cassert> // For C++ assert
#include <sstream> // For C++ string streams

#include "vtkCell.h"
#include "vtkCompositeDataWriter.h"
#include "vtkImageToStructuredGrid.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkOverlappingAMR.h"
#include "vtkStructuredGridWriter.h"
#include "vtkUniformGrid.h"
#include "vtkXMLImageDataWriter.h"
#include "vtkXMLMultiBlockDataWriter.h"
#include "vtkXMLUniformGridAMRReader.h"
#include "vtkXMLUniformGridAMRWriter.h"

namespace AMRCommon
{
VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
// Description:
// Writes a uniform grid as a structure grid
void WriteUniformGrid(vtkUniformGrid* g, const std::string& prefix)
{
  assert("pre: Uniform grid (g) is NULL!" && (g != nullptr));

  vtkNew<vtkXMLImageDataWriter> imgWriter;

  std::ostringstream oss;
  oss << prefix << "." << imgWriter->GetDefaultFileExtension();
  imgWriter->SetFileName(oss.str().c_str());
  imgWriter->SetInputData(g);
  imgWriter->Write();
}

//------------------------------------------------------------------------------
// Description:
// Writes the given AMR dataset to a *.vth file with the given prefix.
void WriteAMRData(vtkOverlappingAMR* amrData, const std::string& prefix)
{
  // Sanity check
  assert("pre: AMR dataset is NULL!" && (amrData != nullptr));

  vtkNew<vtkCompositeDataWriter> writer;

  std::ostringstream oss;
  oss << prefix << ".vthb";
  writer->SetFileName(oss.str().c_str());
  writer->SetInputData(amrData);
  writer->Write();
}

//------------------------------------------------------------------------------
// Description:
// Reads AMR data to the given data-structure from the prescribed file.
vtkOverlappingAMR* ReadAMRData(const std::string& file)
{
  // Sanity check
  //  assert( "pre: AMR dataset is NULL!" && (amrData != NULL) );

  vtkXMLUniformGridAMRReader* myAMRReader = vtkXMLUniformGridAMRReader::New();
  assert("pre: AMR Reader is NULL!" && (myAMRReader != nullptr));

  std::ostringstream oss;
  oss.str("");
  oss.clear();
  oss << file << ".vthb";

  std::cout << "Reading AMR Data from: " << oss.str() << std::endl;
  std::cout.flush();

  myAMRReader->SetFileName(oss.str().c_str());
  myAMRReader->Update();

  vtkOverlappingAMR* amrData = vtkOverlappingAMR::SafeDownCast(myAMRReader->GetOutput());
  assert("post: AMR data read is NULL!" && (amrData != nullptr));
  return (amrData);
}

//------------------------------------------------------------------------------
// Description:
// Writes the given multi-block data to an XML file with the prescribed prefix
void WriteMultiBlockData(vtkMultiBlockDataSet* mbds, const std::string& prefix)
{
  // Sanity check
  assert("pre: Multi-block dataset is NULL" && (mbds != nullptr));
  vtkNew<vtkXMLMultiBlockDataWriter> writer;

  std::ostringstream oss;
  oss.str("");
  oss.clear();
  oss << prefix << "." << writer->GetDefaultFileExtension();
  writer->SetFileName(oss.str().c_str());
  writer->SetInputData(mbds);
  writer->Write();
}

//------------------------------------------------------------------------------
// Constructs a uniform grid instance given the prescribed
// origin, grid spacing and dimensions.
vtkUniformGrid* GetGrid(double* origin, double* h, int* ndim)
{
  vtkNew<vtkUniformGrid> grd;
  grd->Initialize();
  grd->SetOrigin(origin);
  grd->SetSpacing(h);
  grd->SetDimensions(ndim);
  return grd;
}

//------------------------------------------------------------------------------
// Computes the cell center for the cell corresponding to cellIdx w.r.t.
// the given grid. The cell center is stored in the supplied buffer c.
void ComputeCellCenter(vtkUniformGrid* grid, const int cellIdx, double c[3])
{
  assert("pre: grid != NULL" && (grid != nullptr));
  assert("pre: Null cell center buffer" && (c != nullptr));
  assert("pre: cellIdx in bounds" && (cellIdx >= 0) && (cellIdx < grid->GetNumberOfCells()));

  vtkCell* myCell = grid->GetCell(cellIdx);
  assert("post: cell is NULL" && (myCell != nullptr));

  double pCenter[3];
  std::vector<double> weights(myCell->GetNumberOfPoints());
  int subId = myCell->GetParametricCenter(pCenter);
  myCell->EvaluateLocation(subId, pCenter, c, weights.data());
}

VTK_ABI_NAMESPACE_END
} // END namespace

#endif /* AMRCOMMON_H_ */
