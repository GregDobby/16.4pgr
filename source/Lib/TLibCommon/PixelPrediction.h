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

#define MAX_PT_NUM		67108864 //16777216
#define EXTEG			16
#define MAX_PLT_SIZE	16

#define GOLOMB_EXP_BUFFER  (sizeof(TCoeff)*8)
#define INTRA_PR_PALETTE_NUM  4
#define MAX_BUFFER 4096
#define INSERT_LIMIT 5
#define LIST_LIMIT 10
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
	_MatchMetric() :m_uiAbsDiff(255), m_uiNumMatchPoints(0), m_uiNumValidPoints(0), m_uiX(0), m_uiY(0){};
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

enum TemplateState
{
	NEW = 0,
	DISPLACE = 1,
	TO_BE_REMOVED = 2
};
typedef struct _PixelTemplate {
	UInt m_PX;
	UInt m_PY;
	UInt m_uiNumUsed;	// how many times this pixel has been used to predicted
	UInt m_uiHashValue1;
	UInt m_uiHashValue2;
	UInt m_uiNumTemplate;
	UInt m_uiState;
	_PixelTemplate * m_pptNext;
	_PixelTemplate(UInt uiX, UInt uiY, UInt uiHashValue1,UInt uiHashValue2,UInt uiNumTemplate,UInt uiState) :m_PX(uiX), m_PY(uiY), m_uiNumUsed(0), m_uiHashValue1(uiHashValue1),m_uiHashValue2(uiHashValue2), m_uiNumTemplate(uiNumTemplate),m_uiState(uiState), m_pptNext(NULL){};

} PixelTemplate, *PPixelTemplate;

typedef struct _Palette
{
	Pel m_pEntry[MAX_PLT_SIZE];
	UInt m_uiSize;
	_Palette() :m_uiSize(0){};
}Palette;

// ======== Global Variable ========

extern vector<UInt> g_auiOrgToRsmpld[MAX_NUM_COMPONENT][2];			// 0->x,1->y
extern vector<UInt> g_auiRsmpldToOrg[MAX_NUM_COMPONENT][2];

extern int g_auiTemplateOffset[21][2];


extern PixelTemplate* g_pLookupTable[MAX_NUM_COMPONENT][MAX_PT_NUM];

//extern PixelTemplate*	g_pPixelTemplate[MAX_NUM_COMPONENT][MAX_PT_NUM];	        ///< hash table
//extern vector<PixelTemplate*>	g_pPixelTemplatePool;								///< convinient for releasing memory
extern int g_auiTemplateOffset[21][2];

extern TComPicYuv* g_pcYuvPred;
extern TComPicYuv* g_pcYuvResi;
extern TComPicYuv* g_pcYuvAbnormalResi;

extern TCoeff *lumaCoef;
extern TCoeff *lumaCoefR;

extern Palette				g_ppPalette[MAX_NUM_COMPONENT];
extern Palette				g_ppCTUPalette[MAX_NUM_COMPONENT];
// ======== Global Function ========

// release hash table memory
Void clearPTHashTable();
// init hash table
Void initPTHashTable();
// init g_auiOrgToRsmpld,g_auiRsmpldToOrg
Void initCoordinateMap(UInt uiSourceWidth, UInt uiSourceHeight, UInt uiMaxCUWidth, UInt uiMaxCUHeight, ChromaFormat chromaFormatIDC);

// template matching

Void matchTemplate(TComDataCU*& rpcTempCU, Pixel** ppPixel);

Void updatePixel(TComDataCU* pCtu, Pixel** ppPixel);

inline UInt getSerialIndex(UInt uiX, UInt uiY, UInt uiPicWidth);

Void getNeighbors(UInt uiX, UInt uiY, UInt uiPicWidth, Pixel* pPixel, Pixel* vTemplate);

Void getHashValue(UInt uiX, UInt uiY, UInt uiPicWidth, Pixel* pPixel, UInt& uiHashValue1, UInt& uiHashValue2);

Void tryMatch(UInt uiX, UInt uiY, UInt uiCX, UInt uiCY, MatchMetric &mmMatchMetric, UInt uiPicWidth, Pixel* pPixel);

UInt getNumTemplate(UInt uiX, UInt uiY, UInt uiPicWidth, Pixel* pPixel);

Void updateLookupTable(TComDataCU*& rpcTempCU, Pixel** ppPixel);
// ---- Revise Anomaly Residue ----

typedef struct _PelCount
{
	Pel m_uiVal;
	UInt m_uiCount;
	_PelCount(Pel val) :m_uiVal(val), m_uiCount(0){};

	// descend
}PelCount;

Void derivePGRGlobalPLT(TComPicYuv* pcOrgYuv);

Void derivePGRPLT(TComDataCU* pcCu);

int getG0Bits(int x);
int   ExpGolombEncode(int x, int *buffer, UInt &pos);
int   ExpGolombDecode(int *buffer, UInt &pos);
int   UnaryEncode(int n, int *buffer, UInt &pos);
int   UnaryDecode(int *buffer, UInt &pos);
int   ReadBits(int len, int *buffer, UInt &pos);
int   ReadBit(int *buffer, UInt &pos);
Void WriteBit(int n, TCoeff *buffer, UInt &pos);

//Void WriteBits( TCoeff index, int num, TCoeff *buffer, int &pos)

Void  PositionCode_PathLeast(TComYuv *pcResiYuv, const ComponentID compID, TCoeff* pcCoeff, UInt* puiPLTIdx, UInt& uiNumIdx);

UInt  PositionCode_Predict(TComYuv *pcResiYuv, TComTU&     rTu, const ComponentID compID, TCoeff* pcCoefff);

#endif // !_PIXEL_PREDICTION_
