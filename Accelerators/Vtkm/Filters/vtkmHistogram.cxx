// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkmHistogram.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPointData.h"
#include "vtkTable.h"
#include "vtkUnstructuredGrid.h"

#include "vtkmlib/ArrayConverters.h"
#include "vtkmlib/DataSetConverters.h"

#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

#include <viskores/filter/density_estimate/Histogram.h>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkmHistogram);

//------------------------------------------------------------------------------
vtkmHistogram::vtkmHistogram()
{
  this->CustomBinRange[0] = 0;
  this->CustomBinRange[0] = 100;
  this->UseCustomBinRanges = false;
  this->CenterBinsAroundMinAndMax = false;
  this->NumberOfBins = 10;
}

//------------------------------------------------------------------------------
vtkmHistogram::~vtkmHistogram() = default;

//------------------------------------------------------------------------------
int vtkmHistogram::FillInputPortInformation(int port, vtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);

  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

//------------------------------------------------------------------------------
int vtkmHistogram::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkTable* output = vtkTable::GetData(outputVector, 0);
  output->Initialize();

  // These are the mid-points for each of the bins
  vtkSmartPointer<vtkDoubleArray> binExtents = vtkSmartPointer<vtkDoubleArray>::New();
  binExtents->SetNumberOfComponents(1);
  binExtents->SetNumberOfTuples(this->NumberOfBins);
  binExtents->SetName("bin_extents");
  binExtents->FillComponent(0, 0.0);

  // Grab the input array to process to determine the field we want to apply histogram
  int association = this->GetInputArrayAssociation(0, inputVector);
  auto fieldArray = this->GetInputArrayToProcess(0, inputVector);
  if ((association != vtkDataObject::FIELD_ASSOCIATION_POINTS &&
        association != vtkDataObject::FIELD_ASSOCIATION_CELLS) ||
    fieldArray == nullptr || fieldArray->GetName() == nullptr || fieldArray->GetName()[0] == '\0')
  {
    vtkErrorMacro(<< "Invalid field: Requires a point or cell field with a valid name.");
    return 0;
  }

  const char* fieldName = fieldArray->GetName();

  try
  {
    viskores::cont::DataSet in = tovtkm::Convert(input);
    auto field = tovtkm::Convert(fieldArray, association);
    in.AddField(field);

    viskores::filter::density_estimate::Histogram filter;

    filter.SetNumberOfBins(static_cast<viskores::Id>(this->NumberOfBins));
    filter.SetActiveField(fieldName, field.GetAssociation());
    if (this->UseCustomBinRanges)
    {
      if (this->CustomBinRange[0] > this->CustomBinRange[1])
      {
        vtkWarningMacro("Custom bin range adjusted to keep min <= max value");
        double min = this->CustomBinRange[1];
        double max = this->CustomBinRange[0];
        this->CustomBinRange[0] = min;
        this->CustomBinRange[1] = max;
      }
      filter.SetRange(viskores::Range(this->CustomBinRange[0], this->CustomBinRange[1]));
    }
    auto result = filter.Execute(in);
    this->BinDelta = filter.GetBinDelta();
    this->ComputedRange[0] = filter.GetComputedRange().Min;
    this->ComputedRange[1] = filter.GetComputedRange().Max;

    // Convert the result back
    vtkDataArray* resultingArray = fromvtkm::Convert(result.GetField("histogram"));
    resultingArray->SetName("bin_values");
    if (resultingArray == nullptr)
    {
      vtkErrorMacro(<< "Unable to convert result array from Viskores to VTK");
      return 0;
    }
    this->FillBinExtents(binExtents);
    output->GetRowData()->AddArray(binExtents);
    output->GetRowData()->AddArray(resultingArray);

    resultingArray->FastDelete();
  }
  catch (const viskores::cont::Error& e)
  {
    vtkErrorMacro(<< "Viskores error: " << e.GetMessage());
    return 0;
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkmHistogram::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "NumberOfBins: " << NumberOfBins << "\n";
  os << indent << "UseCustomBinRanges: " << UseCustomBinRanges << "\n";
  os << indent << "CenterBinsAroundMinAndMax: " << CenterBinsAroundMinAndMax << "\n";
  os << indent << "CustomBinRange: " << CustomBinRange[0] << ", " << CustomBinRange[1] << "\n";
}

//------------------------------------------------------------------------------
void vtkmHistogram::FillBinExtents(vtkDoubleArray* binExtents)
{
  binExtents->SetNumberOfComponents(1);
  binExtents->SetNumberOfTuples(this->NumberOfBins);
  double binDelta = this->CenterBinsAroundMinAndMax
    ? ((this->ComputedRange[1] - this->ComputedRange[0]) / (this->NumberOfBins - 1))
    : this->BinDelta;
  double halfBinDelta = binDelta / 2.0;
  for (vtkIdType i = 0; i < this->NumberOfBins; i++)
  {
    binExtents->SetValue(i,
      this->ComputedRange[0] + (i * binDelta) +
        (this->CenterBinsAroundMinAndMax ? 0.0 : halfBinDelta));
  }
}
VTK_ABI_NAMESPACE_END
