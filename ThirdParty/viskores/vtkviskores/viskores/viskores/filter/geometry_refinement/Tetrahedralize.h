//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef viskores_filter_geometry_refinement_Tetrahedralize_h
#define viskores_filter_geometry_refinement_Tetrahedralize_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/geometry_refinement/viskores_filter_geometry_refinement_export.h>

namespace viskores
{
namespace filter
{
namespace geometry_refinement
{

/// @brief Convert all polyhedra of a `viskores::cont::DataSet` into tetrahedra.
///
/// Note that although the tetrahedra will occupy the same space of the cells that
/// they replace, the interpolation of point fields within these cells might differ.
/// For example, the first order interpolation of a hexahedron uses trilinear
/// interpolation, which actually results in cubic equations. This differs from the
/// purely linear field in a tetrahedron, so the tetraheda replacement of the hexahedron
/// will not have exactly the same interpolation.
class VISKORES_FILTER_GEOMETRY_REFINEMENT_EXPORT Tetrahedralize : public viskores::filter::Filter
{
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;
};

} // namespace geometry_refinement
} // namespace filter
} // namespace viskores

#endif // viskores_filter_geometry_refinement_Tetrahedralize_h
