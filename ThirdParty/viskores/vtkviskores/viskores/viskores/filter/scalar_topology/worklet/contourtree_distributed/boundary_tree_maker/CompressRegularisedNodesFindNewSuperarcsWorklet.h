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
// Copyright (c) 2018, The Regents of the University of California, through
// Lawrence Berkeley National Laboratory (subject to receipt of any required approvals
// from the U.S. Dept. of Energy).  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// (1) Redistributions of source code must retain the above copyright notice, this
//     list of conditions and the following disclaimer.
//
// (2) Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
// (3) Neither the name of the University of California, Lawrence Berkeley National
//     Laboratory, U.S. Dept. of Energy nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//
//=============================================================================
//
//  This code is an extension of the algorithm presented in the paper:
//  Parallel Peak Pruning for Scalable SMP Contour Tree Computation.
//  Hamish Carr, Gunther Weber, Christopher Sewell, and James Ahrens.
//  Proceedings of the IEEE Symposium on Large Data Analysis and Visualization
//  (LDAV), October 2016, Baltimore, Maryland.
//
//  The PPP2 algorithm and software were jointly developed by
//  Hamish Carr (University of Leeds), Gunther H. Weber (LBNL), and
//  Oliver Ruebel (LBNL)
//==============================================================================

#ifndef viskores_worklet_contourtree_distributed_bract_maker_compress_regularised_nodes_find_new_superarcs_worklet_h
#define viskores_worklet_contourtree_distributed_bract_maker_compress_regularised_nodes_find_new_superarcs_worklet_h

#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace contourtree_distributed
{
namespace bract_maker
{

/// Step 1 of IdentifyRegularisedSupernodes
class CompressRegularisedNodesFindNewSuperarcsWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayIn newVertexId, // Input
                                FieldIn bractSuperarcs,   // input
                                WholeArrayIn upNeighbour, //input
                                WholeArrayIn downNeighbour,
                                WholeArrayOut newSuperarc // output
  );
  using ExecutionSignature = void(InputIndex, _1, _2, _3, _4, _5);
  using InputDomain = _1;

  // Default Constructor
  VISKORES_EXEC_CONT
  CompressRegularisedNodesFindNewSuperarcsWorklet() {}

  template <typename InFieldPortalType, typename OutFieldPortalType>
  VISKORES_EXEC void operator()(const viskores::Id& returnIndex,
                                const InFieldPortalType& newVertexIdPortal,
                                const viskores::Id& bractSuperarcIdIn,
                                const InFieldPortalType& downNeighbourPortal,
                                const InFieldPortalType& upNeighbourPortal,
                                const OutFieldPortalType& newSuperarcPortal) const
  {
    // per vertex
    // skip all unnecessary vertices
    viskores::Id newVertexIdIn = newVertexIdPortal.Get(returnIndex);
    if (viskores::worklet::contourtree_augmented::NoSuchElement(newVertexIdIn))
    {
      return;
    }

    // retrieve the new ID
    viskores::Id newId = newVertexIdIn;

    // for necessary vertices, look at the superarc
    viskores::Id oldInbound = bractSuperarcIdIn;

    //  i. points to nothing - copy it
    if (viskores::worklet::contourtree_augmented::NoSuchElement((oldInbound)))
    {
      newSuperarcPortal.Set(
        newId, (viskores::Id)viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT);
    }
    //  ii. points to a necessary vertex - copy it
    else if (!viskores::worklet::contourtree_augmented::NoSuchElement(
               newVertexIdPortal.Get(oldInbound)))
    {
      newSuperarcPortal.Set(newId, newVertexIdPortal.Get(oldInbound));
    }
    // iii. points to an unnecessary vertex
    else
    { // points to an unnecessary vertex
      // check the old up neighbour
      viskores::Id upNbr =
        viskores::worklet::contourtree_augmented::MaskedIndex(upNeighbourPortal.Get(oldInbound));
      viskores::Id downNbr =
        viskores::worklet::contourtree_augmented::MaskedIndex(downNeighbourPortal.Get(oldInbound));

      // if it's us, we've got a downwards inbound arc
      // and the down neighbour holds the right new superarc
      if (upNbr == returnIndex)
      {
        newSuperarcPortal.Set(newId, newVertexIdPortal.Get(downNbr));
      }
      // otherwise the up neighbour does
      else
      {
        newSuperarcPortal.Set(newId, newVertexIdPortal.Get(upNbr));
      }
    } // points to an unnecessary vertex

    // In serial this worklet implements the following operation
    /*
    for (indexType returnIndex = 0; returnIndex < bractVertexSuperset.size(); returnIndex++)
      { // per vertex
      // skip all unnecessary vertices
      if (noSuchElement(newVertexID[returnIndex]))
        continue;

      // retrieve the new ID
      indexType newID = newVertexID[returnIndex];

      // for necessary vertices, look at the superarc
      indexType oldInbound = bract->superarcs[returnIndex];

      //  i. points to nothing - copy it
      if (noSuchElement(oldInbound))
        newSuperarc[newID] = NO_SUCH_ELEMENT;
      //  ii. points to a necessary vertex - copy it
      else if (!noSuchElement(newVertexID[oldInbound]))
        newSuperarc[newID] = newVertexID[oldInbound];
      // iii. points to an unnecessary vertex
      else
        { // points to an unnecessary vertex
        // check the old up neighbour
        indexType upNbr = maskedIndex(upNeighbour[oldInbound]);
        indexType downNbr = maskedIndex(downNeighbour[oldInbound]);

        // if it's us, we've got a downwards inbound arc
        // and the down neighbour holds the right new superarc
        if (upNbr == returnIndex)
          newSuperarc[newID] = newVertexID[downNbr];
        // otherwise the up neighbour does
        else
          newSuperarc[newID] = newVertexID[upNbr];
        } // points to an unnecessary vertex
      } // per vertex
    */
  }
}; // CompressRegularisedNodesFindNewSuperarcsWorklet


} // namespace bract_maker
} // namespace contourtree_distributed
} // namespace worklet
} // namespace viskores

#endif
