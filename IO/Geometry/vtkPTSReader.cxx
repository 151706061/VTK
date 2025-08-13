// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPTSReader.h"

#include "vtkCellArray.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkStringScanner.h"
#include "vtkUnsignedCharArray.h"

#include "vtksys/FStream.hxx"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPTSReader);

//------------------------------------------------------------------------------
vtkPTSReader::vtkPTSReader()
  : FileName(nullptr)
  , OutputDataTypeIsDouble(false)
  , LimitReadToBounds(false)
  , LimitToMaxNumberOfPoints(false)
  , MaxNumberOfPoints(1000000)
{
  this->SetNumberOfInputPorts(0);
  this->ReadBounds[0] = this->ReadBounds[2] = this->ReadBounds[4] = VTK_DOUBLE_MAX;
  this->ReadBounds[1] = this->ReadBounds[3] = this->ReadBounds[5] = VTK_DOUBLE_MIN;

  this->CreateCells = true;
  this->IncludeColorAndLuminance = true;
}

//------------------------------------------------------------------------------
vtkPTSReader::~vtkPTSReader()
{
  if (this->FileName)
  {
    delete[] this->FileName;
    this->FileName = nullptr;
  }
}

//------------------------------------------------------------------------------
// vtkSetStringMacro except we clear some variables if we update the value
void vtkPTSReader::SetFileName(const char* filename)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting FileName to " << filename);
  if (this->FileName == nullptr && filename == nullptr)
  {
    return;
  }
  if (this->FileName && filename && !strcmp(this->FileName, filename))
  {
    return;
  }
  delete[] this->FileName;
  if (filename)
  {
    size_t n = strlen(filename) + 1;
    char* cp1 = new char[n];
    const char* cp2 = (filename);
    this->FileName = cp1;
    do
    {
      *cp1++ = *cp2++;
    } while (--n);
  }
  else
  {
    this->FileName = nullptr;
  }
  this->Modified();
}

