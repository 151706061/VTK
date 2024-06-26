// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkRTXMLPolyDataReader.h"
#include "vtkDirectory.h"
#include "vtkObjectFactory.h"
#include <string>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkRTXMLPolyDataReader);

class vtkRTXMLPolyDataReaderInternals
{
public:
  std::vector<std::string> AvailableDataFileList;
  std::vector<std::string> ProcessedFileList;
};

//------------------------------------------------------------------------------
vtkRTXMLPolyDataReader::vtkRTXMLPolyDataReader()
{
  this->Internal = new vtkRTXMLPolyDataReaderInternals;
  this->DataLocation = nullptr;
}

//------------------------------------------------------------------------------
vtkRTXMLPolyDataReader::~vtkRTXMLPolyDataReader()
{
  delete this->Internal;
  this->SetDataLocation(nullptr);
}

void vtkRTXMLPolyDataReader::SetLocation(const char* dataLocation)
{
  this->SetDataLocation(dataLocation);
  this->ResetReader();
}

//------------------------------------------------------------------------------
void vtkRTXMLPolyDataReader::UpdateToNextFile()
{
  if (!this->Internal->AvailableDataFileList.empty())
  {
    // set the Reader to read the new available data file
    char* fullname = const_cast<char*>(this->Internal->AvailableDataFileList[0].c_str());
    this->SetFileName(fullname);
    this->Internal->ProcessedFileList.push_back(this->Internal->AvailableDataFileList[0]);
    this->Internal->AvailableDataFileList.erase(this->Internal->AvailableDataFileList.begin());
    this->Update();
    this->Modified();
  }
}

//------------------------------------------------------------------------------
const char* vtkRTXMLPolyDataReader::GetNextFileName()
{
  if (!this->Internal->AvailableDataFileList.empty())
  {
    return this->Internal->AvailableDataFileList[0].c_str();
  }
  else
  {
    return nullptr;
  }
}

//------------------------------------------------------------------------------
int vtkRTXMLPolyDataReader::NewDataAvailable()
{
  // TODO: data concurrency issue
  // what if data is available partly on the file system
  // i.e. a data writer is writing the file, but not done yet
  // enforce the writer to obtain a "flock" is too restrictive, is it?

  // no data directory is specified, use the current dir
  if (!this->DataLocation)
  {
    this->InitializeToCurrentDir();
    return VTK_ERROR;
  }

  // now the reader should be initialized already
  if (!this->Internal->AvailableDataFileList.empty())
  {
    return VTK_OK;
  }

  vtkDirectory* dataDir = vtkDirectory::New();
  dataDir->Open(this->DataLocation);

  // now check if there are new files available
  // and place them into the AvailableDataFileList
  int current = dataDir->GetNumberOfFiles();
  int processed = static_cast<int>(this->Internal->ProcessedFileList.size());

  if (current > processed)
  {
    for (int i = 0; i < current; i++)
    {
      char* file = this->GetDataFileFullPathName(dataDir->GetFile(i));
      if (!IsProcessed(file))
      {
        this->Internal->AvailableDataFileList.emplace_back(file);
      }
      else
      {
        delete[] file;
      }
    }
    dataDir->Delete();
    return VTK_OK;
  }
  else
  {
    dataDir->Delete();
    return VTK_ERROR;
  }
}

// Description:
// Internal method to return the fullpath name of a file, which
// is the concatenation of "this->DataLocation" and "name"
// caller has to free the memory of returned fullpath name.
//------------------------------------------------------------------------------
char* vtkRTXMLPolyDataReader::GetDataFileFullPathName(const char* name)
{
  char* fullpath;
  int n = static_cast<int>(strlen(this->DataLocation));
  int m = static_cast<int>(strlen(name));
  fullpath = new char[n + m + 2];
  strcpy(fullpath, this->DataLocation);
#if defined(_WIN32) // WINDOW style path
  if (fullpath[n - 1] != '/' && fullpath[n - 1] != '\\')
  {
#if !defined(__CYGWIN__)
    fullpath[n++] = '\\';
#else
    fullpath[n++] = '/';
#endif
  }
#else  // POSIX style path
  if (fullpath[n - 1] != '/')
  {
    fullpath[n++] = '/';
  }
#endif //
  strcpy(&fullpath[n], name);

  return fullpath;
}

void vtkRTXMLPolyDataReader::InitializeToCurrentDir()
{
  // TODO: need to add support for other platform like windows
  this->SetLocation("./");
}

//------------------------------------------------------------------------------
int vtkRTXMLPolyDataReader::IsProcessed(const char* fname)
{
  int size = static_cast<int>(this->Internal->ProcessedFileList.size());
  for (int i = 0; i < size; i++)
  {
    const char* file = this->Internal->ProcessedFileList[i].c_str();
    if (strcmp(fname, file) == 0)
    {
      return 1;
    }
  }
  return 0;
}

//------------------------------------------------------------------------------
void vtkRTXMLPolyDataReader::ResetReader()
{
  // assume the DataLocation is set at this point
  // clean up the two vectors first
  this->Internal->ProcessedFileList.clear();
  this->Internal->AvailableDataFileList.clear();

  vtkDirectory* dataDir = vtkDirectory::New();
  dataDir->Open(this->DataLocation);
  for (int i = 0; i < dataDir->GetNumberOfFiles(); i++)
  {
    this->Internal->ProcessedFileList.emplace_back(
      this->GetDataFileFullPathName(dataDir->GetFile(i)));
  }
  // initialize with an empty filename if filename is not set
  if (!this->GetFileName())
  {
    this->SetFileName("");
  }
  dataDir->Delete();
}

//------------------------------------------------------------------------------
void vtkRTXMLPolyDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DataLocation: " << (this->DataLocation ? this->DataLocation : "(none)") << "\n";
}
VTK_ABI_NAMESPACE_END
