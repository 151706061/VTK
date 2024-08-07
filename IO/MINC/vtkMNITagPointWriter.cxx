// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2006 Atamai, Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkMNITagPointWriter.h"

#include "vtkObjectFactory.h"

#include "vtkCommand.h"
#include "vtkDoubleArray.h"
#include "vtkErrorCode.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkStringArray.h"
#include "vtksys/FStream.hxx"

#include <cctype>
#include <cmath>

#if !defined(_WIN32) || defined(__CYGWIN__)
#include <unistd.h> /* unlink */
#else
#include <io.h> /* unlink */
#endif

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkMNITagPointWriter);

vtkCxxSetObjectMacro(vtkMNITagPointWriter, LabelText, vtkStringArray);
vtkCxxSetObjectMacro(vtkMNITagPointWriter, Weights, vtkDoubleArray);
vtkCxxSetObjectMacro(vtkMNITagPointWriter, StructureIds, vtkIntArray);
vtkCxxSetObjectMacro(vtkMNITagPointWriter, PatientIds, vtkIntArray);

//------------------------------------------------------------------------------
vtkMNITagPointWriter::vtkMNITagPointWriter()
{
  this->Points[0] = nullptr;
  this->Points[1] = nullptr;

  this->LabelText = nullptr;
  this->Weights = nullptr;
  this->StructureIds = nullptr;
  this->PatientIds = nullptr;

  this->Comments = nullptr;

  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(0);

  this->FileName = nullptr;
}

//------------------------------------------------------------------------------
vtkMNITagPointWriter::~vtkMNITagPointWriter()
{
  vtkObject* objects[6];
  objects[0] = this->Points[0];
  objects[1] = this->Points[1];
  objects[2] = this->LabelText;
  objects[3] = this->Weights;
  objects[4] = this->StructureIds;
  objects[5] = this->PatientIds;

  for (int i = 0; i < 6; i++)
  {
    if (objects[i])
    {
      objects[i]->Delete();
    }
  }

  delete[] this->Comments;

  delete[] this->FileName;
}

//------------------------------------------------------------------------------
void vtkMNITagPointWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Points: " << this->Points[0] << " " << this->Points[1] << "\n";
  os << indent << "LabelText: " << this->LabelText << "\n";
  os << indent << "Weights: " << this->Weights << "\n";
  os << indent << "StructureIds: " << this->StructureIds << "\n";
  os << indent << "PatientIds: " << this->PatientIds << "\n";

  os << indent << "Comments: " << (this->Comments ? this->Comments : "none") << "\n";
}

//------------------------------------------------------------------------------
int vtkMNITagPointWriter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//------------------------------------------------------------------------------
vtkMTimeType vtkMNITagPointWriter::GetMTime()
{
  vtkMTimeType mtime = this->Superclass::GetMTime();

  vtkObject* objects[6];
  objects[0] = this->Points[0];
  objects[1] = this->Points[1];
  objects[2] = this->LabelText;
  objects[3] = this->Weights;
  objects[4] = this->StructureIds;
  objects[5] = this->PatientIds;

  for (int i = 0; i < 6; i++)
  {
    if (objects[i])
    {
      vtkMTimeType m = objects[i]->GetMTime();
      if (m > mtime)
      {
        mtime = m;
      }
    }
  }

  return mtime;
}

//------------------------------------------------------------------------------
void vtkMNITagPointWriter::SetPoints(int port, vtkPoints* points)
{
  if (port < 0 || port > 1)
  {
    return;
  }
  if (this->Points[port] == points)
  {
    return;
  }
  if (this->Points[port])
  {
    this->Points[port]->Delete();
  }
  this->Points[port] = points;
  if (this->Points[port])
  {
    this->Points[port]->Register(this);
  }
  this->Modified();
}

//------------------------------------------------------------------------------
vtkPoints* vtkMNITagPointWriter::GetPoints(int port)
{
  if (port < 0 || port > 1)
  {
    return nullptr;
  }
  return this->Points[port];
}

