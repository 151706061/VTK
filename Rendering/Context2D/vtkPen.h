// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkPen
 * @brief   provides a pen that draws the outlines of shapes drawn
 * by vtkContext2D.
 *
 *
 * The vtkPen defines the outline of shapes that are drawn by vtkContext2D.
 * The color is stored as four unsigned chars (RGBA), where the
 * opacity defaults to 255, but can be modified separately to the other
 * components. Ideally we would use a lightweight color class to store and pass
 * around colors.
 */

#ifndef vtkPen_h
#define vtkPen_h

#include "vtkColor.h" // Needed for vtkColor4ub
#include "vtkObject.h"
#include "vtkRenderingContext2DModule.h" // For export macro
#include "vtkWrappingHints.h"            // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGCONTEXT2D_EXPORT VTK_MARSHALAUTO vtkPen : public vtkObject
{
public:
  vtkTypeMacro(vtkPen, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkPen* New();

  /**
   * Enum of the available line types.
   */
  enum
  {
    NO_PEN,
    SOLID_LINE,
    DASH_LINE,
    DOT_LINE,
    DASH_DOT_LINE,
    DASH_DOT_DOT_LINE,
    DENSE_DOT_LINE
  };

  /**
   * Set the type of line that the pen should draw. The default is solid (1).
   */
  void SetLineType(int type);

  /**
   * Get the type of line that the pen will draw.
   */
  int GetLineType();

  /**
   * Set the color of the brush with three component doubles (RGB), ranging from
   * 0.0 to 1.0.
   */
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_REDUNDANT)
  void SetColorF(double color[3]);

  /**
   * Set the color of the brush with three component doubles (RGB), ranging from
   * 0.0 to 1.0.
   */
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_REDUNDANT)
  void SetColorF(double r, double g, double b);

  /**
   * Set the color of the brush with four component doubles (RGBA), ranging from
   * 0.0 to 1.0.
   */
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_REDUNDANT)
  void SetColorF(double r, double g, double b, double a);

  /**
   * Set the opacity with a double, ranging from 0.0 (transparent) to 1.0
   * (opaque).
   */
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_REDUNDANT)
  void SetOpacityF(double a);

  /**
   * Set the color of the brush with three component unsigned chars (RGB),
   * ranging from 0 to 255.
   */
  void SetColor(unsigned char color[3]);

  /**
   * Set the color of the brush with three component unsigned chars (RGB),
   * ranging from 0 to 255.
   */
  void SetColor(unsigned char r, unsigned char g, unsigned char b);

  ///@{
  /**
   * Set the color of the brush with four component unsigned chars (RGBA),
   * ranging from 0 to 255.
   */
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_REDUNDANT)
  void SetColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
  void SetColor(const vtkColor4ub& color);
  ///@}

  /**
   * Set the opacity with an unsigned char, ranging from 0 (transparent) to 255
   * (opaque).
   */
  void SetOpacity(unsigned char a);

  /**
   * Get the color of the brush - expects a double of length 3 to copy into.
   */
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_REDUNDANT)
  void GetColorF(double color[3]);

  /**
   * Get the color of the brush - expects an unsigned char of length 3.
   */
  void GetColor(unsigned char color[3]);

  /**
   * Get the color of the pen.
   */
  vtkColor4ub GetColorObject();

  /**
   * Get the opacity (unsigned char), ranging from 0 (transparent) to 255
   * (opaque).
   */
  unsigned char GetOpacity();

  /**
   * Get the color of the brush - gives a pointer to the underlying data.
   */
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_REDUNDANT)
  unsigned char* GetColor() { return this->Color; }

  ///@{
  /**
   * Set/Get the width of the pen.
   */
  vtkSetMacro(Width, float);
  vtkGetMacro(Width, float);
  ///@}

  /**
   * Make a deep copy of the supplied pen.
   */
  void DeepCopy(vtkPen* pen);

protected:
  vtkPen();
  ~vtkPen() override;

  ///@{
  /**
   * Storage of the color in RGBA format (0-255 per channel).
   */
  unsigned char* Color;
  vtkColor4ub PenColor;
  ///@}

  /**
   * Store the width of the pen in pixels.
   */
  float Width;

  /**
   * The type of line to be drawn with this pen.
   */
  int LineType;

private:
  vtkPen(const vtkPen&) = delete;
  void operator=(const vtkPen&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkPen_h
