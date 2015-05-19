#include "PixelPrediction.h"
#include "TComChromaFormat.h"

vector<UInt> g_auiOrgToRsmpld[MAX_NUM_COMPONENT][2];
vector<UInt> g_auiRsmpldToOrg[MAX_NUM_COMPONENT][2];

PixelTemplate*	g_pPixelTemplate[MAX_NUM_COMPONENT][MAX_PT_NUM];	        ///< hash table
vector<PixelTemplate*>	g_pPixelTemplatePool;								///< convinient for releasing memory

// release hash table memory
Void releaseMemoryPTHashTable()
{
	for (int i = 0; i < g_pPixelTemplatePool.size(); i++)
	{
		delete g_pPixelTemplatePool[i];
		g_pPixelTemplatePool[i] = NULL;
	}
	g_pPixelTemplatePool.clear();
}

// init hash table
Void initPTHashTable()
{
	releaseMemoryPTHashTable();
	for (UInt ch = 0; ch < MAX_NUM_COMPONENT; ch++)
	{
		ComponentID cId = ComponentID(ch);
		for (UInt ui = 0; ui < MAX_PT_NUM; ui++)
		{
			g_pPixelTemplate[cId][ui] = NULL;
		}
	}
}

Void initCoordinateMap(UInt uiSourceWidth, UInt uiSourceHeight, UInt uiMaxCUWidth, UInt uiMaxCUHeight, ChromaFormat chromaFormatIDC)
{
	UInt uiNumValidComponents = getNumberValidComponents(chromaFormatIDC);
	for (UInt ch = 0; ch < uiNumValidComponents; ch++)
	{
		ComponentID cId = ComponentID(ch);
		UInt uiPicWidth = uiSourceWidth >> getComponentScaleX(cId, chromaFormatIDC);
		UInt uiPicHeight = uiSourceHeight >> getComponentScaleY(cId, chromaFormatIDC);

		// init elements in the map with value -1
		g_auiOrgToRsmpld[cId][0].resize(uiPicWidth, -1);		// orginal to resampled x
		g_auiOrgToRsmpld[cId][1].resize(uiPicHeight, -1);		// original to resampled y
		g_auiRsmpldToOrg[cId][0].resize(uiPicWidth, -1);		// resampled to original x
		g_auiRsmpldToOrg[cId][1].resize(uiPicHeight, -1);		// resampled to original y

		UInt uiStrideX = uiPicWidth / uiMaxCUWidth;			// sample stride in horizontal direction as well as the number of intact CUs in a row
		UInt uiStrideY = uiPicHeight / uiMaxCUHeight;		// sample stride in vertical direction as well as the number of intact CUs in a column

		UInt uiStrideXplus1 = uiStrideX + 1;
		UInt uiStrideYplus1 = uiStrideY + 1;

		UInt uiNumberUseBiggerStrideX = uiPicWidth % uiMaxCUWidth;		// number of bigger strides in x direction
		UInt uiNumberUseBiggerStrideY = uiPicHeight % uiMaxCUHeight;	// number of bigger strides in y direction

		// traverse the resampled picture
		for (UInt uiPicRsmpldY = 0; uiPicRsmpldY < uiPicHeight; uiPicRsmpldY++)
		{
			for (UInt uiPicRsmpldX = 0; uiPicRsmpldX < uiPicWidth; uiPicRsmpldX++)
			{
				UInt uiIdX = uiPicRsmpldX % uiMaxCUWidth;
				UInt uiIdY = uiPicRsmpldY % uiMaxCUHeight;
				UInt uiPicOrgX, uiPicOrgY;
				if (uiIdX < uiNumberUseBiggerStrideX)
					uiPicOrgX = uiPicRsmpldX / uiMaxCUWidth + uiIdX * uiStrideXplus1;	// corresponding X in the original picture
				else
					uiPicOrgX = uiPicRsmpldX / uiMaxCUWidth + uiNumberUseBiggerStrideX * uiStrideXplus1 + (uiIdX - uiNumberUseBiggerStrideX) * uiStrideX;

				if (uiIdY < uiNumberUseBiggerStrideY)
					uiPicOrgY = uiPicRsmpldY / uiMaxCUHeight + uiIdY * uiStrideYplus1;	// corresponding Y in the original picture
				else
					uiPicOrgY = uiPicRsmpldY / uiMaxCUHeight + uiNumberUseBiggerStrideY * uiStrideYplus1 + (uiIdY - uiNumberUseBiggerStrideY) * uiStrideY;
				
				g_auiOrgToRsmpld[cId][0][uiPicOrgX] = uiPicRsmpldX;
				g_auiOrgToRsmpld[cId][1][uiPicOrgY] = uiPicRsmpldY;

				g_auiRsmpldToOrg[cId][0][uiPicRsmpldX] = uiPicOrgX;
				g_auiRsmpldToOrg[cId][1][uiPicRsmpldY] = uiPicOrgY;
			}
		}
	}
}