//------------------------------------------------------------------------------
int vtkPTSReader::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* vtkNotUsed(outputVector))
{
  if (!this->FileName)
  {
    vtkErrorMacro("FileName has to be specified!");
    return 0;
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkPTSReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "File Name: " << (this->FileName ? this->FileName : "(none)") << "\n";
  os << indent << "OutputDataType = " << (this->OutputDataTypeIsDouble ? "double" : "float")
     << "\n";

  os << indent << "CreateCells = " << (this->CreateCells ? "yes" : "no") << "\n";

  os << indent << "IncludeColorAndLuminance = " << (this->IncludeColorAndLuminance ? "yes" : "no")
     << "\n";

  if (this->LimitReadToBounds)
  {
    os << indent << "LimitReadToBounds = true\n";
    os << indent << "ReadBounds = [" << this->ReadBounds[0] << "," << this->ReadBounds[1] << ","
       << this->ReadBounds[2] << this->ReadBounds[3] << "," << this->ReadBounds[4] << ","
       << this->ReadBounds[5] << "]\n";
  }
  else
  {
    os << indent << "LimitReadToBounds = false\n";
  }

  if (this->LimitToMaxNumberOfPoints)
  {
    os << indent << "LimitToMaxNumberOfPoints = true\n";
    os << indent << "MaxNumberOfPoints" << MaxNumberOfPoints << "\n";
  }
  else
  {
    os << indent << "LimitToMaxNumberOfPoints = false\n";
  }
}

//------------------------------------------------------------------------------
int vtkPTSReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  // See if we can open in the file
  if (!this->FileName)
  {
    vtkErrorMacro(<< "FileName must be specified.");
    return 0;
  }

  // Open the new file.
  vtkDebugMacro(<< "Opening file " << this->FileName);
  vtksys::ifstream file(this->FileName, ios::in | ios::binary);
  if (!file || file.fail())
  {
    vtkErrorMacro(<< "Could not open file " << this->FileName);
    return 0;
  }

  this->UpdateProgress(0);

  // Determine the number of points to be read in which should be
  // a single int at the top of the file
  std::string buffer;
  vtkTypeInt32 numPts = -1;
  for (numPts = -1; !file.eof();)
  {
    getline(file, buffer);
    // scan should match the integer part but not the string
    auto resultInt = vtk::scan_int<int>(buffer);
    auto resultStr = vtk::scan_value<std::string_view>(resultInt->range());
    if (resultInt && !resultStr)
    {
      numPts = resultInt->value();
      break;
    }
    if (resultInt || resultStr)
    {
      // We have a file that doesn't have a number of points line
      // Instead we need to count the number of lines in the file
      // Remember we already read in the first line hence numPts starts
      // at 1
      for (numPts = 1; getline(file, buffer); ++numPts)
      {
        if (numPts % 1000000 == 0)
        {
          this->UpdateProgress(0.1);
          if (this->GetAbortExecute())
          {
            return 0;
          }
        }
      }
      file.clear();
      file.seekg(0);
      break;
    }
  }

  // Next determine the format the point info. Which of the above is it?
  // 1) x y z,
  // 2) x y z intensity
  // 3) x y z intensity r g b
  double irgb[4], pt[3];

  if (numPts == -1)
  {
    vtkErrorMacro(<< "Could not process file " << this->FileName << " - Unknown Format");
    return 0;
  }
  if (numPts == 0)
  {
    // Trivial case of no points - lets set it to 3
    vtkErrorMacro(<< "Could not process file " << this->FileName << " - No points specified");
    return 0;
  }
  getline(file, buffer);
  auto resultPoint = vtk::scan<double, double, double>(buffer, "{:f} {:f} {:f}");
  auto resultIntensity = vtk::scan_value<double>(resultPoint->range());
  auto resultColor = vtk::scan<double, double, double>(resultIntensity->range(), "{:f} {:f} {:f}");
  if (resultPoint)
  {
    std::tie(pt[0], pt[1], pt[2]) = resultPoint->values();
    if (resultIntensity)
    {
      irgb[0] = resultIntensity->value();
      if (resultColor)
      {
        std::tie(irgb[1], irgb[2], irgb[3]) = resultColor->values();
      }
    }
  }
  if (!resultPoint)
  {
    // Unsupported line format!
    vtkErrorMacro(<< "Invalid Pts Format in the file:" << this->FileName);
    return 0;
  }

  // Lets setup the VTK Arrays and Points
  // get the info object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the output
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // If we are trying to limit the max number of points calculate the
  // onRatio - else set it to 1
  double onRatio = 1.0;
  vtkTypeInt32 targetNumPts = numPts;
  if (this->LimitToMaxNumberOfPoints)
  {
    onRatio = static_cast<double>(this->MaxNumberOfPoints) / numPts;
    targetNumPts = numPts * onRatio + 1;
  }

  vtkNew<vtkPoints> newPts;
  if (this->OutputDataTypeIsDouble)
  {
    newPts->SetDataTypeToDouble();
  }
  else
  {
    newPts->SetDataTypeToFloat();
  }
  newPts->Allocate(targetNumPts);

  vtkNew<vtkUnsignedCharArray> colors;
  vtkNew<vtkFloatArray> intensities;
  output->SetPoints(newPts);

  vtkNew<vtkCellArray> newVerts;
  if (this->CreateCells)
  {
    output->SetVerts(newVerts);
  }

  bool wantIntensities = (resultIntensity || resultColor);
  if (resultColor)
  {
    colors->SetNumberOfComponents(3);
    colors->SetName("Color");
    colors->Allocate(targetNumPts * 3);
    output->GetPointData()->SetScalars(colors);
    if (!this->IncludeColorAndLuminance)
    {
      wantIntensities = false;
    }
  }

  if (wantIntensities)
  {
    intensities->SetName("Intensities");
    intensities->SetNumberOfComponents(1);
    intensities->Allocate(targetNumPts);
    output->GetPointData()->AddArray(intensities);
  }

  if (numPts == 0)
  {
    // we are done
    return 1;
  }

  this->UpdateProgress(0.2);
  if (this->GetAbortExecute())
  {
    this->UpdateProgress(1.0);
    return 1;
  }

  // setup the ReadBBox, IF we're limiting the read to specified ReadBounds
  if (this->LimitReadToBounds)
  {
    this->ReadBBox.Reset();
    this->ReadBBox.SetMinPoint(this->ReadBounds[0], this->ReadBounds[2], this->ReadBounds[4]);
    this->ReadBBox.SetMaxPoint(this->ReadBounds[1], this->ReadBounds[3], this->ReadBounds[5]);
    // the ReadBBox is guaranteed to be "valid", regardless of the whether
    // ReadBounds is valid.  If any of the MonPoint values are greater than
    // the corresponding MaxPoint, the MinPoint component will be set to be
    // the same as the MaxPoint during the SetMaxPoint fn call.
  }

  // Lets Process the points!  Remember that we have already loaded in
  // the first line of points in the buffer
  vtkIdType* pids = nullptr;
  vtkIdType pid;
  if (this->CreateCells)
  {
    pids = new vtkIdType[targetNumPts];
  }
  const bool hasOnlyPoint = resultPoint && !resultIntensity && !resultColor;
  const bool hasOnlyPointAndIntensity = resultPoint && resultIntensity && !resultColor;
  long lastCount = 0;
  for (long i = 0; i < numPts; i++)
  {
    // Should we process this point?  Meaning that we skipped the appropriate number of points
    // based on the Max Number of points (onRatio) or the filtering by the read bounding box
    // OK to process based on Max Number of Points
    if (floor(i * onRatio) > lastCount)
    {
      lastCount++;
      if (hasOnlyPoint)
      {
        auto resultPointI = vtk::scan<double, double, double>(buffer, "{:f} {:f} {:f}");
        std::tie(pt[0], pt[1], pt[2]) = resultPointI->values();
      }
      else if (hasOnlyPointAndIntensity)
      {
        auto resultPointAndIntensityI =
          vtk::scan<double, double, double, double>(buffer, "{:f} {:f} {:f} {:f}");
        std::tie(pt[0], pt[1], pt[2], irgb[0]) = resultPointAndIntensityI->values();
      }
      else
      {
        auto resultPointAndIntensityAndColorI =
          vtk::scan<double, double, double, double, double, double, double>(
            buffer, "{:f} {:f} {:f} {:f} {:f} {:f} {:f}");
        std::tie(pt[0], pt[1], pt[2], irgb[0], irgb[1], irgb[2], irgb[3]) =
          resultPointAndIntensityAndColorI->values();
      }
      // OK to process based on bounding box
      if ((!this->LimitReadToBounds) || this->ReadBBox.ContainsPoint(pt))
      {
        pid = newPts->InsertNextPoint(pt);
        // std::cerr << "Point " << i << " : " << pt[0] << " " << pt[1] << " " << pt[2] << "\n";
        if (this->CreateCells)
        {
          pids[pid] = pid;
        }
        if (wantIntensities)
        {
          intensities->InsertNextValue(irgb[0]);
        }
        if (resultColor)
        {
          // if we have intensity then the color info starts with the second value in the array
          // else it starts with the first
          if (wantIntensities)
          {
            colors->InsertNextTuple(irgb + 1);
          }
          else
          {
            colors->InsertNextTuple(irgb);
          }
        }
      }
    }
    if (file.eof())
    {
      break;
    }
    if (i % 1000000 == 0)
    {
      this->UpdateProgress(0.2 + (0.75 * i) / numPts);
      if (this->GetAbortExecute())
      {
        return 0;
      }
    }
    getline(file, buffer);
  }

  // Do we have to squeeze any of the arrays?
  if (newPts->GetNumberOfPoints() < targetNumPts)
  {
    newPts->Squeeze();
    if (wantIntensities)
    {
      intensities->Squeeze();
    }
    if (resultColor)
    {
      colors->Squeeze();
    }
  }

  if (this->CreateCells)
  {
    newVerts->InsertNextCell(newPts->GetNumberOfPoints(), pids);
    delete[] pids;
  }

  this->UpdateProgress(1.0);
  return 1;
}
VTK_ABI_NAMESPACE_END
