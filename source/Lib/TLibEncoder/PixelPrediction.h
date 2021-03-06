
#ifndef _PIXEL_GRANULARITY_PREDICTION_
#define _PIXEL_GRANULARITY_PREDICTION_
#include "TLibCommon\TypeDef.h"

// ---- Macro ----

#define MAX_PT_NUM		16777216
#define EXTEG			16

// ---- Structures ----

// ---- Template Matching ----
typedef struct _MatchMetric
{
#if LOSSY
	Double m_lfAbsDiff;
#else
	UInt m_uiAbsDiff;
#endif
	UInt m_uiNumMatchPoints;
	UInt m_uiNumValidPoints;
	UInt m_uiX, m_uiY;
	_MatchMetric() :m_uiAbsDiff(0), m_uiNumMatchPoints(0), m_uiNumValidPoints(0), m_uiX(0), m_uiY(0){};
}*PMatchMetric,MatchMetric;

typedef struct _Pixel {
	Bool m_bIsRec;				///< pixel is reconstructed or not
	UInt m_uiX;					///< pixel x position
	UInt m_uiY;					///< pixel y position
	UInt m_uiOrg;				///< pixel original value
	UInt m_uiPred;				///< pixel prediction value
	Int  m_iResi;				///< pixel prediction residule value
	UInt m_uiReco;				///< pixel reconstruction value
	Int  m_iDiff;				///< pixel reconstruction residule value
	UInt m_uiBestPX;			///< best prediction template x position
	UInt m_uiBestPY;			///< best prediction template x position
	MatchMetric m_mmMatch;		///< best match metric
} Pixel, *PPixel;

typedef struct _PixelTemplate {
	UInt m_PX;
	UInt m_PY;
	UInt m_uiNumUsed;	// how many times this pixel has been used to predicted
	_PixelTemplate * m_pptNext;
	_PixelTemplate(UInt uiX, UInt uiY) :m_PX(uiX), m_PY(uiY), m_uiNumUsed(0), m_pptNext(NULL){};
} PixelTemplate, *PPixelTemplate;

// ---- Revise Anomaly Residue ----

typedef struct _ResiPLTInfo
{
	UInt m_uiX, m_uiY;			///< position in a CTU
	UInt m_uiSign;				///< 0 --> negitive, 1 --> positive
	UInt m_uiPLTIdx;			///< corresponding index in the palette

}ResiPLTInfo;

#endif // !_PIXEL_PREDICTION_
