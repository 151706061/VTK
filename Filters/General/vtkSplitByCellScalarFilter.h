// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSplitByCellScalarFilter
 * @brief   splits input dataset according an integer cell scalar array.
 *
 * vtkSplitByCellScalarFilter is a filter that splits any dataset type
 * according an integer cell scalar value (typically a material identifier) to
 * a multiblock. Each block of the output contains cells that have the same
 * scalar value. Output blocks will be of type vtkUnstructuredGrid except if
 * input is of type vtkPolyData. In that case output blocks are of type
 * vtkPolyData.
 *
 * As vtkMultiBlockDataSets tends to be replaced by vtkPartitionedDataSetCollection,
 * vtkExplodeDataSet should be used in place. Also vtkExplodeDataSet benefits from
 * SMPTools threading acceleration.
 *
 * @sa
 * vtkThreshold, vtkExplodeDataSet
 *
 * @par Thanks:
 * This class was written by Joachim Pouderoux, Kitware 2016.
 */

#ifndef vtkSplitByCellScalarFilter_h
#define vtkSplitByCellScalarFilter_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSGENERAL_EXPORT vtkSplitByCellScalarFilter : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkSplitByCellScalarFilter* New();
  vtkTypeMacro(vtkSplitByCellScalarFilter, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify if input points array must be passed to output blocks. If so,
   * filter processing is faster but outblocks will contains more points than
   * what is needed by the cells it owns. If not, a new points array is created
   * for every block and it will only contains points for copied cells.
   * Note that this function is only possible for PointSet datasets.
   * The default is true.
   */
  vtkGetMacro(PassAllPoints, bool);
  vtkSetMacro(PassAllPoints, bool);
  vtkBooleanMacro(PassAllPoints, bool);
  ///@}

protected:
  vtkSplitByCellScalarFilter();
  ~vtkSplitByCellScalarFilter() override;

  // Usual data generation method
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  bool PassAllPoints;

private:
  vtkSplitByCellScalarFilter(const vtkSplitByCellScalarFilter&) = delete;
  void operator=(const vtkSplitByCellScalarFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
