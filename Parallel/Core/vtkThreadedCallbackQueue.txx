/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThreadedCallbackQueue.txx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <functional>
#include <tuple>
#include <type_traits>

VTK_ABI_NAMESPACE_BEGIN

//=============================================================================
struct vtkThreadedCallbackQueue::InvokerImpl
{
  /**
   * Helper to extract the parameter types of a function (passed by template)
   */
  template <class FT>
  struct Signature;

  /**
   * Substitute for std::integer_sequence which is C++14
   */
  template <std::size_t... Is>
  struct IntegerSequence;
  template <std::size_t N, std::size_t... Is>
  struct MakeIntegerSequence;

  /**
   * This function is used to discriminate whether you can dereference the template parameter T.
   * It uses SNFINAE and jumps to the std::false_type version if it fails dereferencing T.
   */
  template <class T>
  static decltype(*std::declval<T&>(), std::true_type{}) CanBeDereferenced(std::nullptr_t);
  template <class>
  static std::false_type CanBeDereferenced(...);

  /**
   * Helper that provides a static method Get that converts an input variable to a non-pointer type
   * if possible.
   *
   * A few examples:
   * Dereference<std::unique_ptr<int>>::Get returns an int.
   * Dereference<int>::Get returns an int.
   */
  template <class T, class CanBeDereferencedT>
  struct Dereference;

  /**
   * Convenient typedef that use Signature to convert the parameters of function of type FT
   * to a std::tuple.
   */
  template <class FT>
  using ArgsTuple = typename Signature<typename std::remove_reference<FT>::type>::ArgsTuple;

  /**
   * Convenient typedef that, given a function of type FT and an index I, returns the type of the
   * Ith parameter of the function.
   */
  template <class FT, std::size_t I>
  using ArgType = typename std::tuple_element<I, ArgsTuple<FT>>::type;

  /**
   * This holds the attributes of a function.
   * There are 2 implementations: one for member function pointers, and one for all the others
   * (functors, lambdas, function pointers)
   */
  template <bool IsMemberFunctionPointer, class... ArgsT>
  class InvokerHandle;
};

//=============================================================================
// For lamdas or std::function
template <class ReturnT, class... ArgsT>
struct vtkThreadedCallbackQueue::InvokerImpl::Signature<ReturnT(ArgsT...)>
{
  using ArgsTuple = std::tuple<ArgsT...>;
};

//=============================================================================
// For methods inside a class ClassT
template <class ClassT, class ReturnT, class... ArgsT>
struct vtkThreadedCallbackQueue::InvokerImpl::Signature<ReturnT (ClassT::*)(ArgsT...)>
{
  using ArgsTuple = std::tuple<ArgsT...>;
};

//=============================================================================
// For const methods inside a class ClassT
template <class ClassT, class ReturnT, class... ArgsT>
struct vtkThreadedCallbackQueue::InvokerImpl::Signature<ReturnT (ClassT::*)(ArgsT...) const>
{
  using ArgsTuple = std::tuple<ArgsT...>;
};

//=============================================================================
// For function pointers
template <class ReturnT, class... ArgsT>
struct vtkThreadedCallbackQueue::InvokerImpl::Signature<ReturnT (*)(ArgsT...)>
{
  using ArgsTuple = std::tuple<ArgsT...>;
};

//=============================================================================
// For functors
template <class FT>
struct vtkThreadedCallbackQueue::InvokerImpl::Signature
  : vtkThreadedCallbackQueue::InvokerImpl::Signature<decltype(&FT::operator())>
{
};

//=============================================================================
template <std::size_t... Is>
struct vtkThreadedCallbackQueue::InvokerImpl::IntegerSequence
{
};

//=============================================================================
template <std::size_t N, std::size_t... Is>
struct vtkThreadedCallbackQueue::InvokerImpl::MakeIntegerSequence
  : vtkThreadedCallbackQueue::InvokerImpl::MakeIntegerSequence<N - 1, N - 1, Is...>
{
};

//=============================================================================
template <std::size_t... Is>
struct vtkThreadedCallbackQueue::InvokerImpl::MakeIntegerSequence<0, Is...>
  : vtkThreadedCallbackQueue::InvokerImpl::IntegerSequence<Is...>
{
};

//=============================================================================
template <class T>
struct vtkThreadedCallbackQueue::InvokerImpl::Dereference<T,
  std::true_type /* CanBeDereferencedT */>
{
  using Type = decltype(*std::declval<T>());
  static Type& Get(T& instance) { return *instance; }
};

//=============================================================================
template <class T>
struct vtkThreadedCallbackQueue::InvokerImpl::Dereference<T,
  std::false_type /* CanBeDereferencedT */>
{
  using Type = T;
  static Type& Get(T& instance) { return instance; }
};

