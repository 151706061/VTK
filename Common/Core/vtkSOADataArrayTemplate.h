// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSOADataArrayTemplate
 * @brief   Struct-Of-Arrays implementation of vtkGenericDataArray.
 *
 *
 * vtkSOADataArrayTemplate is the counterpart of vtkAOSDataArrayTemplate. Because
 * of current needed support for GetVoidPointer() the underlying data might actually
 * be stored in SOA or AOS memory layout. For SOA layout each component is stored in
 * a separate array. For AOS layout the data is stored in the standard legacy way.
 * The default storage layout is AOS due to needing to conform to VTK's standard
 * layout for use with GetVoidPointer().
 *
 * @sa
 * vtkGenericDataArray vtkAOSDataArrayTemplate
 */

#ifndef vtkSOADataArrayTemplate_h
#define vtkSOADataArrayTemplate_h

#include "vtkBuffer.h"
#include "vtkCommonCoreModule.h" // For export macro
#include "vtkCompiler.h"         // for VTK_USE_EXTERN_TEMPLATE
#include "vtkGenericDataArray.h"

// The export macro below makes no sense, but is necessary for older compilers
// when we export instantiations of this class from vtkCommonCore.
VTK_ABI_NAMESPACE_BEGIN
template <class ValueTypeT>
class VTKCOMMONCORE_EXPORT vtkSOADataArrayTemplate
  : public vtkGenericDataArray<vtkSOADataArrayTemplate<ValueTypeT>, ValueTypeT>
{
  typedef vtkGenericDataArray<vtkSOADataArrayTemplate<ValueTypeT>, ValueTypeT> GenericDataArrayType;

public:
  typedef vtkSOADataArrayTemplate<ValueTypeT> SelfType;
  vtkTemplateTypeMacro(SelfType, GenericDataArrayType);
  typedef typename Superclass::ValueType ValueType;

  enum DeleteMethod
  {
    VTK_DATA_ARRAY_FREE = vtkAbstractArray::VTK_DATA_ARRAY_FREE,
    VTK_DATA_ARRAY_DELETE = vtkAbstractArray::VTK_DATA_ARRAY_DELETE,
    VTK_DATA_ARRAY_ALIGNED_FREE = vtkAbstractArray::VTK_DATA_ARRAY_ALIGNED_FREE,
    VTK_DATA_ARRAY_USER_DEFINED = vtkAbstractArray::VTK_DATA_ARRAY_USER_DEFINED
  };

  static vtkSOADataArrayTemplate* New();

  ///@{
  /**
   * Get the value at @a valueIdx. @a valueIdx assumes AOS ordering.
   */
  ValueType GetValue(vtkIdType valueIdx) const
  {
    vtkIdType tupleIdx;
    int comp;
    this->GetTupleIndexFromValueIndex(valueIdx, tupleIdx, comp);
    return this->GetTypedComponent(tupleIdx, comp);
  }
  ///@}

  ///@{
  /**
   * Set the value at @a valueIdx to @a value. @a valueIdx assumes AOS ordering.
   */
  void SetValue(vtkIdType valueIdx, ValueType value)
  {
    vtkIdType tupleIdx;
    int comp;
    this->GetTupleIndexFromValueIndex(valueIdx, tupleIdx, comp);
    this->SetTypedComponent(tupleIdx, comp, value);
  }
  ///@}

  /**
   * Copy the tuple at @a tupleIdx into @a tuple.
   */
  void GetTypedTuple(vtkIdType tupleIdx, ValueType* tuple) const
  {
    if (this->StorageType == StorageTypeEnum::SOA)
    {
      for (size_t cc = 0; cc < this->Data.size(); cc++)
      {
        tuple[cc] = this->Data[cc]->GetBuffer()[tupleIdx];
      }
    }
    else
    {
      ValueType* buffer = this->AoSData->GetBuffer();
      std::copy(buffer + tupleIdx * this->GetNumberOfComponents(),
        buffer + (tupleIdx + 1) * this->GetNumberOfComponents(), tuple);
    }
  }

  /**
   * Set this array's tuple at @a tupleIdx to the values in @a tuple.
   */
  void SetTypedTuple(vtkIdType tupleIdx, const ValueType* tuple)
  {
    if (this->StorageType == StorageTypeEnum::SOA)
    {
      for (size_t cc = 0; cc < this->Data.size(); ++cc)
      {
        this->Data[cc]->GetBuffer()[tupleIdx] = tuple[cc];
      }
    }
    else
    {
      ValueType* buffer = this->AoSData->GetBuffer();
      std::copy(tuple, tuple + this->GetNumberOfComponents(),
        buffer + tupleIdx * this->GetNumberOfComponents());
    }
  }

  /**
   * Get component @a comp of the tuple at @a tupleIdx.
   */
  ValueType GetTypedComponent(vtkIdType tupleIdx, int comp) const
  {
    if (this->StorageType == StorageTypeEnum::SOA)
    {
      return this->Data[comp]->GetBuffer()[tupleIdx];
    }
    return this->AoSData->GetBuffer()[tupleIdx * this->GetNumberOfComponents() + comp];
  }

  /**
   * Set component @a comp of the tuple at @a tupleIdx to @a value.
   */
  void SetTypedComponent(vtkIdType tupleIdx, int comp, ValueType value)
  {
    if (this->StorageType == StorageTypeEnum::SOA)
    {
      this->Data[comp]->GetBuffer()[tupleIdx] = value;
    }
    else
    {
      this->AoSData->GetBuffer()[tupleIdx * this->GetNumberOfComponents() + comp] = value;
    }
  }

  /**
   * Set component @a comp of all tuples to @a value.
   */
  void FillTypedComponent(int compIdx, ValueType value) override;

  /**
   * Use this API to pass externally allocated memory to this instance. Since
   * vtkSOADataArrayTemplate uses separate contiguous regions for each
   * component, use this API to add arrays for each of the component.
   * \c save: When set to true, vtkSOADataArrayTemplate will not release or
   * realloc the memory even when the AllocatorType is set to RESIZABLE. If
   * needed it will simply allow new memory buffers and "forget" the supplied
   * pointers. When save is set to false, this will be the \c deleteMethod
   * specified to release the array.
   * If updateMaxId is true, the array's MaxId will be updated, and assumes
   * that size is the number of tuples in the array.
   * \c size is specified in number of elements of ScalarType.
   */
  void SetArray(int comp, VTK_ZEROCOPY ValueType* array, vtkIdType size, bool updateMaxId = false,
    bool save = false, int deleteMethod = VTK_DATA_ARRAY_FREE);

  /**
   * This method allows the user to specify a custom free function to be
   * called when the array is deallocated. Calling this method will implicitly
   * mean that the given free function will be called when the class
   * cleans up or reallocates memory. This custom free function will be
   * used for all components.
   **/
  void SetArrayFreeFunction(void (*callback)(void*)) override;

  /**
   * This method allows the user to specify a custom free function to be
   * called when the array is deallocated. Calling this method will implicitly
   * mean that the given free function will be called when the class
   * cleans up or reallocates memory.
   **/
  void SetArrayFreeFunction(int comp, void (*callback)(void*));

  /**
   * Return a pointer to a contiguous block of memory containing all values for
   * a particular components (ie. a single array of the struct-of-arrays).
   */
  ValueType* GetComponentArrayPointer(int comp);

  /**
   * Use of this method is discouraged, it creates a deep copy of the data into
   * a contiguous AoS-ordered buffer and prints a warning.
   */
  void* GetVoidPointer(vtkIdType valueIdx) override;

  /**
   * Export a copy of the data in AoS ordering to the preallocated memory
   * buffer.
   */
  void ExportToVoidPointer(void* ptr) override;

#ifndef __VTK_WRAP__
  ///@{
  /**
   * Perform a fast, safe cast from a vtkAbstractArray to a vtkDataArray.
   * This method checks if source->GetArrayType() returns DataArray
   * or a more derived type, and performs a static_cast to return
   * source as a vtkDataArray pointer. Otherwise, nullptr is returned.
   */
  static vtkSOADataArrayTemplate<ValueType>* FastDownCast(vtkAbstractArray* source);
  ///@}
#endif

  int GetArrayType() const override { return vtkAbstractArray::SoADataArrayTemplate; }
  VTK_NEWINSTANCE vtkArrayIterator* NewIterator() override;
  void SetNumberOfComponents(int numComps) override;
  void ShallowCopy(vtkDataArray* other) override;

  // Reimplemented for efficiency:
  void InsertTuples(
    vtkIdType dstStart, vtkIdType n, vtkIdType srcStart, vtkAbstractArray* source) override;
  // MSVC doesn't like 'using' here (error C2487). Just forward instead:
  // using Superclass::InsertTuples;
  void InsertTuples(vtkIdList* dstIds, vtkIdList* srcIds, vtkAbstractArray* source) override
  {
    this->Superclass::InsertTuples(dstIds, srcIds, source);
  }
  void InsertTuplesStartingAt(
    vtkIdType dstStart, vtkIdList* srcIds, vtkAbstractArray* source) override
  {
    this->Superclass::InsertTuplesStartingAt(dstStart, srcIds, source);
  }

#ifndef __VTK_WRAP__
  // helper method for vtkDataArray.cxx DeepCopyWorker () operator
  void CopyData(vtkSOADataArrayTemplate<ValueType>* src);
#endif

protected:
  vtkSOADataArrayTemplate();
  ~vtkSOADataArrayTemplate() override;

  /**
   * Allocate space for numTuples. Old data is not preserved. If numTuples == 0,
   * all data is freed.
   */
  bool AllocateTuples(vtkIdType numTuples);

  /**
   * Allocate space for numTuples. Old data is preserved. If numTuples == 0,
   * all data is freed.
   */
  bool ReallocateTuples(vtkIdType numTuples);

  std::vector<vtkBuffer<ValueType>*> Data;
  vtkBuffer<ValueType>* AoSData;

  /**
   * Because we still need to support GetVoidPointer() for both reading from and writing
   * to memory we may have the actual data stored in either this->Data or this->AoSData.
   * This enum lets us know where our actual data buffer is stored.
   */
  enum StorageTypeEnum
  {
    AOS,
    SOA
  };
  StorageTypeEnum StorageType;

  void ClearSOAData();

private:
  vtkSOADataArrayTemplate(const vtkSOADataArrayTemplate&) = delete;
  void operator=(const vtkSOADataArrayTemplate&) = delete;

  void GetTupleIndexFromValueIndex(vtkIdType valueIdx, vtkIdType& tupleIdx, int& comp) const
  {
    tupleIdx = valueIdx / this->NumberOfComponents;
    comp = valueIdx % this->NumberOfComponents;
  }

  friend class vtkGenericDataArray<vtkSOADataArrayTemplate<ValueTypeT>, ValueTypeT>;
};

