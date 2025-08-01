// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// VTK_DEPRECATED_IN_9_5_0
#define VTK_DEPRECATION_LEVEL 0

#include "vtkDataObjectTreeIterator.h"

#include "vtkDataObjectTree.h"
#include "vtkDataObjectTreeInternals.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDataObjectTreeIterator::vtkInternals
{
public:
  // This implements a simple, no frills, depth-first iterator that iterates
  // over the composite dataset.
  class vtkIterator
  {
    vtkDataObject* DataObject;
    vtkDataObjectTree* CompositeDataSet;

    vtkDataObjectTreeInternals::Iterator Iter;
    vtkDataObjectTreeInternals::ReverseIterator ReverseIter;
    vtkIterator* ChildIterator;

    vtkInternals* Parent;
    bool Reverse;
    bool PassSelf;
    unsigned int ChildIndex;

    void InitChildIterator()
    {
      if (!this->ChildIterator)
      {
        this->ChildIterator = new vtkIterator(this->Parent);
      }
      this->ChildIterator->Initialize(this->Reverse, nullptr);

      if (this->Reverse &&
        this->ReverseIter != this->GetInternals(this->CompositeDataSet)->Children.rend())
      {
        this->ChildIterator->Initialize(this->Reverse, this->ReverseIter->DataObject);
      }
      else if (!this->Reverse &&
        this->Iter != this->GetInternals(this->CompositeDataSet)->Children.end())
      {
        this->ChildIterator->Initialize(this->Reverse, this->Iter->DataObject);
      }
    }

    vtkDataObjectTreeInternals* GetInternals(vtkDataObjectTree* cd)
    {
      return this->Parent->GetInternals(cd);
    }

  public:
    vtkIterator(vtkInternals* parent)
    {
      this->ChildIterator = nullptr;
      this->Parent = parent;
    }

    ~vtkIterator()
    {
      delete this->ChildIterator;
      this->ChildIterator = nullptr;
    }

    void Initialize(bool reverse, vtkDataObject* dataObj)
    {
      vtkDataObjectTree* compositeData = nullptr;
      if (vtkDataObjectTreeIterator::IsDataObjectTree(dataObj))
      {
        compositeData = static_cast<vtkDataObjectTree*>(dataObj);
      }
      this->Reverse = reverse;
      this->DataObject = dataObj;
      this->CompositeDataSet = compositeData;
      this->ChildIndex = 0;
      this->PassSelf = true;

      delete this->ChildIterator;
      this->ChildIterator = nullptr;

      if (compositeData)
      {
        this->Iter = this->GetInternals(compositeData)->Children.begin();
        this->ReverseIter = this->GetInternals(compositeData)->Children.rbegin();
        this->InitChildIterator();
      }
    }

    bool InSubTree()
    {
      if (this->PassSelf || this->IsDoneWithTraversal())
      {
        return false;
      }

      if (!this->ChildIterator)
      {
        return false;
      }

      if (this->ChildIterator->PassSelf)
      {
        return false;
      }

      return true;
    }

    bool IsDoneWithTraversal()
    {
      if (!this->DataObject)
      {
        return true;
      }

      if (this->PassSelf)
      {
        return false;
      }

      if (!this->CompositeDataSet)
      {
        return true;
      }

      if (this->Reverse &&
        this->ReverseIter == this->GetInternals(this->CompositeDataSet)->Children.rend())
      {
        return true;
      }

      if (!this->Reverse &&
        this->Iter == this->GetInternals(this->CompositeDataSet)->Children.end())
      {
        return true;
      }
      return false;
    }

    // Should not be called is this->IsDoneWithTraversal() returns true.
    vtkDataObject* GetCurrentDataObject()
    {
      if (this->PassSelf)
      {
        return this->DataObject;
      }
      return this->ChildIterator ? this->ChildIterator->GetCurrentDataObject() : nullptr;
    }

    vtkInformation* GetCurrentMetaData()
    {
      if (this->PassSelf || !this->ChildIterator)
      {
        return nullptr;
      }

      if (this->ChildIterator->PassSelf)
      {
        if (this->Reverse)
        {
          if (!this->ReverseIter->MetaData)
          {
            this->ReverseIter->MetaData.TakeReference(vtkInformation::New());
          }
          return this->ReverseIter->MetaData;
        }
        else
        {
          if (!this->Iter->MetaData)
          {
            this->Iter->MetaData.TakeReference(vtkInformation::New());
          }
          return this->Iter->MetaData;
        }
      }
      return this->ChildIterator->GetCurrentMetaData();
    }

    vtkTypeBool HasCurrentMetaData()
    {
      if (this->PassSelf || !this->ChildIterator)
      {
        return 0;
      }

      if (this->ChildIterator->PassSelf)
      {
        return this->Reverse ? (this->ReverseIter->MetaData != nullptr)
                             : (this->Iter->MetaData != nullptr);
      }

      return this->ChildIterator->HasCurrentMetaData();
    }

    // Go to the next element.
    void Next()
    {
      if (this->PassSelf)
      {
        this->PassSelf = false;
      }
      else if (this->ChildIterator)
      {
        this->ChildIterator->Next();
        if (this->ChildIterator->IsDoneWithTraversal())
        {
          this->ChildIndex++;
          if (this->Reverse)
          {
            ++this->ReverseIter;
          }
          else
          {
            ++this->Iter;
          }
          this->InitChildIterator();
        }
      }
    }

    // Returns the full-tree index for the current location.
    vtkDataObjectTreeIndex GetCurrentIndex()
    {
      vtkDataObjectTreeIndex index;
      if (this->PassSelf || this->IsDoneWithTraversal() || !this->ChildIterator)
      {
        return index;
      }
      vtkDataObjectTreeIndex childIndex = this->ChildIterator->GetCurrentIndex();
      childIndex.reserve(childIndex.size() + 1);
      index.push_back(this->ChildIndex);
      index.insert(index.end(), childIndex.begin(), childIndex.end());
      return index;
    }
  };

  // Description:
  // Helper method used by vtkInternals to get access to the internals of
  // vtkDataObjectTree.
  vtkDataObjectTreeInternals* GetInternals(vtkDataObjectTree* cd)
  {
    return this->CompositeDataIterator->GetInternals(cd);
  }

  vtkInternals() { this->Iterator = new vtkIterator(this); }
  ~vtkInternals()
  {
    delete this->Iterator;
    this->Iterator = nullptr;
  }

  vtkIterator* Iterator;
  vtkDataObjectTreeIterator* CompositeDataIterator;
};

