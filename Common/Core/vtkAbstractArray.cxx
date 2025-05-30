// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAbstractArray.h"

#include "vtkArrayDispatch.h"
#include "vtkBitArray.h"
#include "vtkCharArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationInformationVectorKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationVariantVectorKey.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkLongLongArray.h"
#include "vtkMath.h"
#include "vtkMinimalStandardRandomSequence.h"
#include "vtkNew.h"
#include "vtkShortArray.h"
#include "vtkSignedCharArray.h"
#include "vtkStringArray.h"
#include "vtkTypeFloat32Array.h"
#include "vtkTypeFloat64Array.h"
#include "vtkTypeInt16Array.h"
#include "vtkTypeInt32Array.h"
#include "vtkTypeInt64Array.h"
#include "vtkTypeInt8Array.h"
#include "vtkTypeUInt16Array.h"
#include "vtkTypeUInt32Array.h"
#include "vtkTypeUInt64Array.h"
#include "vtkTypeUInt8Array.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedLongLongArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkVariantArray.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <set>

VTK_ABI_NAMESPACE_BEGIN
vtkInformationKeyMacro(vtkAbstractArray, GUI_HIDE, Integer);
vtkInformationKeyMacro(vtkAbstractArray, PER_COMPONENT, InformationVector);
vtkInformationKeyMacro(vtkAbstractArray, PER_FINITE_COMPONENT, InformationVector);
vtkInformationKeyMacro(vtkAbstractArray, DISCRETE_VALUES, VariantVector);
vtkInformationKeyRestrictedMacro(
  vtkAbstractArray, DISCRETE_VALUE_SAMPLE_PARAMETERS, DoubleVector, 2);
VTK_ABI_NAMESPACE_END

namespace
{
typedef std::vector<std::string*> vtkInternalComponentNameBase;

struct PrintDataArrayWorker
{
  template <typename InArrayT>
  void operator()(InArrayT* inArray, std::ostream& outStream)
  {
    using T = vtk::GetAPIType<InArrayT>;
    const auto inRange = vtk::DataArrayValueRange(inArray);
    std::copy(inRange.begin(), inRange.end(), std::ostream_iterator<T>(outStream, " "));
  }
};
}

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractArray::vtkInternalComponentNames : public vtkInternalComponentNameBase
{
};

//------------------------------------------------------------------------------
// Construct object with sane defaults.
vtkAbstractArray::vtkAbstractArray()
{
  this->Size = 0;
  this->MaxId = -1;
  this->NumberOfComponents = 1;
  this->Name = nullptr;
  this->RebuildArray = false;
  this->Information = nullptr;
  this->ComponentNames = nullptr;

  this->MaxDiscreteValues = vtkAbstractArray::MAX_DISCRETE_VALUES; // 32
}

//------------------------------------------------------------------------------
vtkAbstractArray::~vtkAbstractArray()
{
  if (this->ComponentNames)
  {
    for (unsigned int i = 0; i < this->ComponentNames->size(); ++i)
    {
      delete this->ComponentNames->at(i);
    }
    this->ComponentNames->clear();
    delete this->ComponentNames;
    this->ComponentNames = nullptr;
  }

  this->SetName(nullptr);
  this->SetInformation(nullptr);
}

//------------------------------------------------------------------------------
void vtkAbstractArray::SetComponentName(vtkIdType component, const char* name)
{
  if (component < 0 || name == nullptr)
  {
    return;
  }
  unsigned int index = static_cast<unsigned int>(component);
  if (this->ComponentNames == nullptr)
  {
    // delayed allocate
    this->ComponentNames = new vtkAbstractArray::vtkInternalComponentNames();
  }

  if (index == this->ComponentNames->size())
  {
    // the array isn't large enough, so we will resize
    this->ComponentNames->push_back(new std::string(name));
    return;
  }
  else if (index > this->ComponentNames->size())
  {
    this->ComponentNames->resize(index + 1, nullptr);
  }

  // replace an existing element
  std::string* compName = this->ComponentNames->at(index);
  if (!compName)
  {
    compName = new std::string(name);
    this->ComponentNames->at(index) = compName;
  }
  else
  {
    compName->assign(name);
  }
}

