// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAnariVolumeNode
 * @brief   links vtkVolume and vtkMapper to ANARI
 *
 * Translates vtkVolume/Mapper state into ANARI rendering calls
 *
 * @par Thanks:
 * Kevin Griffin kgriffin@nvidia.com for creating and contributing the class
 * and NVIDIA for supporting this work.
 */

#ifndef vtkAnariVolumeNode_h
#define vtkAnariVolumeNode_h

#include "vtkRenderingAnariModule.h" // For export macro
#include "vtkVolumeNode.h"

VTK_ABI_NAMESPACE_BEGIN

class vtkInformationStringKey;

class VTKRENDERINGANARI_EXPORT vtkAnariVolumeNode : public vtkVolumeNode
{
public:
  static vtkAnariVolumeNode* New();
  vtkTypeMacro(vtkAnariVolumeNode, vtkVolumeNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Overridden to take into account this renderables time, including
   * mapper and data into mapper inclusive of composite input
   */
  vtkMTimeType GetMTime() override;

  /**
   * Name of the node, used for debugging or representation
   * metadata in case ANARI output is written out to intermediate
   * authoring stages.
   * \ingroup InformationKeys
   */
  static vtkInformationStringKey* VOLUME_NODE_NAME();

protected:
  vtkAnariVolumeNode() = default;

private:
  vtkAnariVolumeNode(const vtkAnariVolumeNode&) = delete;
  void operator=(const vtkAnariVolumeNode&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