//=============================================================================
template <class FT, class ObjectT, class... ArgsT>
class vtkThreadedCallbackQueue::InvokerImpl::InvokerHandle<true /* IsMemberFunctionPointer */, FT,
  ObjectT, ArgsT...>
{
public:
  template <class FTT, class ObjectTT, class... ArgsTT>
  InvokerHandle(FTT&& f, ObjectTT&& instance, ArgsTT&&... args)
    : Function(std::forward<FTT>(f))
    , Instance(std::forward<ObjectTT>(instance))
    , Args(std::forward<ArgsTT>(args)...)
  {
  }

  void operator()() { this->Invoke(MakeIntegerSequence<sizeof...(ArgsT)>()); }

private:
  template <std::size_t... Is>
  void Invoke(IntegerSequence<Is...>)
  {
    // If the input object is wrapped inside a pointer (could be shared_ptr, vtkSmartPointer),
    // we need to dereference the object before invoking it.
    using DerefImpl = decltype(CanBeDereferenced<ObjectT>(nullptr));
    using Deref = Dereference<ObjectT, DerefImpl>;
    auto& deref = Deref::Get(this->Instance);

    // The static_cast to ArgType forces casts to the correct types of the function.
    // There are conflicts with rvalue references not being able to be converted to lvalue
    // references if this static_cast is not performed
    (deref.*Function)(static_cast<ArgType<decltype(deref), Is>>(std::get<Is>(this->Args))...);
  }

  FT Function;
  // We DO NOT want to hold lvalue references! They could be destroyed before we execute them.
  // This forces to call the copy constructor on lvalue references inputs.
  typename std::remove_reference<ObjectT>::type Instance;
  std::tuple<typename std::remove_reference<ArgsT>::type...> Args;
};

//=============================================================================
template <class FT, class... ArgsT>
class vtkThreadedCallbackQueue::InvokerImpl::InvokerHandle<false /* IsMemberFunctionPointer */, FT,
  ArgsT...>
{
public:
  template <class FTT, class... ArgsTT>
  InvokerHandle(FTT&& f, ArgsTT&&... args)
    : Function(std::forward<FTT>(f))
    , Args(std::forward<ArgsTT>(args)...)
  {
  }

  void operator()() { this->Invoke(MakeIntegerSequence<sizeof...(ArgsT)>()); }

private:
  template <std::size_t... Is>
  void Invoke(IntegerSequence<Is...>)
  {
    // If the input is a functor and is wrapped inside a pointer (could be shared_ptr),
    // we need to dereference the functor before invoking it.
    using DerefImpl = decltype(CanBeDereferenced<FT>(nullptr));
    using Deref = Dereference<FT, DerefImpl>;
    auto& f = Deref::Get(this->Function);

    // The static_cast to ArgType forces casts to the correct types of the function.
    // There are conflicts with rvalue references not being able to be converted to lvalue
    // references if this static_cast is not performed
    f(static_cast<ArgType<decltype(f), Is>>(std::get<Is>(this->Args))...);
  }

  // We DO NOT want to hold lvalue references! They could be destroyed before we execute them.
  // This forces to call the copy constructor on lvalue references inputs.
  // DereferencedFunction Function;
  typename std::remove_reference<FT>::type Function;
  std::tuple<typename std::remove_reference<ArgsT>::type...> Args;
};

//=============================================================================
struct vtkThreadedCallbackQueue::InvokerBase
{
  virtual ~InvokerBase() = default;
  virtual void operator()() = 0;
};

//=============================================================================
template <class FT, class... ArgsT>
class vtkThreadedCallbackQueue::Invoker : public vtkThreadedCallbackQueue::InvokerBase
{
public:
  template <class... ArgsTT>
  Invoker(ArgsTT&&... args)
    : Impl(std::forward<ArgsTT>(args)...)
  {
  }

  void operator()() override { this->Impl(); }

private:
  InvokerImpl::InvokerHandle<std::is_member_function_pointer<FT>::value, FT, ArgsT...> Impl;
};

//-----------------------------------------------------------------------------
template <class... ArgsT>
void vtkThreadedCallbackQueue::Push(ArgsT&&... args)
{
  {
    std::lock_guard<std::mutex> lock(this->Mutex);

    // We remove referenceness so the tuple holding the arguments is valid
    this->InvokerQueue.emplace(InvokerPointer(new Invoker<ArgsT...>(std::forward<ArgsT>(args)...)));
    this->Empty = false;
  }

  this->ConditionVariable.notify_one();
}

VTK_ABI_NAMESPACE_END