//------------------------------------------------------------------------------
const char* vtkAbstractArray::GetComponentName(vtkIdType component) const
{
  unsigned int index = static_cast<unsigned int>(component);
  if (!this->ComponentNames || component < 0 || index >= this->ComponentNames->size())
  {
    // make sure we have valid vector
    return nullptr;
  }

  std::string* compName = this->ComponentNames->at(index);
  return (compName) ? compName->c_str() : nullptr;
}

//------------------------------------------------------------------------------
bool vtkAbstractArray::HasAComponentName() const
{
  return this->ComponentNames && !this->ComponentNames->empty();
}

//------------------------------------------------------------------------------
int vtkAbstractArray::CopyComponentNames(vtkAbstractArray* da)
{
  if (da && da != this && da->ComponentNames)
  {
    // clear the vector of the all data
    if (!this->ComponentNames)
    {
      this->ComponentNames = new vtkAbstractArray::vtkInternalComponentNames();
    }

    // copy the passed in components
    for (unsigned int i = 0; i < this->ComponentNames->size(); ++i)
    {
      delete this->ComponentNames->at(i);
    }
    this->ComponentNames->clear();
    this->ComponentNames->reserve(da->ComponentNames->size());
    const char* name;
    for (unsigned int i = 0; i < da->ComponentNames->size(); ++i)
    {
      name = da->GetComponentName(i);
      if (name)
      {
        this->SetComponentName(i, name);
      }
    }
    return 1;
  }
  return 0;
}

//------------------------------------------------------------------------------
bool vtkAbstractArray::SetNumberOfValues(vtkIdType numValues)
{
  vtkIdType numTuples = numValues / this->NumberOfComponents;
  if (numValues % this->NumberOfComponents)
  {
    ++numTuples;
  }
  if (!this->Resize(numTuples))
  {
    return false;
  }
  this->MaxId = numValues - 1;
  return true;
}

//------------------------------------------------------------------------------
void vtkAbstractArray::SetInformation(vtkInformation* args)
{
  // Same as in vtkCxxSetObjectMacro, but no Modified() so that
  // this doesn't cause extra pipeline updates.
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting Information to " << args);
  if (this->Information != args)
  {
    vtkInformation* tempSGMacroVar = this->Information;
    this->Information = args;
    if (this->Information != nullptr)
    {
      this->Information->Register(this);
    }
    if (tempSGMacroVar != nullptr)
    {
      tempSGMacroVar->UnRegister(this);
    }
  }
}

//------------------------------------------------------------------------------
void vtkAbstractArray::GetTuples(vtkIdList* tupleIds, vtkAbstractArray* aa)
{
  if (aa->GetNumberOfComponents() != this->GetNumberOfComponents())
  {
    vtkWarningMacro("Number of components for input and output do not match.");
    return;
  }
  // Here we give the slowest implementation. Subclasses can override
  // to use the knowledge about the data.
  vtkIdType num = tupleIds->GetNumberOfIds();
  for (vtkIdType i = 0; i < num; i++)
  {
    aa->SetTuple(i, tupleIds->GetId(i), this);
  }
}

//------------------------------------------------------------------------------
void vtkAbstractArray::GetTuples(vtkIdType p1, vtkIdType p2, vtkAbstractArray* aa)
{
  if (aa->GetNumberOfComponents() != this->GetNumberOfComponents())
  {
    vtkWarningMacro("Number of components for input and output do not match.");
    return;
  }

  // Here we give the slowest implementation. Subclasses can override
  // to use the knowledge about the data.
  vtkIdType num = p2 - p1 + 1;
  for (vtkIdType i = 0; i < num; i++)
  {
    aa->SetTuple(i, (p1 + i), this);
  }
}

//------------------------------------------------------------------------------
bool vtkAbstractArray::HasStandardMemoryLayout() const
{
  return true;
}

//------------------------------------------------------------------------------
void vtkAbstractArray::DeepCopy(vtkAbstractArray* da)
{
  if (!da || da == this)
  {
    return;
  }

  if (da->HasInformation())
  {
    this->CopyInformation(da->GetInformation(), /*deep=*/1);
  }
  else
  {
    this->SetInformation(nullptr);
  }

  this->SetName(da->Name);

  this->CopyComponentNames(da);
}

