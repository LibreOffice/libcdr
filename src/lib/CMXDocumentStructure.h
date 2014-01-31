/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libcdr project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __CMXDOCUMENTSTRUCTURE_H__
#define __CMXDOCUMENTSTRUCTURE_H__

#define CMX_Tag_Null 0

#define CMX_Tag_AddClippingRegion_RegionSpecification 1
#define CMX_Tag_AddClippingRegion_ClipModeRecoverySpecification 2

#define CMX_Tag_AddGlobalTransform_Matrix 1
#define CMX_Tag_SetGlobalTransform_Matrix 2

#define CMX_Tag_BeginEmbedded_EmbeddedSpecification 1

#define CMX_Tag_BeginGroup_GroupSpecification 1

#define CMX_Tag_BeginGroup_GroupSpecification 1

#define CMX_Tag_BeginLayer_LayerSpecification 1
#define CMX_Tag_BeginLayer_Matrix 2
#define CMX_Tag_BeginLayer_MappingMode 3

#define CMX_Tag_BeginPage_PageSpecification 1
#define CMX_Tag_BeginPage_Matrix 2
#define CMX_Tag_BeginPage_MappingMode 3

#define CMX_Tag_BeginParagraph_RenderingAttr 1
#define CMX_Tag_BeginParagraph_FontSpecification 2
#define CMX_Tag_BeginParagraph_KerningSpecification 3
#define CMX_Tag_BeginParagraph_Justification 4
#define CMX_Tag_BeginParagraph_SpacingSpecification 5
#define CMX_Tag_BeginParagraph_TabSpecification 6
#define CMX_Tag_BeginParagraph_BulletSpecification 7
#define CMX_Tag_BeginParagraph_Indentation 8
#define CMX_Tag_BeginParagraph_Hyphenation 9
#define CMX_Tag_BeginParagraph_DropCap 10
#define CMX_Tag_BeginParagraph_FontSpec_UseFontCombination 11
#define CMX_Tag_BeginParagraph_BulletSpec_UseFontCombination 12
#define CMX_Tag_BeginParagraph_DropCapSpec_UseFontCombination 13
#define CMX_Tag_BeginParagraph_TabSpecification_Extra 15

#define CMX_Tag_BeginProcedure_ProcedureSpecification 1

#define CMX_Tag_BeginTextGroup_RenderingAttr 1
#define CMX_Tag_BeginTextGroup_Matrix 2
#define CMX_Tag_BeginTextGroup_Rectangle 3

#define CMX_Tag_BeginTextObject_Reserved 1

#define CMX_Tag_BeginTextStream_TextStreamSpecification 1

#define CMX_Tag_CharInfo_Anchor_HotLink 1
#define CMX_Tag_CharInfo_CharInfo 2
#define CMX_Tag_CharInfo_EmbeddedCompleteCMX 3

#define CMX_Tag_Characters_CountIndex 1
#define CMX_Tag_Characters_FitTextShift 2

#define CMX_Tag_Comment_CommentSpecification 1

#define CMX_Tag_DrawImage_RenderingAttr 1
#define CMX_Tag_DrawImage_DrawImageSpecification 2
#define CMX_Tag_DrawImage_ProcRefForSoftBitmap 3

#define CMX_Tag_DrawChars_DrawCharsSpecification 1

#define CMX_Tag_Ellips_RenderingAttr 1
#define CMX_Tag_Ellips_EllipsSpecification 2

#define CMX_Tag_JumpAbsolute_Offset 1

#define CMX_Tag_PolyCurve_RenderingAttr 1
#define CMX_Tag_PolyCurve_PointList 2
#define CMX_Tag_Polycurve_BoundingBox 3
#define CMX_Tag_Polycurve_KeepFillForOpenPath 4

#define CMX_Tag_PushMappingMod_SourceDestination 1

#define CMX_Tag_PushTint_PushTintSpecification 1

#define CMX_Tag_Rectangle_RenderingAttr 1
#define CMX_Tag_Rectangle_RectangleSpecification 2

#define CMX_Tag_SetCharStyle_RenderingAttr 1
#define CMX_Tag_SetCharStyle_SetCharStyleSpecification 2

#define CMX_Tag_SimpleWideText_RenderingAttr 1
#define CMX_Tag_SimpleWideText_SimpleWideTextSpecification 2
#define CMX_Tag_SimpleWideText_CountCharSpecification 3
#define CMX_Tag_SimpleWideText_BoundingBox 4

#define CMX_Tag_TextFrame_ColumnSpecification 1
#define CMX_Tag_TextFrame_HeightSkewMatrix 2
#define CMX_Tag_TextFrame_VJustifyAutoFrameHeight 3
#define CMX_Tag_TextFrame_PointsMatrix 4

