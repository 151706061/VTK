// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkXMLHierarchicalBoxDataFileConverter.h"

#include "vtkBoundingBox.h"
#include "vtkExecutive.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkStructuredData.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLDataParser.h"
#include "vtkXMLImageDataReader.h"
#include "vtksys/SystemTools.hxx"

#include <cassert>
#include <map>
#include <set>
#include <string>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkXMLHierarchicalBoxDataFileConverter);
//------------------------------------------------------------------------------
vtkXMLHierarchicalBoxDataFileConverter::vtkXMLHierarchicalBoxDataFileConverter()
{
  this->InputFileName = nullptr;
  this->OutputFileName = nullptr;
  this->FilePath = nullptr;
}

//------------------------------------------------------------------------------
vtkXMLHierarchicalBoxDataFileConverter::~vtkXMLHierarchicalBoxDataFileConverter()
{
  this->SetInputFileName(nullptr);
  this->SetOutputFileName(nullptr);
  this->SetFilePath(nullptr);
}

//------------------------------------------------------------------------------
bool vtkXMLHierarchicalBoxDataFileConverter::Convert()
{
  if (!this->InputFileName)
  {
    vtkErrorMacro("Missing InputFileName.");
    return false;
  }

  if (!this->OutputFileName)
  {
    vtkErrorMacro("Missing OutputFileName.");
    return false;
  }

  vtkSmartPointer<vtkXMLDataElement> dom;
  dom.TakeReference(this->ParseXML(this->InputFileName));
  if (!dom)
  {
    return false;
  }

  // Ensure this file we can convert.
  if (dom->GetName() == nullptr || strcmp(dom->GetName(), "VTKFile") != 0 ||
    dom->GetAttribute("type") == nullptr ||
    strcmp(dom->GetAttribute("type"), "vtkHierarchicalBoxDataSet") != 0 ||
    dom->GetAttribute("version") == nullptr || strcmp(dom->GetAttribute("version"), "1.0") != 0)
  {
    vtkErrorMacro("Cannot convert the input file: " << this->InputFileName);
    return false;
  }

  dom->SetAttribute("version", "1.1");
  dom->SetAttribute("type", "vtkOverlappingAMR");

  // locate primary element.
  vtkXMLDataElement* ePrimary = dom->FindNestedElementWithName("vtkHierarchicalBoxDataSet");
  if (!ePrimary)
  {
    vtkErrorMacro("Failed to locate primary element.");
    return false;
  }

  ePrimary->SetName("vtkOverlappingAMR");

  // Find the path to this file in case the internal files are
  // specified as relative paths.
  std::string filePath = this->InputFileName;
  std::string::size_type pos = filePath.find_last_of("/\\");
  if (pos != std::string::npos)
  {
    filePath = filePath.substr(0, pos);
  }
  else
  {
    filePath = "";
  }
  this->SetFilePath(filePath.c_str());

  // We need origin for level 0, and spacing for all levels.
  double origin[3];
  double* spacing = nullptr;

  int gridDescription = this->GetOriginAndSpacing(ePrimary, origin, spacing);
  if (gridDescription < vtkStructuredData::VTK_STRUCTURED_XY_PLANE ||
    gridDescription > vtkStructuredData::VTK_STRUCTURED_XYZ_GRID)
  {
    delete[] spacing;
    vtkErrorMacro("Failed to determine origin/spacing/grid description.");
    return false;
  }

  //  cout << "Origin: " << origin[0] <<", " << origin[1] << ", " << origin[2]
  //    << endl;
  //  cout << "Spacing: " << spacing[0] << ", " << spacing[1] << ", " << spacing[2]
  //    << endl;

  const char* grid_description = "XYZ";
  switch (gridDescription)
  {
    case vtkStructuredData::VTK_STRUCTURED_XY_PLANE:
      grid_description = "XY";
      break;
    case vtkStructuredData::VTK_STRUCTURED_XZ_PLANE:
      grid_description = "XZ";
      break;
    case vtkStructuredData::VTK_STRUCTURED_YZ_PLANE:
      grid_description = "YZ";
      break;
  }

  ePrimary->SetAttribute("grid_description", grid_description);
  ePrimary->SetVectorAttribute("origin", 3, origin);

  // Now iterate over all "<Block>" elements and update them.
  for (int cc = 0; cc < ePrimary->GetNumberOfNestedElements(); cc++)
  {
    int level = 0;
    vtkXMLDataElement* block = ePrimary->GetNestedElement(cc);
    // iterate over all <DataSet> inside the current block
    // and replace the folder for the file attribute.
    for (int i = 0; i < block->GetNumberOfNestedElements(); ++i)
    {
      vtkXMLDataElement* dataset = block->GetNestedElement(i);
      std::string file(dataset->GetAttribute("file"));
      std::string fileNoDir(vtksys::SystemTools::GetFilenameName(file));
      std::string dir(vtksys::SystemTools::GetFilenameWithoutLastExtension(this->OutputFileName));
      dataset->SetAttribute("file", (dir + '/' + fileNoDir).c_str());
    }
    if (block && block->GetName() && strcmp(block->GetName(), "Block") == 0 &&
      block->GetScalarAttribute("level", level) && level >= 0)
    {
    }
    else
    {
      continue;
    }
    block->SetVectorAttribute("spacing", 3, &spacing[3 * level]);
    block->RemoveAttribute("refinement_ratio");
  }
  delete[] spacing;

  // now save the xml out.
  dom->PrintXML(this->OutputFileName);
  return true;
}

