// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkmClip.h"
#include "vtkmClipInternals.h"

#include "vtkCellIterator.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkImplicitFunction.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkTableBasedClipDataSet.h"
#include "vtkUnstructuredGrid.h"

#include "vtkmlib/ArrayConverters.h"
#include "vtkmlib/DataSetConverters.h"
#include "vtkmlib/ImplicitFunctionConverter.h"
#include "vtkmlib/PolyDataConverter.h"
#include "vtkmlib/UnstructuredGridConverter.h"

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/cont/Invoker.h>
#include <viskores/worklet/WorkletMapTopology.h>

#include <algorithm>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkmClip);

//------------------------------------------------------------------------------
vtkmClip::vtkmClip() = default;

//------------------------------------------------------------------------------
vtkmClip::~vtkmClip() = default;

//------------------------------------------------------------------------------
void vtkmClip::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ComputeScalars: " << (this->GetComputeScalars() ? "On" : "Off") << "\n";
}

//------------------------------------------------------------------------------
namespace
{

struct IsCellSupported : public viskores::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn, FieldOutCell);
  using ExecutionSignature = _2(CellShape);

  template <typename CellShapeTag>
  VISKORES_EXEC bool operator()(CellShapeTag shape) const
  {
    return (shape.Id != viskores::CELL_SHAPE_POLY_LINE) &&
      (shape.Id != viskores::CELL_SHAPE_POLYGON);
  }
};

// Checks if there are cells that are supported by viskores in general but unsupported
// by clip
bool CellSetHasUnsupportedCells(const viskores::cont::UnknownCellSet& cellset)
{
  viskores::cont::ArrayHandle<bool> supported;
  viskores::cont::Invoker{}(IsCellSupported{}, cellset, supported);
  return !viskores::cont::Algorithm::Reduce(supported, true, viskores::LogicalAnd());
}

} // anonymous namespace

//------------------------------------------------------------------------------
int vtkmClip::RequestData(
  vtkInformation* request, vtkInformationVector** inInfoVec, vtkInformationVector* outInfoVec)
{
  vtkInformation* inInfo = inInfoVec[0]->GetInformationObject(0);
  vtkInformation* outInfo = outInfoVec->GetInformationObject(0);

  // Extract data objects from info:
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid* output =
    vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid* clippedOutput = this->GetClippedOutput();

  if (input->GetNumberOfPoints() == 0 || input->GetNumberOfCells() == 0)
  {
    return 1; // nothing to do
  }

  // Find the scalar array:
  int assoc = this->GetInputArrayAssociation(0, inInfoVec);
  vtkDataArray* scalars = this->GetInputArrayToProcess(0, inInfoVec);
  if (!this->GetClipFunction() &&
    (assoc != vtkDataObject::FIELD_ASSOCIATION_POINTS || scalars == nullptr))
  {
    vtkErrorMacro("Invalid scalar array; array missing or not a point array.");
    return 0;
  }

  try
  {
    // Due to our use of `CleanGrid`, our output will always have single precision points
    auto pointSet = vtkPointSet::SafeDownCast(input);
    if ((this->GetOutputPointsPrecision() == vtkAlgorithm::DOUBLE_PRECISION) ||
      (this->GetOutputPointsPrecision() == vtkAlgorithm::DEFAULT_PRECISION && pointSet &&
        pointSet->GetPoints()->GetDataType() != VTK_FLOAT))
    {
      throw viskores::cont::ErrorFilterExecution(
        "vtkmClip only supports generating single precision output points.");
    }

    if (this->GetClipFunction())
    {
      // `UseValueAsOffset` is on by default, so check `Value` also to determine support.
      if (this->UseValueAsOffset && this->Value != 0.0)
      {
        throw viskores::cont::ErrorFilterExecution("`UseValueAsOffset` is not supported");
      }
      if (this->GenerateClipScalars)
      {
        throw viskores::cont::ErrorFilterExecution("`GenerateClipScalars` is not supported");
      }
    }

    // Convert inputs to viskores objects:
    auto fieldsFlag =
      this->GetComputeScalars() ? tovtkm::FieldsFlag::PointsAndCells : tovtkm::FieldsFlag::None;
    auto in = tovtkm::Convert(input, fieldsFlag);

    if (CellSetHasUnsupportedCells(in.GetCellSet()))
    {
      throw viskores::cont::ErrorFilterExecution("Unsupported cell in input");
    }

    // Run filter:
    viskores::cont::DataSet result, result1;
    if (this->GetClipFunction())
    {
      result = internals::ExecuteClipWithImplicitFunction(in, this->ClipFunction, this->InsideOut);
      if (clippedOutput)
      {
        this->InsideOut = !this->InsideOut;
        result1 =
          internals::ExecuteClipWithImplicitFunction(in, this->ClipFunction, this->InsideOut);
        this->InsideOut = !this->InsideOut;
      }
    }
    else
    {
      result = internals::ExecuteClipWithField(
        in, scalars, assoc, this->Value, this->InsideOut, this->ComputeScalars);
      if (clippedOutput)
      {
        this->InsideOut = !this->InsideOut;
        result1 = internals::ExecuteClipWithField(
          in, scalars, assoc, this->Value, this->InsideOut, this->ComputeScalars);
        this->InsideOut = !this->InsideOut;
      }
    }

    // Convert result to output:
    if (!fromvtkm::Convert(result, output, input) ||
      (clippedOutput && !fromvtkm::Convert(result1, clippedOutput, input)))
    {
      throw viskores::cont::ErrorFilterExecution(
        "Unable to convert Viskores result dataSet back to VTK.");
    }

    if (!this->GetClipFunction() && this->GetComputeScalars())
    {
      output->GetPointData()->SetActiveScalars(scalars->GetName());
      if (clippedOutput)
      {
        clippedOutput->GetPointData()->SetActiveScalars(scalars->GetName());
      }
    }

    return 1;
  }
  catch (const viskores::cont::Error& e)
  {
    if (this->ForceVTKm)
    {
      vtkErrorMacro(<< "Viskores error: " << e.GetMessage());
      return 0;
    }
    else
    {
      vtkWarningMacro(<< "Viskores failed with message: " << e.GetMessage() << "\n"
                      << "Falling back to the default VTK implementation.");
      return this->Superclass::RequestData(request, inInfoVec, outInfoVec);
    }
  }
}
VTK_ABI_NAMESPACE_END