//------------------------------------------------------------------------------
void vtkAbstractArray::ExportToVoidPointer(void* dest)
{
  if (this->MaxId > 0 && this->GetDataTypeSize() > 0)
  {
    void* src = this->GetVoidPointer(0);
    memcpy(dest, src, ((this->MaxId + 1) * this->GetDataTypeSize()));
  }
}

//------------------------------------------------------------------------------
int vtkAbstractArray::CopyInformation(vtkInformation* infoFrom, vtkTypeBool deep)
{
  // Copy all keys. NOTE: subclasses rely on this.
  vtkInformation* myInfo = this->GetInformation();
  myInfo->Copy(infoFrom, deep);

  // Remove any keys we own that are not to be copied here.
  // For now, remove per-component metadata.
  myInfo->Remove(PER_COMPONENT());
  myInfo->Remove(PER_FINITE_COMPONENT());
  myInfo->Remove(DISCRETE_VALUES());

  return 1;
}

//------------------------------------------------------------------------------
// call modified on superclass
void vtkAbstractArray::Modified()
{
  if (this->HasInformation())
  {
    vtkInformation* info = this->GetInformation();
    // Clear key-value pairs that are now out of date.
    info->Remove(PER_COMPONENT());
    info->Remove(PER_FINITE_COMPONENT());
  }
  this->Superclass::Modified();
}

//------------------------------------------------------------------------------
vtkInformation* vtkAbstractArray::GetInformation()
{
  if (!this->Information)
  {
    vtkInformation* info = vtkInformation::New();
    this->SetInformation(info);
    info->FastDelete();
  }
  return this->Information;
}

//------------------------------------------------------------------------------
template <class T>
int vtkAbstractArrayGetDataTypeSize(T*)
{
  return sizeof(T);
}

int vtkAbstractArray::GetDataTypeSize(int type)
{
  switch (type)
  {
    vtkTemplateMacro(return vtkAbstractArrayGetDataTypeSize(static_cast<VTK_TT*>(nullptr)));

    case VTK_BIT:
    case VTK_STRING:
      return 0;

    default:
      vtkGenericWarningMacro(<< "Unsupported data type!");
  }

  return 1;
}

//------------------------------------------------------------------------------
vtkAbstractArray* vtkAbstractArray::CreateArray(int dataType)
{
  // First we check IntT and FloatN type to allow downcasting to such array types.
  switch (dataType)
  {
    case VTK_TYPE_UINT8:
      return vtkTypeUInt8Array::New();

    case VTK_TYPE_INT8:
      return vtkTypeInt8Array::New();

    case VTK_TYPE_UINT16:
      return vtkTypeUInt16Array::New();

    case VTK_TYPE_INT16:
      return vtkTypeInt16Array::New();

    case VTK_TYPE_UINT32:
      return vtkTypeUInt32Array::New();

    case VTK_TYPE_INT32:
      return vtkTypeInt32Array::New();

    case VTK_TYPE_UINT64:
      return vtkTypeUInt64Array::New();

    case VTK_TYPE_INT64:
      return vtkTypeInt64Array::New();

    case VTK_TYPE_FLOAT32:
      return vtkTypeFloat32Array::New();

    case VTK_TYPE_FLOAT64:
      return vtkTypeFloat64Array::New();

    default:
      break;
  }

  // Didn't find any IntN or FloatN? The array we want should be down there.
  switch (dataType)
  {
    case VTK_BIT:
      return vtkBitArray::New();

    case VTK_CHAR:
      return vtkCharArray::New();

    case VTK_SIGNED_CHAR:
      return vtkSignedCharArray::New();

    case VTK_UNSIGNED_CHAR:
      return vtkUnsignedCharArray::New();

    case VTK_SHORT:
      return vtkShortArray::New();

    case VTK_UNSIGNED_SHORT:
      return vtkUnsignedShortArray::New();

    case VTK_INT:
      return vtkIntArray::New();

    case VTK_UNSIGNED_INT:
      return vtkUnsignedIntArray::New();

    case VTK_LONG:
      return vtkLongArray::New();

    case VTK_UNSIGNED_LONG:
      return vtkUnsignedLongArray::New();

    case VTK_LONG_LONG:
      return vtkLongLongArray::New();

    case VTK_UNSIGNED_LONG_LONG:
      return vtkUnsignedLongLongArray::New();

    case VTK_FLOAT:
      return vtkFloatArray::New();

    case VTK_DOUBLE:
      return vtkDoubleArray::New();

    case VTK_ID_TYPE:
      return vtkIdTypeArray::New();

    case VTK_STRING:
      return vtkStringArray::New();

    case VTK_VARIANT:
      return vtkVariantArray::New();

    default:
      break;
  }

  vtkGenericWarningMacro("Unsupported data type: " << dataType << "! Setting to VTK_DOUBLE");
  return vtkDoubleArray::New();
}

