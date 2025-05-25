// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkStandaloneSession.h"
#include "vtkLogger.h"
#include "vtkWebAssemblySessionHelper.h"

#include <cstdint>

VTK_ABI_NAMESPACE_BEGIN

//-------------------------------------------------------------------------------
vtkStandaloneSession::vtkStandaloneSession()
{
  this->Session = NewVTKInterfaceForJavaScript();
  vtkSessionInitializeObjectManager(this->Session);
}

//-------------------------------------------------------------------------------
vtkStandaloneSession::~vtkStandaloneSession()
{
  vtkFreeSession(this->Session);
}

//-------------------------------------------------------------------------------
vtkObjectHandle vtkStandaloneSession::Create(const std::string& className)
{
  return vtkSessionCreateObject(this->Session, className.c_str());
}

//-------------------------------------------------------------------------------
void vtkStandaloneSession::Destroy(vtkObjectHandle object)
{
  vtkSessionDestroyObject(this->Session, object);
}

//-------------------------------------------------------------------------------
void vtkStandaloneSession::Set(vtkObjectHandle object, emscripten::val properties)
{
  // Ensure the ID is set in the JSON state before updating the object
  properties.set("Id", object);
  vtkSessionJsonImpl propertiesImpl{ properties };
  return vtkSessionUpdateObjectFromState(this->Session, &propertiesImpl);
}

//-------------------------------------------------------------------------------
emscripten::val vtkStandaloneSession::Get(vtkObjectHandle object)
{

  vtkSessionUpdateStateFromObject(this->Session, object);
  auto propertiesImpl = vtkSessionGetState(this->Session, object);
  auto result = std::move(propertiesImpl->JsonValue);
  delete propertiesImpl;
  return result;
}

//-------------------------------------------------------------------------------
emscripten::val vtkStandaloneSession::Invoke(
  vtkObjectHandle object, const std::string& methodName, emscripten::val args)
{
  if (!args.instanceof (Array))
  {
    vtkLog(
      ERROR, << "Invoke must be called with an objectId: u32, methodName: string, args: Array");
    return emscripten::val::undefined();
  }
  if (auto* manager = static_cast<vtkObjectManager*>(vtkSessionGetManager(this->Session)))
  {
    if (auto* dataArray = vtkDataArray::SafeDownCast(manager->GetObjectAtId(object)))
    {
      if (methodName == "SetArray")
      {
        if (args["length"].as<std::size_t>() == 1)
        {
          auto jsArray = args[0];
          for (const auto& it : IsJSArraySameTypeAsVtkDataArray)
          {
            const auto& typeName = it.first;
            if (jsArray.instanceof (val::global(typeName.c_str())) && it.second(dataArray))
            {
              const auto length = jsArray["length"].as<std::size_t>();
              dataArray->SetNumberOfValues(length);
              // Copy the data from the JS array to the VTK data array
              using DispatchT = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::AllTypes>;
              if (!DispatchT::Execute(dataArray, CopyJSArrayToVTKDataArray{}, jsArray))
              {
                // Fallback to the default implementation if the DispatchT fails
                CopyJSArrayToVTKDataArray{}(dataArray, jsArray);
              }
              return emscripten::val::undefined();
            }
          }
          vtkLog(ERROR,
            "Unsupported argument constructed by "
              << jsArray["constructor"].call<std::string>("toString") << " for "
              << dataArray->GetClassName() << "::SetArray"
              << " method.");
          return emscripten::val::undefined();
        }
        else
        {
          vtkLog(ERROR, << "vtkDataArray::SetArray expects a list of a single TypedArray");
          return emscripten::val::undefined();
        }
      }
    }
  }
  else
  {
    vtkLog(ERROR, "Invalid session: " << this->Session);
    return emscripten::val::undefined();
  }
  vtkSessionJsonImpl argsJsonImpl{ args };
  auto resultImpl = vtkSessionInvoke(this->Session, object, methodName.c_str(), &argsJsonImpl);
  auto result = std::move(resultImpl->JsonValue);
  delete resultImpl;
  return result;
}

//-------------------------------------------------------------------------------
unsigned long vtkStandaloneSession::Observe(
  vtkObjectHandle object, const std::string& eventName, emscripten::val jsFunction)
{
  int fp = val::module_property("addFunction")(jsFunction, std::string("vii")).as<int>();
  auto callback = reinterpret_cast<vtkSessionObserverCallbackFunc>(fp);
  return vtkSessionAddObserver(this->Session, object, eventName.c_str(), callback);
}

//-------------------------------------------------------------------------------
bool vtkStandaloneSession::UnObserve(vtkObjectHandle object, unsigned long tag)
{
  return vtkSessionRemoveObserver(this->Session, object, tag) == vtkSessionResultSuccess;
}

VTK_ABI_NAMESPACE_END
