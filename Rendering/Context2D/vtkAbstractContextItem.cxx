// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAbstractContextItem.h"
#include "vtkContext2D.h"
#include "vtkContext3D.h"
#include "vtkContextDevice2D.h"
#include "vtkContextDevice3D.h"
#include "vtkContextMouseEvent.h"
#include "vtkContextScenePrivate.h"
#include "vtkObjectFactory.h"

// STL headers
#include <algorithm>

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkAbstractContextItem::vtkAbstractContextItem()
{
  this->Scene = nullptr;
  this->Parent = nullptr;
  this->Children = new vtkContextScenePrivate(this);
  this->Visible = true;
  this->Interactive = true;
}

//------------------------------------------------------------------------------
vtkAbstractContextItem::~vtkAbstractContextItem()
{
  delete this->Children;
}

//------------------------------------------------------------------------------
bool vtkAbstractContextItem::Paint(vtkContext2D* painter)
{
  this->Children->PaintItems(painter);
  return true;
}

//------------------------------------------------------------------------------
bool vtkAbstractContextItem::PaintChildren(vtkContext2D* painter)
{
  this->Children->PaintItems(painter);
  return true;
}

//------------------------------------------------------------------------------
void vtkAbstractContextItem::Update() {}

//------------------------------------------------------------------------------
vtkIdType vtkAbstractContextItem::AddItem(vtkAbstractContextItem* item)
{
  return this->Children->AddItem(item);
}

//------------------------------------------------------------------------------
bool vtkAbstractContextItem::RemoveItem(vtkAbstractContextItem* item)
{
  return this->Children->RemoveItem(item);
}

//------------------------------------------------------------------------------
bool vtkAbstractContextItem::RemoveItem(vtkIdType index)
{
  if (index >= 0 && index < static_cast<vtkIdType>(this->Children->size()))
  {
    return this->Children->RemoveItem(index);
  }
  else
  {
    return false;
  }
}