bool vtkAbstractArray::IsIntegral() const
{
  if (!this->IsNumeric())
  {
    return false;
  }
  int dtype = this->GetDataType();
  switch (dtype)
  {
    case VTK_VOID:
      return false;
    case VTK_BIT:
    case VTK_CHAR:
    case VTK_SIGNED_CHAR:
    case VTK_UNSIGNED_CHAR:
    case VTK_SHORT:
    case VTK_UNSIGNED_SHORT:
    case VTK_INT:
    case VTK_UNSIGNED_INT:
    case VTK_LONG:
    case VTK_UNSIGNED_LONG:
    case VTK_ID_TYPE:
    case VTK_LONG_LONG:
    case VTK_UNSIGNED_LONG_LONG:
      return true;
    default:
    case VTK_FLOAT:
    case VTK_DOUBLE:
    case VTK_STRING:
    case VTK_OPAQUE:
      return false;
  }
}

//------------------------------------------------------------------------------
template <typename T>
vtkVariant vtkAbstractArrayGetVariantValue(T* arr, vtkIdType index)
{
  return vtkVariant(arr[index]);
}

//------------------------------------------------------------------------------
vtkVariant vtkAbstractArray::GetVariantValue(vtkIdType valueIdx)
{
  vtkVariant val;
  switch (this->GetDataType())
  {
    vtkExtraExtendedTemplateMacro(val = vtkAbstractArrayGetVariantValue(
                                    static_cast<VTK_TT*>(this->GetVoidPointer(0)), valueIdx));
    vtkTemplateMacroCase(
      VTK_BIT, int, val = static_cast<VTK_TT>(static_cast<vtkBitArray*>(this)->GetValue(valueIdx)));
  }
  return val;
}

//------------------------------------------------------------------------------
void vtkAbstractArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  const char* name = this->GetName();
  if (name)
  {
    os << indent << "Name: " << name << "\n";
  }
  else
  {
    os << indent << "Name: (none)\n";
  }
  os << indent << "Data type: " << this->GetDataTypeAsString() << "\n";
  os << indent << "Size: " << this->Size << "\n";
  os << indent << "MaxId: " << this->MaxId << "\n";
  os << indent << "NumberOfComponents: " << this->NumberOfComponents << endl;
  if (this->ComponentNames)
  {
    os << indent << "ComponentNames: " << endl;
    vtkIndent nextIndent = indent.GetNextIndent();
    for (unsigned int i = 0; i < this->ComponentNames->size(); ++i)
    {
      os << nextIndent << i << " : " << this->ComponentNames->at(i) << endl;
    }
  }
  os << indent << "Information: " << this->Information << endl;
  if (this->Information)
  {
    this->Information->PrintSelf(os, indent.GetNextIndent());
  }
}

//------------------------------------------------------------------------------
const char* vtkAbstractArray::GetArrayTypeAsString() const
{
  switch (this->GetArrayType())
  {
    case AbstractArray:
      return "AbstractArray";
    case DataArray:
      return "DataArray";
    case AoSDataArrayTemplate:
      return "AoSDataArrayTemplate";
    case SoADataArrayTemplate:
      return "SoADataArrayTemplate";
    case TypedDataArray:
      return "TypedDataArray";
    case MappedDataArray:
      return "MappedDataArray";
    case ScaleSoADataArrayTemplate:
      return "ScaleSoADataArrayTemplate";
    case ImplicitArray:
      return "ImplicitArray";
  }
  return "Unknown";
}