// Declare vtkArrayDownCast implementations for SoA containers:
vtkArrayDownCast_TemplateFastCastMacro(vtkSOADataArrayTemplate);

VTK_ABI_NAMESPACE_END
#endif // header guard

// This portion must be OUTSIDE the include blockers. This is used to tell
// libraries other than vtkCommonCore that instantiations of
// vtkSOADataArrayTemplate can be found externally. This prevents each library
// from instantiating these on their own.
#ifdef VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATING
#define VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATE(T)                                                 \
  namespace vtkDataArrayPrivate                                                                    \
  {                                                                                                \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  VTK_INSTANTIATE_VALUERANGE_ARRAYTYPE(vtkSOADataArrayTemplate<T>, double);                        \
  VTK_ABI_NAMESPACE_END                                                                            \
  }                                                                                                \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  template class VTKCOMMONCORE_EXPORT vtkSOADataArrayTemplate<T>;                                  \
  VTK_ABI_NAMESPACE_END

#elif defined(VTK_USE_EXTERN_TEMPLATE)
#ifndef VTK_SOA_DATA_ARRAY_TEMPLATE_EXTERN
#define VTK_SOA_DATA_ARRAY_TEMPLATE_EXTERN
#ifdef _MSC_VER
#pragma warning(push)
// The following is needed when the vtkSOADataArrayTemplate is declared
// dllexport and is used from another class in vtkCommonCore
#pragma warning(disable : 4910) // extern and dllexport incompatible
#endif
VTK_ABI_NAMESPACE_BEGIN
vtkExternTemplateMacro(extern template class VTKCOMMONCORE_EXPORT vtkSOADataArrayTemplate);
VTK_ABI_NAMESPACE_END