vtkStandardNewMacro(vtkDataObjectTreeIterator);
//------------------------------------------------------------------------------
vtkDataObjectTreeIterator::vtkDataObjectTreeIterator()
{
  this->VisitOnlyLeaves = 1;
  this->TraverseSubTree = 1;
  this->CurrentFlatIndex = 0;
  this->Internals = new vtkInternals();
  this->Internals->CompositeDataIterator = this;
}

//------------------------------------------------------------------------------
vtkDataObjectTreeIterator::~vtkDataObjectTreeIterator()
{
  delete this->Internals;
}

//------------------------------------------------------------------------------
int vtkDataObjectTreeIterator::IsDoneWithTraversal()
{
  return this->Internals->Iterator->IsDoneWithTraversal();
}

//------------------------------------------------------------------------------
bool vtkDataObjectTreeIterator::IsDataObjectTree(vtkDataObject* dataObject)
{
  if (!dataObject)
  {
    return false;
  }
  switch (dataObject->GetDataObjectType())
  {
    case VTK_DATA_OBJECT_TREE:
    case VTK_PARTITIONED_DATA_SET:
    case VTK_PARTITIONED_DATA_SET_COLLECTION:
    case VTK_MULTIPIECE_DATA_SET:
    case VTK_MULTIBLOCK_DATA_SET:
    case VTK_UNIFORM_GRID_AMR:
    case VTK_NON_OVERLAPPING_AMR:
    case VTK_OVERLAPPING_AMR:
    case VTK_HIERARCHICAL_BOX_DATA_SET: // VTK_DEPRECATED_IN_9_5_0
      return true;
    default:
      return false;
  }
}