//------------------------------------------------------------------------------
void vtkAbstractArray::GetProminentComponentValues(
  int comp, vtkVariantArray* values, double uncertainty, double minimumProminence)
{
  if (!values || comp < -1 || comp >= this->NumberOfComponents)
  {
    return;
  }

  values->Initialize();
  values->SetNumberOfComponents(comp < 0 ? this->NumberOfComponents : 1);

  bool justCreated = false;
  vtkInformation* info = this->GetInformation();
  const double* lastParams = info
    ? (info->Has(DISCRETE_VALUE_SAMPLE_PARAMETERS()) ? info->Get(DISCRETE_VALUE_SAMPLE_PARAMETERS())
                                                     : nullptr)
    : nullptr;
  if (comp >= 0 && info)
  {
    vtkInformationVector* infoVec = info->Get(PER_COMPONENT());
    if (!infoVec || infoVec->GetNumberOfInformationObjects() < this->NumberOfComponents)
    {
      infoVec = vtkInformationVector::New();
      infoVec->SetNumberOfInformationObjects(this->NumberOfComponents);
      info->Set(PER_COMPONENT(), infoVec);
      infoVec->FastDelete();
      justCreated = true;
    }
    info = infoVec->GetInformationObject(comp);
  }
  if (info)
  {
    // Any insane parameter values map to
    // deterministic, exhaustive enumeration of all
    // distinct values:
    if (uncertainty < 0. || uncertainty > 1.)
    {
      uncertainty = 0.;
    }
    if (minimumProminence < 0. || minimumProminence > 1.)
    {
      minimumProminence = 0.;
    }
    // Are parameter values requesting more certainty in reporting or
    // that less-prominent values be reported? If so, recompute.
    bool tighterParams =
      lastParams ? (lastParams[0] > uncertainty || lastParams[1] > minimumProminence) : true;
    // Recompute discrete value set when the array has been
    // modified since the information was written.
    if (!info->Has(DISCRETE_VALUES()) || tighterParams || this->GetMTime() > info->GetMTime() ||
      justCreated)
    {
      this->UpdateDiscreteValueSet(uncertainty, minimumProminence);
    }
  }
  else
  {
    return;
  }

  vtkIdType len;
  const vtkVariant* vals = info->Get(DISCRETE_VALUES());
  if (vals != nullptr)
  {
    len = info->Length(DISCRETE_VALUES());
    values->SetNumberOfTuples(len / values->GetNumberOfComponents());
    for (vtkIdType i = 0; i < len; ++i)
    {
      values->SetVariantValue(i, vals[i]);
    }
  }
}
VTK_ABI_NAMESPACE_END