namespace vtkDataArrayPrivate
{
VTK_ABI_NAMESPACE_BEGIN

// These are instantiated in vtkSOADataArrayTemplateInstantiate${i}.cxx
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkSOADataArrayTemplate<float>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkSOADataArrayTemplate<double>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkSOADataArrayTemplate<char>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkSOADataArrayTemplate<signed char>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkSOADataArrayTemplate<unsigned char>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkSOADataArrayTemplate<short>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkSOADataArrayTemplate<unsigned short>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkSOADataArrayTemplate<int>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkSOADataArrayTemplate<unsigned int>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkSOADataArrayTemplate<long>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkSOADataArrayTemplate<unsigned long>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkSOADataArrayTemplate<long long>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkSOADataArrayTemplate<unsigned long long>, double)

VTK_ABI_NAMESPACE_END
} // namespace vtkDataArrayPrivate

#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif // VTK_SOA_DATA_ARRAY_TEMPLATE_EXTERN

// The following clause is only for MSVC 2008 and 2010
#elif defined(_MSC_VER) && !defined(VTK_BUILD_SHARED_LIBS)
#pragma warning(push)

// C4091: 'extern ' : ignored on left of 'int' when no variable is declared
#pragma warning(disable : 4091)

// Compiler-specific extension warning.
#pragma warning(disable : 4231)

// We need to disable warning 4910 and do an extern dllexport
// anyway.  When deriving new arrays from an
// instantiation of this template the compiler does an explicit
// instantiation of the base class.  From outside the vtkCommon
// library we block this using an extern dllimport instantiation.
// For classes inside vtkCommon we should be able to just do an
// extern instantiation, but VS 2008 complains about missing
// definitions.  We cannot do an extern dllimport inside vtkCommon
// since the symbols are local to the dll.  An extern dllexport
// seems to be the only way to convince VS 2008 to do the right
// thing, so we just disable the warning.
#pragma warning(disable : 4910) // extern and dllexport incompatible

// Use an "extern explicit instantiation" to give the class a DLL
// interface.  This is a compiler-specific extension.
VTK_ABI_NAMESPACE_BEGIN
vtkInstantiateTemplateMacro(extern template class VTKCOMMONCORE_EXPORT vtkSOADataArrayTemplate);
VTK_ABI_NAMESPACE_END

#pragma warning(pop)

#endif

// VTK-HeaderTest-Exclude: vtkSOADataArrayTemplate.h