//------------------------------------------------------------------------------
void vtkMNITagPointWriter::WriteData(vtkPointSet* inputs[2])
{
  static const char* arrayNames[3] = { "Weights", "StructureIds", "PatientIds" };

  vtkPoints* points[2];

  vtkStringArray* labels = nullptr;
  vtkDataArray* darray[3];
  darray[0] = nullptr;
  darray[1] = nullptr;
  darray[2] = nullptr;

  vtkDataArray* ivarArrays[3];
  ivarArrays[0] = this->Weights;
  ivarArrays[1] = this->StructureIds;
  ivarArrays[2] = this->PatientIds;

  for (int ii = 1; ii >= 0; --ii)
  {
    points[ii] = nullptr;
    if (inputs[ii])
    {
      points[ii] = inputs[ii]->GetPoints();

      vtkStringArray* stringArray =
        vtkArrayDownCast<vtkStringArray>(inputs[ii]->GetPointData()->GetAbstractArray("LabelText"));
      if (stringArray)
      {
        labels = stringArray;
      }

      for (int j = 0; j < 3; j++)
      {
        vtkDataArray* dataArray = inputs[ii]->GetPointData()->GetArray(arrayNames[j]);
        if (dataArray)
        {
          darray[j] = dataArray;
        }
      }
    }

    if (this->Points[ii])
    {
      points[ii] = this->Points[ii];
    }
  }

  if (this->LabelText)
  {
    labels = this->LabelText;
  }

  for (int j = 0; j < 3; j++)
  {
    if (ivarArrays[j])
    {
      darray[j] = ivarArrays[j];
    }
  }

  if (points[0] == nullptr)
  {
    vtkErrorMacro("No input points have been provided");
    return;
  }

  // numVolumes is 1 if there is only one set of points
  int numVolumes = 1;
  vtkIdType n = points[0]->GetNumberOfPoints();
  if (points[1])
  {
    numVolumes = 2;
    if (points[1]->GetNumberOfPoints() != n)
    {
      vtkErrorMacro(
        "Input point counts do not match: " << n << " versus " << points[1]->GetNumberOfPoints());
      return;
    }
  }

  // labels is null if there are no labels
  if (labels && labels->GetNumberOfValues() != n)
  {
    vtkErrorMacro("LabelText count does not match point count: " << labels->GetNumberOfValues()
                                                                 << " versus " << n);
    return;
  }

  // dataArrays is null if there are no data arrays
  vtkDataArray** dataArrays = nullptr;
  for (int jj = 0; jj < 3; jj++)
  {
    if (darray[jj])
    {
      dataArrays = darray;
      if (darray[jj]->GetNumberOfTuples() != n)
      {
        vtkErrorMacro("" << arrayNames[jj] << " count does not match point count: "
                         << darray[jj]->GetNumberOfTuples() << " versus " << n);
        return;
      }
    }
  }

  // If we got this far, the data seems to be okay
  ostream* outfilep = this->OpenFile();
  if (!outfilep)
  {
    return;
  }

  ostream& outfile = *outfilep;

  // Write the header
  outfile << "MNI Tag Point File\n";
  outfile << "Volumes = " << numVolumes << ";\n";

  // Write user comments
  if (this->Comments)
  {
    char* cp = this->Comments;
    while (*cp)
    {
      if (*cp != '%')
      {
        outfile << "% ";
      }
      while (*cp && *cp != '\n')
      {
        if (isprint(*cp) || *cp == '\t')
        {
          outfile << *cp;
        }
        cp++;
      }
      outfile << "\n";
      if (*cp == '\n')
      {
        cp++;
      }
    }
  }
  else
  {
    for (int k = 0; k < numVolumes; k++)
    {
      outfile << "% Volume " << (k + 1) << " produced by VTK\n";
    }
  }

  // Add a blank line
  outfile << "\n";

  // Write the points
  outfile << "Points =\n";

  char text[256];
  for (int i = 0; i < n; i++)
  {
    for (int kk = 0; kk < 2; kk++)
    {
      if (points[kk])
      {
        double point[3];
        points[kk]->GetPoint(i, point);
        snprintf(text, sizeof(text), " %.15g %.15g %.15g", point[0], point[1], point[2]);
        outfile << text;
      }
    }

    if (dataArrays)
    {
      double w = 0.0;
      int s = -1;
      int p = -1;
      if (dataArrays[0])
      {
        w = dataArrays[0]->GetComponent(i, 0);
      }
      if (dataArrays[1])
      {
        s = static_cast<int>(dataArrays[1]->GetComponent(i, 0));
      }
      if (dataArrays[2])
      {
        p = static_cast<int>(dataArrays[2]->GetComponent(i, 0));
      }

      snprintf(text, sizeof(text), " %.15g %d %d", w, s, p);
      outfile << text;
    }

    if (labels)
    {
      std::string l = labels->GetValue(i);
      outfile << " \"";
      for (std::string::iterator si = l.begin(); si != l.end(); ++si)
      {
        if (isprint(*si) && *si != '\"' && *si != '\\')
        {
          outfile.put(*si);
        }
        else
        {
          outfile.put('\\');
          char c = '\0';
          static char ctrltable[] = { '\a', 'a', '\b', 'b', '\f', 'f', '\n', 'n', '\r', 'r', '\t',
            't', '\v', 'v', '\\', '\\', '\"', '\"', '\0', '\0' };
          for (int ci = 0; ctrltable[ci] != '\0'; ci += 2)
          {
            if (*si == ctrltable[ci])
            {
              c = ctrltable[ci + 1];
              break;
            }
          }
          if (c != '\0')
          {
            outfile.put(c);
          }
          else
          {
            snprintf(text, sizeof(text), "x%2.2x", (static_cast<unsigned int>(*si) & 0x00ff));
            outfile << text;
          }
        }
      }

      outfile << "\"";
    }

    if (i < n - 1)
    {
      outfile << "\n";
    }
  }

  outfile << ";\n";
  outfile.flush();

  // Close the file
  this->CloseFile(outfilep);

  // Delete the file if an error occurred
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    vtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    unlink(this->FileName);
  }
}