//------------------------------------------------------------------------------
vtkAbstractContextItem* vtkAbstractContextItem::GetItem(vtkIdType index)
{
  if (index >= 0 && index < static_cast<vtkIdType>(this->Children->size()))
  {
    return this->Children->at(index);
  }
  else
  {
    return nullptr;
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkAbstractContextItem::GetItemIndex(vtkAbstractContextItem* item)
{
  vtkContextScenePrivate::const_iterator it =
    std::find(this->Children->begin(), this->Children->end(), item);
  if (it == this->Children->end())
  {
    return -1;
  }
  return it - this->Children->begin();
}

//------------------------------------------------------------------------------
vtkIdType vtkAbstractContextItem::GetNumberOfItems() VTK_FUTURE_CONST
{
  return static_cast<vtkIdType>(this->Children->size());
}

//------------------------------------------------------------------------------
void vtkAbstractContextItem::ClearItems()
{
  this->Children->Clear();
}

//------------------------------------------------------------------------------
vtkIdType vtkAbstractContextItem::Raise(vtkIdType index)
{
  return this->StackAbove(index, this->GetNumberOfItems() - 1);
}

//------------------------------------------------------------------------------
vtkIdType vtkAbstractContextItem::StackAbove(vtkIdType index, vtkIdType under)
{
  vtkIdType res = index;
  if (index == under || index < 0)
  {
    return res;
  }
  vtkIdType start = 0;
  vtkIdType middle = 0;
  vtkIdType end = 0;
  if (under == -1)
  {
    start = 0;
    middle = index;
    end = index + 1;
    res = 0;
  }
  else if (index > under)
  {
    start = under + 1;
    middle = index;
    end = index + 1;
    res = start;
  }
  else // if (index < under)
  {
    start = index;
    middle = index + 1;
    end = under + 1;
    res = end - 1;
  }
  std::rotate(this->Children->begin() + start, this->Children->begin() + middle,
    this->Children->begin() + end);
  return res;
}

//------------------------------------------------------------------------------
vtkIdType vtkAbstractContextItem::Lower(vtkIdType index)
{
  return this->StackUnder(index, 0);
}

//------------------------------------------------------------------------------
vtkIdType vtkAbstractContextItem::StackUnder(vtkIdType child, vtkIdType above)
{
  return this->StackAbove(child, above - 1);
}

//------------------------------------------------------------------------------
bool vtkAbstractContextItem::Hit(const vtkContextMouseEvent&)
{
  return false;
}

//------------------------------------------------------------------------------
bool vtkAbstractContextItem::MouseEnterEvent(const vtkContextMouseEvent&)
{
  return false;
}

//------------------------------------------------------------------------------
bool vtkAbstractContextItem::MouseMoveEvent(const vtkContextMouseEvent&)
{
  return false;
}

//------------------------------------------------------------------------------
bool vtkAbstractContextItem::MouseLeaveEvent(const vtkContextMouseEvent&)
{
  return false;
}

//------------------------------------------------------------------------------
bool vtkAbstractContextItem::MouseButtonPressEvent(const vtkContextMouseEvent&)
{
  return false;
}

//------------------------------------------------------------------------------
bool vtkAbstractContextItem::MouseButtonReleaseEvent(const vtkContextMouseEvent&)
{
  return false;
}

//------------------------------------------------------------------------------
bool vtkAbstractContextItem::MouseDoubleClickEvent(const vtkContextMouseEvent&)
{
  return false;
}

//------------------------------------------------------------------------------
bool vtkAbstractContextItem::MouseWheelEvent(const vtkContextMouseEvent&, int)
{
  return false;
}

//------------------------------------------------------------------------------
bool vtkAbstractContextItem::KeyPressEvent(const vtkContextKeyEvent&)
{
  return false;
}

//------------------------------------------------------------------------------
bool vtkAbstractContextItem::KeyReleaseEvent(const vtkContextKeyEvent&)
{
  return false;
}

//------------------------------------------------------------------------------
vtkAbstractContextItem* vtkAbstractContextItem::GetPickedItem(const vtkContextMouseEvent& mouse)
{
  vtkContextMouseEvent childMouse = mouse;
  childMouse.SetPos(this->MapFromParent(mouse.GetPos()));
  childMouse.SetLastPos(this->MapFromParent(mouse.GetLastPos()));
  for (vtkContextScenePrivate::const_reverse_iterator it = this->Children->rbegin();
       it != this->Children->rend(); ++it)
  {
    vtkAbstractContextItem* item = (*it)->GetPickedItem(childMouse);
    if (item)
    {
      return item;
    }
  }
  return this->Hit(mouse) ? this : nullptr;
}

//------------------------------------------------------------------------------
void vtkAbstractContextItem::ReleaseGraphicsResources()
{
  for (vtkContextScenePrivate::const_iterator it = this->Children->begin();
       it != this->Children->end(); ++it)
  {
    (*it)->ReleaseGraphicsResources();
  }
  if (!this->Scene)
  {
    return;
  }
  this->ReleaseGraphicsCache();
}

//------------------------------------------------------------------------------
void vtkAbstractContextItem::ReleaseGraphicsCache()
{
  // Remove our cache from 2D and 3D devices.
  if (auto lastPainter = this->Scene->GetLastPainter())
  {
    if (auto device2d = lastPainter->GetDevice())
    {
      device2d->ReleaseCache(reinterpret_cast<std::uintptr_t>(this));
    }
    if (auto ctx3D = lastPainter->GetContext3D())
    {
      if (auto device3D = ctx3D->GetDevice())
      {
        device3D->ReleaseCache(reinterpret_cast<std::uintptr_t>(this));
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkAbstractContextItem::SetScene(vtkContextScene* scene)
{
  this->Scene = scene;
  this->Children->SetScene(scene);
}

//------------------------------------------------------------------------------
void vtkAbstractContextItem::SetParent(vtkAbstractContextItem* parent)
{
  this->Parent = parent;
}

//------------------------------------------------------------------------------
vtkVector2f vtkAbstractContextItem::MapToParent(const vtkVector2f& point)
{
  return point;
}

//------------------------------------------------------------------------------
vtkVector2f vtkAbstractContextItem::MapFromParent(const vtkVector2f& point)
{
  return point;
}

//------------------------------------------------------------------------------
vtkVector2f vtkAbstractContextItem::MapToScene(const vtkVector2f& point)
{
  if (this->Parent)
  {
    vtkVector2f p = this->MapToParent(point);
    p = this->Parent->MapToScene(p);
    return p;
  }
  else
  {
    return this->MapToParent(point);
  }
}

//------------------------------------------------------------------------------
vtkVector2f vtkAbstractContextItem::MapFromScene(const vtkVector2f& point)
{
  if (this->Parent)
  {
    vtkVector2f p = this->Parent->MapFromScene(point);
    p = this->MapFromParent(p);
    return p;
  }
  else
  {
    return this->MapFromParent(point);
  }
}

//------------------------------------------------------------------------------
void vtkAbstractContextItem::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Visible: " << this->Visible << '\n';
  os << indent << "Interactive: " << this->Interactive << '\n';
  os << indent << "Scene: " << this->Scene << '\n';
  os << indent << "Parent: " << this->Parent << '\n';
  os << indent << "Children: " << this->Children << '\n';
  if (this->Children)
  {
    this->Children->PrintSelf(os, indent.GetNextIndent());
  }
}
VTK_ABI_NAMESPACE_END