//------------------------------------------------------------------------------
vtkXMLDataElement* vtkXMLHierarchicalBoxDataFileConverter::ParseXML(const char* fname)
{
  assert(fname);

  vtkNew<vtkXMLDataParser> parser;
  parser->SetFileName(fname);
  if (!parser->Parse())
  {
    vtkErrorMacro("Failed to parse input XML: " << fname);
    return nullptr;
  }

  vtkXMLDataElement* element = parser->GetRootElement();
  element->Register(this);
  return element;
}

//------------------------------------------------------------------------------
int vtkXMLHierarchicalBoxDataFileConverter::GetOriginAndSpacing(
  vtkXMLDataElement* ePrimary, double origin[3], double*& spacing)
{
  // Build list of filenames for all levels.
  std::map<int, std::set<std::string>> filenames;

  for (int cc = 0; cc < ePrimary->GetNumberOfNestedElements(); cc++)
  {
    int level = 0;

    vtkXMLDataElement* child = ePrimary->GetNestedElement(cc);
    if (child && child->GetName() && strcmp(child->GetName(), "Block") == 0 &&
      child->GetScalarAttribute("level", level) && level >= 0)
    {
    }
    else
    {
      continue;
    }

    for (int kk = 0; kk < child->GetNumberOfNestedElements(); kk++)
    {
      vtkXMLDataElement* dsElement = child->GetNestedElement(cc);
      if (dsElement && dsElement->GetName() && strcmp(dsElement->GetName(), "DataSet") == 0 &&
        dsElement->GetAttribute("file") != nullptr)
      {
        std::string file = dsElement->GetAttribute("file");
        if (file.c_str()[0] != '/' && file.c_str()[1] != ':')
        {
          std::string prefix = this->FilePath;
          if (!prefix.empty())
          {
            prefix += "/";
          }
          file.insert(0, prefix);
        }
        filenames[level].insert(file);
      }
    }
  }

  vtkBoundingBox bbox;
  int gridDescription = vtkStructuredData::VTK_STRUCTURED_UNCHANGED;
  spacing = new double[3 * filenames.size() + 1];
  memset(spacing, 0, (3 * filenames.size() + 1) * sizeof(double));

  // Now read all the datasets at level 0.
  for (std::set<std::string>::iterator iter = filenames[0].begin(); iter != filenames[0].end();
       ++iter)
  {
    vtkNew<vtkXMLImageDataReader> imageReader;
    imageReader->SetFileName((*iter).c_str());
    imageReader->Update();

    vtkImageData* image = imageReader->GetOutput();
    if (image && vtkMath::AreBoundsInitialized(image->GetBounds()))
    {
      if (!bbox.IsValid())
      {
        gridDescription = vtkStructuredData::GetDataDescription(image->GetDimensions());
      }
      bbox.AddBounds(image->GetBounds());
    }
  }

  if (bbox.IsValid())
  {
    bbox.GetMinPoint(origin[0], origin[1], origin[2]);
  }

  // Read 1 dataset from each level to get information about spacing.
  for (std::map<int, std::set<std::string>>::iterator iter = filenames.begin();
       iter != filenames.end(); ++iter)
  {
    if (iter->second.empty())
    {
      continue;
    }

    std::string filename = (*iter->second.begin());
    vtkNew<vtkXMLImageDataReader> imageReader;
    imageReader->SetFileName(filename.c_str());
    imageReader->UpdateInformation();
    vtkInformation* outInfo = imageReader->GetExecutive()->GetOutputInformation(0);
    if (outInfo->Has(vtkDataObject::SPACING()))
    {
      assert(outInfo->Length(vtkDataObject::SPACING()) == 3);
      outInfo->Get(vtkDataObject::SPACING(), &spacing[3 * iter->first]);
    }
  }

  return gridDescription;
}

//------------------------------------------------------------------------------
void vtkXMLHierarchicalBoxDataFileConverter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "InputFileName: " << (this->InputFileName ? this->InputFileName : "(none)")
     << endl;
  os << indent << "OutputFileName: " << (this->OutputFileName ? this->OutputFileName : "(none)")
     << endl;
}
VTK_ABI_NAMESPACE_END
