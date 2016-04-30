#ifndef __BLOCK_TEMPLATE__
#define __BLOCK_TEMPLATE__

#include "TLibCommon\TypeDef.h"
#include "TComPicYuv.h"
#include "TComDataCU.h"
#include "TComQuadTree.h"

#include <vector>

#define BLOCK_SIZE 4
#define BLOCK_NUM_IN_TEMPLATE 3

extern vector<UInt> g_auiBlockOrgToRsmpld[MAX_NUM_COMPONENT][2];
extern vector<UInt> g_auiBlockRsmpldToOrg[MAX_NUM_COMPONENT][2];

Void initCoordinateMap(UInt uiSourceWidth, UInt uiSourceHeight, UInt uiMaxCUWidth, UInt uiMaxCUHeight, UInt uiBlockSize, ChromaFormat chromaFormatIDC);

Void matchTemplate(TComDataCU* rpcTempCU, UInt uiBlockSize);

Void getTemplate(vector<vector<Pel>> vTemplate[],ComponentID cId, UInt uiBlockIdX, UInt uiBlockIdY, UInt uiBlockSize, UInt uiStride, Pel* pRecoBuffer);

UInt getHashvalue(vector<vector<Pel>> vTemplate[]);

UInt getAverageGradient(vector<vector<Pel>>& matrix);

UInt getDC(vector<vector<Pel>>& matrix);

Void getPelValue(vector<vector<Pel>>& matrix, UInt uiBlockIdX, UInt uiBlockIdY, UInt uiBlockSize, UInt uiStride, Pel* pRecoBuffer);






#endif