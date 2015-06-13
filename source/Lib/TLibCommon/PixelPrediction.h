#ifndef _PIXEL_GRANULARITY_PREDICTION_
#define _PIXEL_GRANULARITY_PREDICTION_
#include "TLibCommon\TypeDef.h"
#include "TComPicYuv.h"
#include "TComDataCU.h"
//#include "TComPic.h"
//#include "TypeDef.h"
#include <vector>

using namespace std;

// ======== Macro ========

#define MAX_PT_NUM		16777216
#define EXTEG			16

#define GOLOMB_EXP_BUFFER  (sizeof(TCoeff)*8)
#define INTRA_PR_PALETTE_NUM  4
#define MAX_BUFFER 64*64
// ======== Structures ========

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
}*PMatchMetric, MatchMetric;

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
	UInt m_uiHashValue;			///< hash value
} Pixel, *PPixel;

typedef struct _PixelTemplate {
	UInt m_PX;
	UInt m_PY;
	UInt m_uiNumUsed;	// how many times this pixel has been used to predicted
	_PixelTemplate * m_pptNext;
	_PixelTemplate(UInt uiX, UInt uiY) :m_PX(uiX), m_PY(uiY), m_uiNumUsed(0), m_pptNext(NULL){};
} PixelTemplate, *PPixelTemplate;

// ======== Global Variable ========

extern vector<UInt> g_auiOrgToRsmpld[MAX_NUM_COMPONENT][2];			// 0->x,1->y
extern vector<UInt> g_auiRsmpldToOrg[MAX_NUM_COMPONENT][2];

extern int g_auiTemplateOffset[21][2];

extern PixelTemplate*	g_pPixelTemplate[MAX_NUM_COMPONENT][MAX_PT_NUM];	        ///< hash table
extern vector<PixelTemplate*>	g_pPixelTemplatePool;								///< convinient for releasing memory
extern int g_auiTemplateOffset[21][2];

extern TComPicYuv* g_pcYuvPred;
extern TComPicYuv* g_pcYuvResi;

// ======== Global Function ========

// release hash table memory
Void releaseMemoryPTHashTable();
// init hash table
Void initPTHashTable();
// init g_auiOrgToRsmpld,g_auiRsmpldToOrg
Void initCoordinateMap(UInt uiSourceWidth, UInt uiSourceHeight, UInt uiMaxCUWidth, UInt uiMaxCUHeight, ChromaFormat chromaFormatIDC);

// template matching

Void preDefaultMethod(TComDataCU*& rpcTempCU, Pixel** ppPixel);

Void updatePixelAfterCompressing(TComDataCU* pCtu, Pixel** ppPixel);

inline UInt getSerialIndex(UInt uiX, UInt uiY, UInt uiPicWidth);

Void getNeighbors(UInt uiX, UInt uiY, UInt uiPicWidth, Pixel* pPixel, Pixel* vTemplate);

UInt getHashValue(UInt uiX, UInt uiY, UInt uiPicWidth, Pixel* pPixel);

Void tryMatch(UInt uiX, UInt uiY, UInt uiCX, UInt uiCY, MatchMetric &mmMatchMetric, UInt uiPicWidth, Pixel* pPixel);

Void appendNewTemplate(UInt uiHashValue, PixelTemplate*& rpNewTemplate);

// ---- Revise Anomaly Residue ----

typedef struct _ResiPLTInfo
{
	UInt m_uiX, m_uiY;			///< position in a CTU
	UInt m_uiSign;				///< 0 --> negitive, 1 --> positive
	UInt m_uiPLTIdx;			///< corresponding index in the palette
}ResiPLTInfo;

int   ExpGolombEncode(int x, int *buffer,int &pos);
int   ExpGolombDecode(int *buffer,int &pos);
int   UnaryEncode(int n,int *buffer,int &pos);
int   UnaryDecode(int *buffer,int &pos);
int   ReadBits(int len,int *buffer,int &pos);
int   ReadBit(int *buffer,int &pos);
Void WriteBit(int n,TCoeff *buffer,int &pos) ;
//Void WriteBits( TCoeff index, int num, TCoeff *buffer, int &pos)

UInt  PositionCode_PathLeast(TComYuv *pcResiYuv,TComTU&     rTu,const ComponentID compID, TCoeff* pcCoeff  );
UInt  PositionCode_Predict(TComYuv *pcResiYuv,TComTU&     rTu,const ComponentID compID, TCoeff* pcCoefff  );
UInt  PositionCode_All(TComYuv *pcResiYuv,TComTU&     rTu,const ComponentID compID, TCoeff* pcCoeff );

#endif // !_PIXEL_PREDICTION_