//------------------------------------------------------------------------------
int vtkMNITagPointWriter::Write()
{
  // Allow writer to work when no inputs are provided
  this->Modified();
  this->Update();
  return 1;
}

//------------------------------------------------------------------------------
int vtkMNITagPointWriter::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector*)
{
  this->SetErrorCode(vtkErrorCode::NoError);

  vtkInformation* inInfo[2];
  inInfo[0] = inputVector[0]->GetInformationObject(0);
  inInfo[1] = inputVector[1]->GetInformationObject(0);

  vtkPointSet* input[2];
  input[0] = nullptr;
  input[1] = nullptr;

  vtkMTimeType lastUpdateTime = 0;
  for (int idx = 0; idx < 2; ++idx)
  {
    if (inInfo[idx])
    {
      input[idx] = vtkPointSet::SafeDownCast(inInfo[idx]->Get(vtkDataObject::DATA_OBJECT()));
      if (input[idx])
      {
        vtkMTimeType updateTime = input[idx]->GetUpdateTime();
        if (updateTime > lastUpdateTime)
        {
          lastUpdateTime = updateTime;
        }
      }
    }
  }

  if (lastUpdateTime < this->WriteTime && this->GetMTime() < this->WriteTime)
  {
    // we are up to date
    return 1;
  }

  this->InvokeEvent(vtkCommand::StartEvent, nullptr);
  this->WriteData(input);
  this->InvokeEvent(vtkCommand::EndEvent, nullptr);

  this->WriteTime.Modified();

  return 1;
}

//------------------------------------------------------------------------------
ostream* vtkMNITagPointWriter::OpenFile()
{
  ostream* fptr;

  if (!this->FileName)
  {
    vtkErrorMacro(<< "No FileName specified! Can't write!");
    this->SetErrorCode(vtkErrorCode::NoFileNameError);
    return nullptr;
  }

  vtkDebugMacro(<< "Opening file for writing...");

  fptr = new vtksys::ofstream(this->FileName, ios::out);

  if (fptr->fail())
  {
    vtkErrorMacro(<< "Unable to open file: " << this->FileName);
    this->SetErrorCode(vtkErrorCode::CannotOpenFileError);
    delete fptr;
    return nullptr;
  }

  return fptr;
}

//------------------------------------------------------------------------------
void vtkMNITagPointWriter::CloseFile(ostream* fp)
{
  vtkDebugMacro(<< "Closing file\n");

  delete fp;
}
VTK_ABI_NAMESPACE_END