//------------------------------------------------------------------------------
namespace
{
template <typename T, bool U = std::numeric_limits<T>::has_quiet_NaN>
struct CompareWithNaN
{
  bool operator()(T a, T b) const
  {
    if (std::isnan(a))
    {
      return false;
    }
    if (std::isnan(b))
    {
      return true;
    }
    return a < b;
  }
};

template <typename T>
struct CompareWithNaN<T, false>
{
  bool operator()(T a, T b) const { return a < b; }
};

template <typename T>
bool AccumulateSampleValues(T* array, int nc, vtkIdType begin, vtkIdType end,
  std::vector<std::set<T, CompareWithNaN<T>>>& uniques, std::set<std::vector<T>>& tupleUniques,
  unsigned int maxDiscreteValues)
{
  // number of discrete components remaining (tracked during iteration):
  int ndc = nc;
  std::pair<typename std::set<T>::iterator, bool> result;
  std::pair<typename std::set<std::vector<T>>::iterator, bool> tresult;
  std::vector<T> tuple;
  tuple.resize(nc);
  // Here we iterate over the components and add to their respective lists
  // of previously encountered values -- as long as there are not too many
  // values already in the list. We also accumulate each component's value
  // into a vtkVariantArray named tuple, which is added to the list of
  // unique vectors -- again assuming it is not already too long.
  for (vtkIdType i = begin; i < end && ndc; ++i)
  {
    // First, attempt a per-component insert.
    for (int j = 0; j < nc; ++j)
    {
      if (uniques[j].size() > maxDiscreteValues)
        continue;
      T& val(array[i * nc + j]);
      tuple[j] = val;
      result = uniques[j].insert(val);
      if (result.second)
      {
        if (uniques[j].size() == maxDiscreteValues + 1)
        {
          --ndc;
        }
      }
    }
    // Now, as long as no component has exceeded maxDiscreteValues unique
    // values, it is worth seeing whether the tuple as a whole is unique:
    if (nc > 1 && ndc == nc)
    {
      tresult = tupleUniques.insert(tuple);
      (void)tresult; // nice to have when debugging.
    }
  }
  return ndc == 0;
}

//------------------------------------------------------------------------------
template <typename U>
void SampleProminentValues(std::vector<std::vector<vtkVariant>>& uniques, vtkIdType maxId, int nc,
  vtkIdType nt, int blockSize, vtkIdType numberOfBlocks, U* ptr, unsigned int maxDiscreteValues)
{
  std::vector<std::set<U, CompareWithNaN<U>>> typeSpecificUniques;
  std::set<std::vector<U>> typeSpecificUniqueTuples;
  typeSpecificUniques.resize(nc);
  // I. Accumulate samples for all components plus the tuple,
  //    either for the full array or a random subset.
  if (numberOfBlocks * blockSize > maxId / 2)
  { // Awwww, just do the whole array already!
    AccumulateSampleValues(
      ptr, nc, 0, nt, typeSpecificUniques, typeSpecificUniqueTuples, maxDiscreteValues);
  }
  else
  { // Choose random blocks
    vtkNew<vtkMinimalStandardRandomSequence> seq;
    // test different blocks each time we're called:
    seq->SetSeed(static_cast<int>(seq->GetMTime()) ^ 0xdeadbeef);
    vtkIdType totalBlockCount = nt / blockSize + (nt % blockSize ? 1 : 0);
    std::set<vtkIdType> startTuples;
    // Sort the list of blocks we'll search to maintain cache coherence.
    for (int i = 0; i < numberOfBlocks; ++i, seq->Next())
    {
      vtkIdType startTuple = static_cast<vtkIdType>(seq->GetValue() * totalBlockCount) * blockSize;
      startTuples.insert(startTuple);
    }
    // Now iterate over the blocks, accumulating unique values and tuples.
    std::set<vtkIdType>::iterator blkIt;
    for (blkIt = startTuples.begin(); blkIt != startTuples.end(); ++blkIt)
    {
      vtkIdType startTuple = *blkIt;
      vtkIdType endTuple = startTuple + blockSize;
      endTuple = endTuple < nt ? endTuple : nt;
      bool endEarly = AccumulateSampleValues(ptr, nc, startTuple, endTuple, typeSpecificUniques,
        typeSpecificUniqueTuples, maxDiscreteValues);
      if (endEarly)
        break;
    }
  }

  // II. Convert type-specific sets of unique values into non-type-specific
  //     vectors of vtkVariants for storage in array information.

  // Handle per-component uniques first
  for (int i = 0; i < nc; ++i)
  {
    std::back_insert_iterator<std::vector<vtkVariant>> bi(uniques[i]);
    std::copy(typeSpecificUniques[i].begin(), typeSpecificUniques[i].end(), bi);
  }

  // Now squash any tuple-wide uniques into
  // the final entry of the outer vector.
  typename std::set<std::vector<U>>::iterator si;
  for (si = typeSpecificUniqueTuples.begin(); si != typeSpecificUniqueTuples.end(); ++si)
  {
    std::back_insert_iterator<std::vector<vtkVariant>> bi(uniques[nc]);
    std::copy(si->begin(), si->end(), bi);
  }
}
} // End anonymous namespace.

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
void vtkAbstractArray::UpdateDiscreteValueSet(double uncertainty, double minimumProminence)
{
  // For an array with T tuples and given uncertainty U and mininumum
  // prominence P, we sample N blocks of M tuples each, with
  // M*N = f(T; P, U) and f some sublinear function of T.
  // If every component plus all components taken together each have more than
  // MaxDiscreteValues distinct values, then we exit early.
  // M is chosen based on the number of bytes per tuple to maximize use of a
  // cache line (assuming a 64-byte cache line until kwsys::SystemInformation
  // or the like can provide a platform-independent way to query it).
  //
  // N is chosen to satisfy the requested uncertainty and prominence criteria
  // specified.
  constexpr int cacheLineSize = 64;
  constexpr int sampleFactor = 5;

  // I. Determine the granularity at which the array should be sampled.
  int nc = this->NumberOfComponents;
  int blockSize = cacheLineSize / (this->GetDataTypeSize() * nc);
  if (!blockSize)
  {
    blockSize = 4;
  }
  double logfac = 1.;
  vtkIdType nt = this->GetNumberOfTuples();
  vtkIdType numberOfSampleTuples = nt;
  if (this->MaxId > 0 && minimumProminence > 0.0)
  {
    logfac = -log(uncertainty * minimumProminence) / minimumProminence;
    if (logfac < 0)
    {
      logfac = -logfac;
    }
    if (!vtkMath::IsInf(logfac))
    {
      numberOfSampleTuples = static_cast<vtkIdType>(sampleFactor * logfac);
    }
  }
  /*
  // Theoretically, we should discard values or tuples that recur fewer
  // than minFreq times in our sample, but in practice this involves
  // counting and communication that slow us down.
  vtkIdType minFreq = static_cast<vtkIdType>(
    numberOfSampleTuples * minimumProminence / 2);
    */
  vtkIdType numberOfBlocks =
    numberOfSampleTuples / blockSize + (numberOfSampleTuples % blockSize ? 1 : 0);
  if (static_cast<unsigned int>(numberOfBlocks * blockSize) < 2 * this->MaxDiscreteValues)
  {
    numberOfBlocks =
      2 * this->MaxDiscreteValues / blockSize + (2 * this->MaxDiscreteValues % blockSize ? 1 : 0);
  }
  // II. Sample the array.
  std::vector<std::vector<vtkVariant>> uniques(nc > 1 ? nc + 1 : nc);
  switch (this->GetDataType())
  {
    vtkExtraExtendedTemplateMacro(SampleProminentValues(uniques, this->MaxId, nc, nt, blockSize,
      numberOfBlocks, static_cast<VTK_TT*>(this->GetVoidPointer(0)), this->MaxDiscreteValues));
    default:
      vtkErrorMacro("Array type " << this->GetClassName() << " not supported.");
      break;
  }

  // III. Store the results in the array's vtkInformation.
  int c;
  vtkInformationVector* iv;
  for (c = 0; c < nc; ++c)
  {
    if (!uniques[c].empty() && uniques[c].size() <= this->MaxDiscreteValues)
    {
      iv = this->GetInformation()->Get(PER_COMPONENT());
      if (!iv)
      {
        vtkNew<vtkInformationVector> infoVec;
        infoVec->SetNumberOfInformationObjects(this->NumberOfComponents);
        this->GetInformation()->Set(PER_COMPONENT(), infoVec);
        iv = this->GetInformation()->Get(PER_COMPONENT());
      }
      iv->GetInformationObject(c)->Set(
        DISCRETE_VALUES(), uniques[c].data(), static_cast<int>(uniques[c].size()));
    }
    else
    {
      iv = this->GetInformation()->Get(PER_COMPONENT());
      if (iv)
      {
        iv->GetInformationObject(c)->Remove(DISCRETE_VALUES());
      }
    }
  }
  if (nc > 1 && uniques[nc].size() <= this->MaxDiscreteValues * nc)
  {
    this->GetInformation()->Set(
      DISCRETE_VALUES(), uniques[nc].data(), static_cast<int>(uniques[nc].size()));
  }
  else
  { // Remove the key
    this->GetInformation()->Remove(DISCRETE_VALUES());
  }

  // Always store the sample parameters; this lets us know not to
  // re-run the sampling algorithm.
  double params[2];
  params[0] = uncertainty;
  params[1] = minimumProminence;
  this->GetInformation()->Set(DISCRETE_VALUE_SAMPLE_PARAMETERS(), params, 2);
}

//------------------------------------------------------------------------------
void vtkAbstractArray::PrintValues(ostream& os)
{
  if (auto* dataArray = vtkDataArray::SafeDownCast(this))
  {
    using Dispatcher = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::AllTypes>;
    ::PrintDataArrayWorker worker;
    if (!Dispatcher::Execute(dataArray, worker, os))
    {
      worker(dataArray, os);
    }
  }
  else
  {
    for (vtkIdType ii = 0; ii < this->GetNumberOfValues(); ++ii)
    {
      os << this->GetVariantValue(ii).ToString() << ' ';
    }
  }
}
VTK_ABI_NAMESPACE_END