#define CMX_Tag_RenderAttr_OutlineSpec 1

#define CMX_Tag_RenderAttr_FillSpec 1

#define CMX_Tag_RenderAttr_FillSpec_Uniform 1

#define CMX_Tag_RenderAttr_FillSpec_Fountain_Base 1
#define CMX_Tag_RenderAttr_FillSpec_Fountain_Color 2

#define CMX_Tag_RenderAttr_FillSpec_Postscript_Base 1
#define CMX_Tag_RenderAttr_FillSpec_Postscript_UserFunc 2

#define CMX_Tag_RenderAttr_FillSpec_MonoBM 1

#define CMX_Tag_RenderAttr_FillSpec_ColorBM 1

#define CMX_Tag_RenderAttr_FillSpec_Texture 1

#define CMX_Tag_RenderAttr_FillSpec_TileTransfo 2
#define CMX_Tag_RenderAttr_FillSpec_LensTile 3

#define CMX_Tag_RenderAttr_LensSpec_Base 1
#define CMX_Tag_RenderAttr_LensSpec_BitmapLens7 5
#define CMX_Tag_RenderAttr_LensSpec_GlassExtColor 2
#define CMX_Tag_RenderAttr_LensSpec_FrozViewp 3
#define CMX_Tag_RenderAttr_LensSpec_ROP7 6

#define CMX_Tag_RenderAttr_ContainerSpec 1

#define CMX_Tag_Tiling 1

#define CMX_Tag_DescrSection_Arrow 1

#define CMX_Tag_DescrSection_Color_Base 1
#define CMX_Tag_DescrSection_Color_ColorDescr 2

#define CMX_Tag_DescrSection_Dash 1

#define CMX_Tag_DescrSection_Font_FontInfo 1
#define CMX_Tag_DescrSection_Font_Panose 2

#define CMX_Tag_DescrSection_Image_ImageInfo 1
#define CMX_Tag_DescrSection_Image_ImageInfo_Extra 4
#define CMX_Tag_DescrSection_Image_ImageInfo_LinkData 5

#define CMX_Tag_DescrSection_Image_ImageData 2

#define CMX_Tag_DescrSection_Image_ImageMask 3

#define CMX_Tag_DescrSection_Outline 1

#define CMX_Tag_DescrSection_LineStyle 1

#define CMX_Tag_DescrSection_Pen 1

#define CMX_Tag_DescrSection_Lens 1

#define CMX_Tag_DescrSection_Screen_Basic 1
#define CMX_Tag_DescrSection_Screen_PSFunction 2

#define CMX_Tag_EndTag 255


#define CMX_Command_AddClippingRegion 88
#define CMX_Command_AddGlobalTransform 94
#define CMX_Command_BeginEmbedded 22
#define CMX_Command_BeginGroup 13
#define CMX_Command_BeginLayer 11
#define CMX_Command_BeginPage 9
#define CMX_Command_BeginParagraph 99
#define CMX_Command_BeginProcedure 17
#define CMX_Command_BeginTextGroup 72
#define CMX_Command_BeginTextObject 70
#define CMX_Command_BeginTextStream 20
#define CMX_Command_CharInfo 101
#define CMX_Command_Characters 102
#define CMX_Command_ClearClipping 90
#define CMX_Command_Comment 2
#define CMX_Command_DrawImage 69
#define CMX_Command_DrawChars 65
#define CMX_Command_Ellipse 66
#define CMX_Command_EndEmbedded 23
#define CMX_Command_EndGroup 14
#define CMX_Command_EndLayer 12
#define CMX_Command_EndPage 10
#define CMX_Command_EndParagraph 100
#define CMX_Command_EndSection 18
#define CMX_Command_EndTextGroup 73
#define CMX_Command_EndTextObject 71
#define CMX_Command_EndTextStream 21
#define CMX_Command_JumpAbsolute 111
#define CMX_Command_PolyCurve 67
#define CMX_Command_PopMappingMode 92
#define CMX_Command_PopTint 104
#define CMX_Command_PushMappingMode 91
#define CMX_Command_PushTint 103
#define CMX_Command_Rectangle 68
#define CMX_Command_RemoveLastClippingRegion 89
#define CMX_Command_RestoreLastGlobalTransfo 95
#define CMX_Command_SetCharStyle 85
#define CMX_Command_SetGlobalTransfo 93
#define CMX_Command_SimpleWideText 86
#define CMX_Command_TextFrame 98

#endif // __CMXDOCUMENTSTRUCTURE_H__
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