//------------------------------------------------------------------------------
void vtkDataObjectTreeIterator::InitializeInternal()
{
  this->SetCurrentFlatIndex(0);
  this->Internals->Iterator->Initialize(this->Reverse != 0, this->DataSet);
}

//------------------------------------------------------------------------------
void vtkDataObjectTreeIterator::GoToFirstItem()
{
  this->InitializeInternal();
  this->NextInternal();

  while (!this->Internals->Iterator->IsDoneWithTraversal())
  {
    vtkDataObject* dObj = this->Internals->Iterator->GetCurrentDataObject();
    if ((!dObj && this->SkipEmptyNodes) ||
      (this->VisitOnlyLeaves && vtkDataObjectTreeIterator::IsDataObjectTree(dObj)))
    {
      this->NextInternal();
    }
    else
    {
      break;
    }
  }
}

//------------------------------------------------------------------------------
void vtkDataObjectTreeIterator::GoToNextItem()
{
  if (!this->Internals->Iterator->IsDoneWithTraversal())
  {
    this->NextInternal();

    while (!this->Internals->Iterator->IsDoneWithTraversal())
    {
      vtkDataObject* dObj = this->Internals->Iterator->GetCurrentDataObject();
      if ((!dObj && this->SkipEmptyNodes) ||
        (this->VisitOnlyLeaves && vtkDataObjectTreeIterator::IsDataObjectTree(dObj)))
      {
        this->NextInternal();
      }
      else
      {
        break;
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkDataObjectTreeIterator::NextInternal()
{
  do
  {
    this->CurrentFlatIndex++;
    this->Internals->Iterator->Next();
  } while (!this->TraverseSubTree && this->Internals->Iterator->InSubTree());

  this->Modified();
}

//------------------------------------------------------------------------------
vtkDataObject* vtkDataObjectTreeIterator::GetCurrentDataObject()
{
  if (!this->IsDoneWithTraversal())
  {
    return this->Internals->Iterator->GetCurrentDataObject();
  }

  return nullptr;
}

//------------------------------------------------------------------------------
vtkInformation* vtkDataObjectTreeIterator::GetCurrentMetaData()
{
  if (!this->IsDoneWithTraversal())
  {
    return this->Internals->Iterator->GetCurrentMetaData();
  }

  return nullptr;
}

//------------------------------------------------------------------------------
vtkTypeBool vtkDataObjectTreeIterator::HasCurrentMetaData()
{
  if (!this->IsDoneWithTraversal())
  {
    return this->Internals->Iterator->HasCurrentMetaData();
  }

  return 0;
}

//------------------------------------------------------------------------------
vtkDataObjectTreeIndex vtkDataObjectTreeIterator::GetCurrentIndex()
{
  return this->Internals->Iterator->GetCurrentIndex();
}

//------------------------------------------------------------------------------
unsigned int vtkDataObjectTreeIterator::GetCurrentFlatIndex()
{
  if (this->Reverse)
  {
    vtkErrorMacro("FlatIndex cannot be obtained when iterating in reverse order.");
    return 0;
  }
  return this->CurrentFlatIndex;
}

//------------------------------------------------------------------------------
vtkDataObjectTreeInternals* vtkDataObjectTreeIterator::GetInternals(vtkDataObjectTree* cd)
{
  if (cd)
  {
    return cd->Internals;
  }

  return nullptr;
}

//------------------------------------------------------------------------------
void vtkDataObjectTreeIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "VisitOnlyLeaves: " << (this->VisitOnlyLeaves ? "On" : "Off") << endl;
  os << indent << "Reverse: " << (this->Reverse ? "On" : "Off") << endl;
  os << indent << "TraverseSubTree: " << (this->TraverseSubTree ? "On" : "Off") << endl;
  os << indent << "SkipEmptyNodes: " << (this->SkipEmptyNodes ? "On" : "Off") << endl;
  os << indent << "CurrentFlatIndex: " << this->CurrentFlatIndex << endl;
}
VTK_ABI_NAMESPACE_END
