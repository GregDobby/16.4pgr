#include <TLibCommon\TComDataCU.h>
#include <TLibCommon\TComPicYuv.h>
#include <TLibCommon\TComPic.h>
#include <TLibCommon\TComTU.h>
#include <TLibCommon\TComYuv.h>
#include "TypeDef.h"
#include "PixelPrediction.h"

#include <fstream>

vector<UInt> g_auiOrgToRsmpld[MAX_NUM_COMPONENT][2];
vector<UInt> g_auiRsmpldToOrg[MAX_NUM_COMPONENT][2];

PixelTemplate*	g_pPixelTemplate[MAX_NUM_COMPONENT][MAX_PT_NUM];	        ///< hash table
vector<PixelTemplate*>	g_pPixelTemplatePool;								///< convinient for releasing memory

int g_auiTemplateOffset[21][2] = { { -1, 0 }, { -1, -1 }, { 0, -1 }, { 1, -1 }, { 2, -1 }, { -2, 0 }, { -2, -1 }, { -2, -2 }, { -1, -2 }, { 0, -2 }, { 1, -2 },
{ 2, -2 }, { -3, 0 }, { -3, -1 }, { -3, -2 }, { -3, -3 }, { -2, -3 }, { -1, -3 }, { 0, -3 }, { 1, -3 }, { 2, -3 } };

TComPicYuv* g_pcYuvPred = NULL;
TComPicYuv* g_pcYuvResi = NULL;
UInt g_uiMaxCUWidth = 64;
UInt g_uiMaxCUHeight = 64;

Void preDefaultMethod(TComDataCU*& rpcTempCU, Pixel** ppPixel)
{
	// template matching
	UInt uiCUPelX = rpcTempCU->getCUPelX();			// x of upper left corner of the cu
	UInt uiCUPelY = rpcTempCU->getCUPelY();			// y of upper left corner of the

	UInt uiMaxCUWidth = rpcTempCU->getSlice()->getSPS()->getMaxCUWidth();		// max cu width
	UInt uiMaxCUHeight = rpcTempCU->getSlice()->getSPS()->getMaxCUHeight();		// max cu height

	// pic
	TComPic* pcPic = rpcTempCU->getPic();
	TComPicYuv* pcPredYuv = pcPic->getPicYuvPred();
	TComPicYuv* pcResiYuv = pcPic->getPicYuvResi();
	UInt uiNumValidCopmonent = pcPic->getNumberValidComponents();

	//cout << uiCUPelX << "\t" << uiCUPelY << endl;
	for (UInt ch = 0; ch < uiNumValidCopmonent; ch++)
	{
		ComponentID cId = ComponentID(ch);
		// picture description
		UInt uiStride = pcPredYuv->getStride(cId);									// stride for a certain component
		UInt uiPicWidth = pcPredYuv->getWidth(cId);									// picture width for a certain component
		UInt uiPicHeight = pcPredYuv->getHeight(cId);								// picture height for a certain component

		UInt uiCBWidth = uiMaxCUWidth >> (pcPredYuv->getComponentScaleX(cId));		// code block width for a certain component
		UInt uiCBHeight = uiMaxCUHeight >> (pcPredYuv->getComponentScaleY(cId));	// code block height for a certain component

		// rectangle of the code block
		UInt uiTopX = Clip3((UInt)0, uiPicWidth, uiCUPelX);
		UInt uiTopY = Clip3((UInt)0, uiPicHeight, uiCUPelY);
		UInt uiBottomX = Clip3((UInt)0, uiPicWidth, uiCUPelX + uiCBWidth);
		UInt uiBottomY = Clip3((UInt)0, uiPicHeight, uiCUPelY + uiCBHeight);

		for (UInt uiY = uiTopY; uiY < uiBottomY; uiY++)
		{
			for (UInt uiX = uiTopX; uiX < uiBottomX; uiX++)
			{
				UInt uiOrgX, uiOrgY;
				uiOrgX = g_auiRsmpldToOrg[cId][0][uiX];
				uiOrgY = g_auiRsmpldToOrg[cId][1][uiY];

				// template match
				UInt uiHashValue = getHashValue(uiOrgX, uiOrgY, uiPicWidth, ppPixel[cId]);
				Pixel* pCurPixel = ppPixel[cId] + getSerialIndex(uiOrgX, uiOrgY, uiPicWidth);
				pCurPixel->m_uiHashValue = uiHashValue;

				assert(uiHashValue >= 0 && uiHashValue < MAX_PT_NUM);
				PixelTemplate* pHashList = g_pPixelTemplate[cId][uiHashValue];	// hash list
				PixelTemplate* pBestMatch = NULL;								// best match template

				MatchMetric mmBestMetric;
				UInt uiListLength = 0;

				while (pHashList)
				{
					uiListLength++;

					UInt uiCX = pHashList->m_PX;
					UInt uiCY = pHashList->m_PY;

					if (!(ppPixel[cId] + getSerialIndex(uiCX, uiCY, uiPicWidth))->m_bIsRec)
					{
						pHashList = pHashList->m_pptNext;
						continue;
					}

					MatchMetric mmTmp;
					// match
					tryMatch(uiOrgX, uiOrgY, uiCX, uiCY, mmTmp, uiPicWidth, ppPixel[cId]);
					if (mmTmp.m_uiNumMatchPoints > mmBestMetric.m_uiNumMatchPoints)
					{
						mmBestMetric = mmTmp;
						pBestMatch = pHashList;
					}
					pHashList = pHashList->m_pptNext;
				}// end while

				if (pBestMatch)
				{
					UInt uiCX = pBestMatch->m_PX;
					UInt uiCY = pBestMatch->m_PY;
					if ((ppPixel[cId] + getSerialIndex(uiCX, uiCY, uiPicWidth))->m_bIsRec)
					{
						pCurPixel->m_mmMatch = mmBestMetric;
						pBestMatch->m_uiNumUsed++;
						Pixel* pRefPixel = ppPixel[cId] + getSerialIndex(mmBestMetric.m_uiX, mmBestMetric.m_uiY, uiPicWidth);
						pCurPixel->m_uiPred = pRefPixel->m_uiReco;								// prediction
						pCurPixel->m_iResi = pCurPixel->m_uiOrg - pCurPixel->m_uiPred;			// residue
					}
				}

				if (mmBestMetric.m_uiNumMatchPoints < 18 && uiListLength < uiPicWidth/10)
				{
					// insert new template
					PixelTemplate* pCurTemplate = new PixelTemplate(uiOrgX, uiOrgY);
					pCurTemplate->m_pptNext = g_pPixelTemplate[cId][uiHashValue];
					g_pPixelTemplate[cId][uiHashValue] = pCurTemplate;
					g_pPixelTemplatePool.push_back(pCurTemplate);
				}

				UInt uiIdx = uiY*uiStride + uiX;
				g_pcYuvPred->getAddr(cId)[uiIdx] = pcPredYuv->getAddr(cId)[uiIdx] = pCurPixel->m_uiPred;
				g_pcYuvResi->getAddr(cId)[uiIdx] = pcResiYuv->getAddr(cId)[uiIdx] = pCurPixel->m_iResi = pCurPixel->m_uiOrg - pCurPixel->m_uiPred;
				assert(pCurPixel->m_uiPred >= 0 && pCurPixel->m_uiPred <= 255);
			}// end for x
		}// end for y
	}// end for ch
}

Void updatePixelAfterCompressing(TComDataCU* pCtu, Pixel** ppPixel)
{
	// template matching
	UInt uiCUPelX = pCtu->getCUPelX();			// x of upper left corner of the cu
	UInt uiCUPelY = pCtu->getCUPelY();			// y of upper left corner of the

	UInt uiMaxCUWidth = pCtu->getSlice()->getSPS()->getMaxCUWidth();		// max cu width
	UInt uiMaxCUHeight = pCtu->getSlice()->getSPS()->getMaxCUHeight();		// max cu height

	// pic
	TComPic *pcPic = pCtu->getPic();
	TComPicYuv* pcPredYuv = pcPic->getPicYuvPred();
	TComPicYuv* pcResiYuv = pcPic->getPicYuvResi();
	TComPicYuv* pcRecoYuv = pcPic->getPicYuvRec();
	UInt uiNumValidCopmonent = pcPic->getNumberValidComponents();

	//fstream fReco;
	//fReco.open("reco.txt", ios::out);
	for (UInt ch = 0; ch < uiNumValidCopmonent; ch++)
	{
		ComponentID cId = ComponentID(ch);
		// picture description
		UInt uiStride = pcPredYuv->getStride(cId);									// stride for a certain component
		UInt uiPicWidth = pcPredYuv->getWidth(cId);									// picture width for a certain component
		UInt uiPicHeight = pcPredYuv->getHeight(cId);								// picture height for a certain component

		UInt uiCBWidth = uiMaxCUWidth >> (pcPredYuv->getComponentScaleX(cId));		// code block width for a certain component
		UInt uiCBHeight = uiMaxCUHeight >> (pcPredYuv->getComponentScaleY(cId));	// code block height for a certain component

		// rectangle of the code block
		UInt uiTopX = Clip3((UInt)0, uiPicWidth, uiCUPelX);
		UInt uiTopY = Clip3((UInt)0, uiPicHeight, uiCUPelY);
		UInt uiBottomX = Clip3((UInt)0, uiPicWidth, uiCUPelX + uiCBWidth);
		UInt uiBottomY = Clip3((UInt)0, uiPicHeight, uiCUPelY + uiCBHeight);

		//fReco << "=====> Channel:" << cId << endl;

		Pel* pBuffer = pcRecoYuv->getAddr(cId);
		for (UInt uiY = uiTopY; uiY < uiBottomY; uiY++)
		{
			for (UInt uiX = uiTopX; uiX < uiBottomX; uiX++)
			{
				UInt uiOrgX, uiOrgY;
				uiOrgX = g_auiRsmpldToOrg[cId][0][uiX];
				uiOrgY = g_auiRsmpldToOrg[cId][1][uiY];
				Pixel* pPixel = ppPixel[cId] + getSerialIndex(uiOrgX, uiOrgY, uiPicWidth);

				pPixel->m_bIsRec = true;
				pPixel->m_uiReco = *(pBuffer + uiY*uiStride + uiX);

				//fReco << pPixel->m_uiReco << "\t";
			}
			//fReco << endl;
		}
	}
	//fReco.close();
}

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

inline UInt getSerialIndex(UInt uiX, UInt uiY, UInt uiPicWidth)
{
	return (uiY + EXTEG)*(uiPicWidth + 2 * EXTEG) + uiX + EXTEG;
}

// get the 21 neighbors
Void getNeighbors(UInt uiX, UInt uiY, UInt uiPicWidth, Pixel* pPixel, Pixel* vTemplate)
{
	UInt uiIndex;
	// 1
	uiIndex = getSerialIndex(uiX - 1, uiY, uiPicWidth);
	vTemplate[0] = pPixel[uiIndex];
	// 2
	uiIndex = getSerialIndex(uiX - 1, uiY - 1, uiPicWidth);
	vTemplate[1] = pPixel[uiIndex];
	// 3
	uiIndex = getSerialIndex(uiX, uiY - 1, uiPicWidth);
	vTemplate[2] = pPixel[uiIndex];
	// 4
	uiIndex = getSerialIndex(uiX + 1, uiY - 1, uiPicWidth);
	vTemplate[3] = pPixel[uiIndex];
	// 5
	uiIndex = getSerialIndex(uiX + 2, uiY - 1, uiPicWidth);
	vTemplate[4] = pPixel[uiIndex];
	// 6
	uiIndex = getSerialIndex(uiX - 2, uiY, uiPicWidth);
	vTemplate[5] = pPixel[uiIndex];
	// 7
	uiIndex = getSerialIndex(uiX - 2, uiY - 1, uiPicWidth);
	vTemplate[6] = pPixel[uiIndex];
	// 8
	uiIndex = getSerialIndex(uiX - 2, uiY - 2, uiPicWidth);
	vTemplate[7] = pPixel[uiIndex];
	// 9
	uiIndex = getSerialIndex(uiX - 1, uiY - 2, uiPicWidth);
	vTemplate[8] = pPixel[uiIndex];
	// 10
	uiIndex = getSerialIndex(uiX, uiY - 2, uiPicWidth);
	vTemplate[9] = pPixel[uiIndex];
	// 11
	uiIndex = getSerialIndex(uiX + 1, uiY - 2, uiPicWidth);
	vTemplate[10] = pPixel[uiIndex];
	// 12
	uiIndex = getSerialIndex(uiX + 2, uiY - 2, uiPicWidth);
	vTemplate[11] = pPixel[uiIndex];
	// 13
	uiIndex = getSerialIndex(uiX - 3, uiY, uiPicWidth);
	vTemplate[12] = pPixel[uiIndex];
	// 14
	uiIndex = getSerialIndex(uiX - 3, uiY - 1, uiPicWidth);
	vTemplate[13] = pPixel[uiIndex];
	// 15
	uiIndex = getSerialIndex(uiX - 3, uiY - 2, uiPicWidth);
	vTemplate[14] = pPixel[uiIndex];
	// 16
	uiIndex = getSerialIndex(uiX - 3, uiY - 3, uiPicWidth);
	vTemplate[15] = pPixel[uiIndex];
	// 17
	uiIndex = getSerialIndex(uiX - 2, uiY - 3, uiPicWidth);
	vTemplate[16] = pPixel[uiIndex];
	// 18
	uiIndex = getSerialIndex(uiX - 1, uiY - 3, uiPicWidth);
	vTemplate[17] = pPixel[uiIndex];
	// 19
	uiIndex = getSerialIndex(uiX, uiY - 3, uiPicWidth);
	vTemplate[18] = pPixel[uiIndex];
	// 20
	uiIndex = getSerialIndex(uiX + 1, uiY - 3, uiPicWidth);
	vTemplate[19] = pPixel[uiIndex];
	// 21
	uiIndex = getSerialIndex(uiX + 2, uiY - 3, uiPicWidth);
	vTemplate[20] = pPixel[uiIndex];
}

// get 24bit hash value
UInt getHashValue(UInt uiX, UInt uiY, UInt uiPicWidth, Pixel* pPixel)
{
	Pixel vTemplate[21];
	// load template data
	getNeighbors(uiX, uiY, uiPicWidth, pPixel, vTemplate);

	// calculate hash value
	UInt uiTmp;
	UInt uiHashValue = 0;
	UInt mask = 0xE0;		// (1110 0000)b
	// 1
	uiTmp = 0;
	uiTmp += vTemplate[0].m_bIsRec ? vTemplate[0].m_uiReco : 0;
	uiHashValue |= ((uiTmp & mask) >> 5) << 21;
	// 2
	uiTmp = 0;
	uiTmp += vTemplate[1].m_bIsRec ? vTemplate[1].m_uiReco : 0;
	uiHashValue |= ((uiTmp & mask) >> 5) << 18;
	// 3
	uiTmp = 0;
	uiTmp += vTemplate[2].m_bIsRec ? vTemplate[2].m_uiReco : 0;
	uiHashValue |= ((uiTmp & mask) >> 5) << 15;
	// 4,5
	uiTmp = 0;
	uiTmp += vTemplate[3].m_bIsRec ? vTemplate[3].m_uiReco : 0;
	uiTmp += vTemplate[4].m_bIsRec ? vTemplate[4].m_uiReco : 0;
	uiTmp /= 2;
	uiHashValue |= ((uiTmp & mask) >> 5) << 12;
	// 6,7,13,14
	uiTmp = 0;
	uiTmp += vTemplate[5].m_bIsRec ? vTemplate[5].m_uiReco : 0;
	uiTmp += vTemplate[6].m_bIsRec ? vTemplate[6].m_uiReco : 0;
	uiTmp += vTemplate[12].m_bIsRec ? vTemplate[12].m_uiReco : 0;
	uiTmp += vTemplate[13].m_bIsRec ? vTemplate[13].m_uiReco : 0;
	uiTmp /= 4;
	uiHashValue |= ((uiTmp & mask) >> 5) << 9;
	// 8,15,16,17
	uiTmp = 0;
	uiTmp += vTemplate[7].m_bIsRec ? vTemplate[7].m_uiReco : 0;
	uiTmp += vTemplate[14].m_bIsRec ? vTemplate[14].m_uiReco : 0;
	uiTmp += vTemplate[15].m_bIsRec ? vTemplate[15].m_uiReco : 0;
	uiTmp += vTemplate[16].m_bIsRec ? vTemplate[16].m_uiReco : 0;
	uiTmp /= 4;
	uiHashValue |= ((uiTmp & mask) >> 5) << 6;
	// 9,10,18,19
	uiTmp = 0;
	uiTmp += vTemplate[8].m_bIsRec ? vTemplate[8].m_uiReco : 0;
	uiTmp += vTemplate[9].m_bIsRec ? vTemplate[9].m_uiReco : 0;
	uiTmp += vTemplate[17].m_bIsRec ? vTemplate[17].m_uiReco : 0;
	uiTmp += vTemplate[18].m_bIsRec ? vTemplate[18].m_uiReco : 0;
	uiTmp /= 4;
	uiHashValue |= ((uiTmp & mask) >> 5) << 3;
	// 11,12,20,21
	uiTmp = 0;
	uiTmp += vTemplate[10].m_bIsRec ? vTemplate[10].m_uiReco : 0;
	uiTmp += vTemplate[11].m_bIsRec ? vTemplate[11].m_uiReco : 0;
	uiTmp += vTemplate[19].m_bIsRec ? vTemplate[19].m_uiReco : 0;
	uiTmp += vTemplate[20].m_bIsRec ? vTemplate[20].m_uiReco : 0;
	uiTmp /= 4;
	uiHashValue |= ((uiTmp & mask) >> 5);

	return uiHashValue;
}

// 总误差的计算、匹配点数量的计算
Void tryMatch(UInt uiX, UInt uiY, UInt uiCX, UInt uiCY, MatchMetric &mmMatchMetric, UInt uiPicWidth, Pixel* pPixel)
{
	Pixel apNeighbor, apCNeighbor;
	// get the 21 neighbors
	//getNeighbors(uiX, uiY, uiPicWidth, pPixel, vTemplate);
	//getNeighbors(uiCX, uiCY, uiPicWidth, pPixel, vCTemplate);

	bool bContinue = true;
	UInt uiTotalAbsDiff = 0;
	UInt uiNumValidPoints = 0;
	UInt uiNumMatchPoints = 0;
	for (int i = 0; i < 21; i++)
	{
		UInt uiIndex;
		uiIndex = getSerialIndex(uiX + g_auiTemplateOffset[i][0], uiY + g_auiTemplateOffset[i][1], uiPicWidth);
		apNeighbor = pPixel[uiIndex];
		uiIndex = getSerialIndex(uiCX + g_auiTemplateOffset[i][0], uiCY + g_auiTemplateOffset[i][1], uiPicWidth);
		apCNeighbor = pPixel[uiIndex];

		if (apNeighbor.m_bIsRec)
		{
			uiNumValidPoints++;
			if (apCNeighbor.m_bIsRec)
			{
				UInt uiDiff = abs((int)(apNeighbor.m_uiReco - apCNeighbor.m_uiReco));
				uiTotalAbsDiff += uiDiff;
				if (uiDiff == 0)
					uiNumMatchPoints++;
			}
			else
			{
				uiTotalAbsDiff += 255;
			}
		}
		if (uiNumMatchPoints < i)
			break;
	}

	//if (uiNumMatchPoints < 3)
	//	bContinue = false;
	//for (int i = 3; i < 12 && bContinue; i++)
	//{
	//	if (vTemplate[i].m_bIsRec)
	//	{
	//		uiNumValidPoints++;
	//		if (vCTemplate[i].m_bIsRec)
	//		{
	//			UInt uiDiff = abs((int)(vTemplate[i].m_uiReco - vCTemplate[i].m_uiReco));
	//			uiTotalAbsDiff += uiDiff;
	//			if (uiDiff == 0)
	//				uiNumMatchPoints++;
	//		}
	//		else
	//		{
	//			uiTotalAbsDiff += 255;
	//		}
	//	}
	//}
	//if (uiNumMatchPoints < 8)
	//	bContinue = false;
	//for (int i = 12; i < 21 && bContinue; i++)
	//{
	//	if (vTemplate[i].m_bIsRec)
	//	{
	//		uiNumValidPoints++;
	//		if (vCTemplate[i].m_bIsRec)
	//		{
	//			UInt uiDiff = abs((int)(vTemplate[i].m_uiReco - vCTemplate[i].m_uiReco));
	//			uiTotalAbsDiff += uiDiff;
	//			if (uiDiff == 0)
	//				uiNumMatchPoints++;
	//		}
	//		else
	//		{
	//			uiTotalAbsDiff += 255;
	//		}
	//	}
	//}

	mmMatchMetric.m_uiX = uiCX;
	mmMatchMetric.m_uiY = uiCY;
	mmMatchMetric.m_uiAbsDiff = uiNumValidPoints == 0 ? 255 : (uiTotalAbsDiff / uiNumValidPoints);
	mmMatchMetric.m_uiNumValidPoints = uiNumValidPoints;
	mmMatchMetric.m_uiNumMatchPoints = uiNumMatchPoints;
}

Void appendNewTemplate(UInt uiHashValue, PixelTemplate*& rpNewTemplate)
{
}

int GetGroupID(int n) 
{
    assert(n>0);
    //only for n>0
    int k=2;
    while( n > (1<< k) - 2 )
    {
        k++;
    }
    return k-1;//1,2,3.....
}

int  UnaryEncode(int n,TCoeff *buffer,int &pos)
{
    int temp = n ;
    while( n>0) {
        WriteBit(1,buffer,pos);
        n--;
    }
    WriteBit(0,buffer,pos);
    return temp+1;
}

int  UnaryDecode(int *buffer,int &pos)
{
    int n = 0 ;
    while( ReadBit( buffer,pos ) == 1) {
        n++;
    }
    return n;
}

int ReadBits(int len,int *buffer,int &pos)
{
    int m,q,r;
    int  n=0;
    int result = 0;
    while( n < len ) {
        q = (pos) / GOLOMB_EXP_BUFFER;
        r = GOLOMB_EXP_BUFFER - 1 - (pos) % GOLOMB_EXP_BUFFER;
        m = ( *(buffer +q) ) >> r & 1;
        result = result *2 + m ;
        pos++;
        n++;
    }
    return result;
}

int ReadBit(int *buffer,int &pos)
{
    int m,q,r;    
    {
        q = (pos) / GOLOMB_EXP_BUFFER;
        r = GOLOMB_EXP_BUFFER - 1 - (pos) % GOLOMB_EXP_BUFFER;
        m = ( *(buffer +q) ) >> r & 1;        
        pos++;
    }
    return m;
}

Void WriteBits( TCoeff index, int num, TCoeff *buffer, int &pos)
{
    assert(num <= GOLOMB_EXP_BUFFER );
    while( num > 0 ) {
        WriteBit(index>>(num-1) & 1,buffer,pos) ;       
        num -- ;
    }
}

int ExpGolombEncode(int x,int *buffer,int &pos)
{
    int temp;
    
    if(x<=0) temp = abs(x)*2;
    else       temp = abs(x)*2 -1;
    if ( temp == 0 )  
    {
        WriteBit(0,buffer,pos);
        return 1;
    }else
    {        
        int GroupID = GetGroupID(temp);
        int bits =UnaryEncode(GroupID,buffer,pos);
        UInt  Index = temp - ( (1 << GroupID) - 1 ) ;
        WriteBits( Index, GroupID , buffer , pos );
        return bits + GroupID ;
    }
    
}
  
int ExpGolombDecode(int *buffer,int &pos)
{
    int result;
    int GroupID = UnaryDecode(buffer,pos) ;
     if( GroupID == 0) {
         result = 0;
     } else {
         int Base = ( 1<< GroupID) -1 ;
         int Index = ReadBits(GroupID,buffer,pos);
         result = (Base + Index) ;
     }
     if( result % 2 ==0  )  
         return -result/2 ;
     else
         return ( result + 1 )/2;
}


Void WriteBit(int n,TCoeff *buffer,int &pos) 
{   
    assert( n==1 || n ==0);
    int  q = (pos) / GOLOMB_EXP_BUFFER;
    int   r = GOLOMB_EXP_BUFFER - 1 - (pos) % GOLOMB_EXP_BUFFER;
    *(buffer +q) &= ~(1<< r );
    *(buffer +q) |= n<< r ;
    
    pos++;
}



int getG0Bits(int x)
{
	int bits = 0;
	if (abs(x)==0) bits=1;
	else if (abs(x)<=1) bits=3;
	else if (abs(x)<=3) bits=5;
	else if (abs(x)<=7) bits=7;
	else if (abs(x)<=15) bits=9;
	else if (abs(x)<=31) bits=11;
	else if (abs(x)<=63) bits=13;
	else if (abs(x)<=127) bits=15;
    else if (abs(x)<=255) bits=17;
	else bits=19;
	return bits;
}

int  PaletteIndex(TComDataCU *pcCU,ComponentID compID,Pel pel )
{
    int distance = MAX_INT;
    int index = -1 ;
    int temp;
    Pel * palette = pcCU->getCtuPalette(compID);
    for( int i=0 ; i< INTRA_PR_PALETTE_NUM ; i++)
    {
         temp = /*getG0Bits(i)+ */getG0Bits( pel-palette[i]  );
        if ( temp < distance)
        {
            distance = temp ;
            index = i ;
        }        
    }
    //assert(index>-1 && index< INTRA_PR_MOSTVALUE_NUM);
    return index;
}


UInt  PositionCode_PathLeast(TComYuv *pcResiYuv,TComTU&     rTu,const ComponentID compID,TCoeff* pcCoeff  )
{      
        if (!rTu.ProcessComponentSection(compID)) return -1;
        const Bool       bIsLuma = isLuma(compID);
        const TComRectangle &rect= rTu.getRect(compID);
        TComDataCU *pcCU=rTu.getCU();
        const UInt uiAbsPartIdx=rTu.GetAbsPartIdxTU();

        const UInt uiTrDepth=rTu.GetTransformDepthRelAdj(compID);
        const UInt uiFullDepth   = rTu.GetTransformDepthTotal();
        const UInt uiLog2TrSize  = rTu.GetLog2LumaTrSize();
        const ChromaFormat chFmt = pcCU->getPic()->getChromaFormat();
        const ChannelType chType = toChannelType(compID);

        const UInt    uiWidth           = rect.width;
        const UInt    uiHeight          = rect.height;
        const UInt    uiStride          = pcResiYuv->getStride (compID);
        Pel*    piResi            = pcResiYuv->getAddr( compID, uiAbsPartIdx );
        Pel*    pResi   = piResi;
        TComPicYuv  *pcPicYuv = pcCU->getPic()->getPicYuvOrg();
        Pel*  piOrg = pcPicYuv->getAddr(compID, pcCU->getCtuRsAddr(),pcCU->getZorderIdxInCtu()+ uiAbsPartIdx );
        const UInt   uiOrgStride = pcPicYuv->getStride(compID);

        TCoeff  *bitsbuffer = pcCoeff;//pcCU->getPRBits(compID) + rTu.getCoefficientOffset(compID);
        //const UInt componentShift = pcCU->getPic()->getComponentScaleX(ComponentID(compID)) + pcCU->getPic()->getComponentScaleY(ComponentID(compID));
        //memset(bitsbuffer,0, sizeof(int)* ( (uiWidth * uiHeight)>>componentShift ) );

       int  indexlist[4096][4];
        memset(indexlist,0,sizeof(int)*4096*4);
        int srcindex=0;

		int contz = 0;
		int contnz = 0;
		int cs = 0;
		int bits = 0;
        int pbits = 0 ;
        int rbits = 0 ;
        int temp;
		int lx = 0; int ly = 0; int lv = 0;
		int hasleft = 1;
  //      Pel  pel;
        Pel *palette = pcCU->getCtuPalette(compID);
		while (hasleft) {
			int ndis = 1000;
			int nx = -1; int ny = -1;
            pResi = piResi ;
			for (UInt y=0; y<uiHeight; y++) {
				for (UInt x=0; x<uiWidth; x++) {
                    assert( pResi[x] >-256 && pResi[x]<256);
					if (pResi[x]!=0) {
						int dis = 0;
						dis+=getG0Bits(x-lx);
						dis+=getG0Bits(y-ly);
						if (dis < ndis) {
							nx = x;
							ny = y;
                            ndis = dis ;
						}
					}
				}
                pResi += uiStride;
			}
			if (nx!=-1 && ny!=-1) {

                        temp = *(piOrg + ny* uiOrgStride+nx ) ;
                        temp = ::PaletteIndex(pcCU,compID, *(piOrg + ny* uiOrgStride+nx ) );
                        indexlist[srcindex][0] = nx-lx;
                        indexlist[srcindex][1] = ny-ly;
                        indexlist[srcindex][2] = temp;
                        indexlist[srcindex][3] = *(piOrg + ny* uiOrgStride+nx ) - palette[temp] ;
                        lv = indexlist[srcindex][3];
                        srcindex++;

    				    lx = nx; ly = ny; 
				        *(piResi +ny*uiStride+nx) =0;
			} else {
				hasleft = 0;
			}
		}
            ExpGolombEncode(srcindex,bitsbuffer,bits);
            for(int i =0  ; i< srcindex ; i++)  {
                ExpGolombEncode(indexlist[i][0],bitsbuffer,bits);
                ExpGolombEncode(indexlist[i][1],bitsbuffer,bits);
                ExpGolombEncode(indexlist[i][2],bitsbuffer,bits);
           //     if(!bNoResidual)
            //    ExpGolombEncode(indexlist[i][3],bitsbuffer,bits);
            }
        
        
		return bits ;


}

int compare4 (const void * a, const void * b)
{    
  return ( *((int*)a+3) - *((int*)b+3) );
}
int compare5 (const void * a, const void * b)
{    
  return ( *((int*)a+4) - *((int*)b+4) );
}
//index== 0 above 1 left 2 above and left 3 above and right
Int  GolombCode_Predict_SingleNeighbor(TComYuv *pcResiYuv,TComTU&     rTu,const ComponentID compID,UInt uiCUHandleAddr,UInt uiAIndex, TCoeff* pcCoeff )
{           
        const Bool       bIsLuma = isLuma(compID);
        const TComRectangle &rect= rTu.getRect(compID);
        TComDataCU *pcCU=rTu.getCU();
        UInt  uiCUAddr = pcCU->getCtuRsAddr() ;
 
        //if  ((int)uiCUHandleAddr < 0)     return -1;

        TComPicYuv *pcPicYuvResi = g_pcYuvResi;
        if(pcPicYuvResi == NULL)  return -1;
        const UInt uiAbsPartIdx = rTu.GetAbsPartIdxTU();
    //    const UInt  uiZOrder      = pcCU->getZorderIdxInCU() +uiAbsPartIdx;

        const UInt uiTrDepth=rTu.GetTransformDepthRelAdj(compID);
        const UInt uiFullDepth   = rTu.GetTransformDepthTotal();
        const UInt uiLog2TrSize  = rTu.GetLog2LumaTrSize();
        const ChromaFormat chFmt = pcCU->getPic()->getChromaFormat();
       

        const UInt    uiWidth          = rect.width;
        const UInt    uiHeight         = rect.height;
        const UInt    uiStride          = pcResiYuv->getStride (compID);

        
        UInt   uiAddr = pcCU->getCtuRsAddr() ;

        TComYuv  *pcTemp;
        pcTemp =  new TComYuv;
        UInt  uiSrc1Stride = pcPicYuvResi->getStride(compID);
        UInt  CUPelX,CUPelY;
        CUPelX           = ( uiCUHandleAddr % pcCU->getPic()->getFrameWidthInCtus() ) * g_uiMaxCUWidth;
        CUPelY           = ( uiCUHandleAddr /   pcCU->getPic()->getFrameWidthInCtus() ) * g_uiMaxCUHeight;
        CUPelX= CUPelX +  g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
        CUPelY= CUPelY +   g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ]; 
        //for(int m=0;m<256;m++) cout<<g_auiZscanToRaster[m] <<" ";cout<<endl;
        //for(int m=0;m<256;m++) cout<<g_auiRasterToPelX[m] <<" ";cout<<endl;
        //for(int m=0;m<256;m++) cout<<g_auiRasterToPelY[m] <<" ";cout<<endl;
        //Pel *pSrc1 = pcPicYuvResi->getAddr(compID) +CUPelY * uiSrc1Stride + CUPelX;
        Pel  *pSrc1= pcPicYuvResi->getAddr(compID, uiCUHandleAddr, uiAbsPartIdx + pcCU->getZorderIdxInCtu());
 /*       if( compID != COMPONENT_Y)
        {
            pSrc1 = pcPicYuvResi->getAddr(COMPONENT_Y, uiCUHandleAddr, uiAbsPartIdx + pcCU->getZorderIdxInCU());
        }*/
        pcTemp->create(uiWidth,uiHeight,chFmt);
 //       pcTemp->copyFromPicComponent(compID,pcPicYuvResi,uiCUHandleAddr, pcCU->getZorderIdxInCU()+uiAbsPartIdx);

         UInt  uiTempStride = pcTemp->getStride(compID);
         Pel  *pTemp =  pcTemp->getAddr(compID);
        for (Int y =  0; y < uiHeight ; y++ )
        {
            for(Int x=0;x < uiWidth ; x++)
            {
                pTemp[x] = pSrc1[x] ;                    
            }
            pTemp += uiTempStride;
            pSrc1 += uiSrc1Stride;                
        }

		int srclx = 0; int srcly = 0; int srclv = 0;
		int srchasleft = 1;
        Pel  srcpel;
        int  srclist[3][64*64];
        int  srcindex = 0 ;
        memset(srclist,-1,3*64*64*sizeof(int));
        int cursrclistindex = 0;
        
        Pel*  piSrc     = pcTemp->getAddr(compID);
        //Pel*  piSrc     = pcTemp->getAddr(compID, uiAbsPartIdx);
        Pel*  pSrc      = piSrc ;
        //found the source list
        while (srchasleft) {
			int ndis = 1000;
			int nx = -1; int ny = -1;
            pSrc = piSrc ;
			for (UInt y=0; y<uiHeight; y++) {
				for (UInt x=0; x<uiWidth; x++) {
                    assert( pSrc[x] >-256 && pSrc[x]< 256 );
					if (pSrc[x]!=0) {
						int dis = 0;
						dis+=getG0Bits( (x-srclx));
						dis+=getG0Bits( (y-srcly));
						if (dis < ndis) {
							nx = x;
							ny = y;
                            ndis = dis ;
						}
					}
				}
                pSrc += uiTempStride;
			}
            if(nx!=-1 && ny!=-1) {
                srcpel =  *(piSrc +ny*uiTempStride+nx) ;	 
				srclx = nx; srcly = ny; srclv = srcpel;
                srclist[0][srcindex] = srclx ;
                srclist[1][srcindex] = srcly ;
                srclist[2][srcindex] = srcpel;
                srcindex++;
			    *(piSrc +ny*uiTempStride+nx) =0;
            }else {
                srchasleft = 0;
            }
        }
        if(srcindex==0)  {
            pcTemp->destroy();
            delete pcTemp;
            pcTemp = NULL;
            return -1;
        }
        ////
        TComPicYuv *pcPicOrg = pcCU->getPic()->getPicYuvOrg();
        Pel*  piOrg = pcPicOrg->getAddr(compID, pcCU->getCtuRsAddr(), pcCU->getZorderIdxInCtu()+ uiAbsPartIdx );
        const UInt   uiOrgStride = pcPicOrg->getStride(compID);
        ////
        Pel*  piResi   = pcResiYuv->getAddr( compID, uiAbsPartIdx );
        Pel*  pResi    = piResi;
        int dstindex=0;
        int  indexlist[64*64][5];
        memset(indexlist,0,5*64*64*sizeof(int));
		int contz = 0;
		int contnz = 0;
		int cs = 0;
		int bits = 0;
//        int temp;
		int lx = 0; int ly = 0; int lv = 0;
		int hasleft = 1;
        Pel  pel;
        Pel *palette = pcCU->getCtuPalette(compID);
		while (hasleft) {
            //found the least distance point
			int ndis = 1000;
			int nx = -1; int ny = -1;
            pResi = piResi ;
			for (UInt y=0; y<uiHeight; y++) {
				for (UInt x=0; x<uiWidth; x++) {
					if (pResi[x]!=0) {
						int dis = 0;
						dis+=getG0Bits( (x-lx));
						dis+=getG0Bits( (y-ly));
						if (dis < ndis) {
							nx = x;
							ny = y;
                            ndis = dis ;
						}
					}
				}
                pResi += uiStride;
			}
			if (nx!=-1 && ny!=-1) { 
                    pel =  *(piResi +ny*uiStride+nx) ;

                    int srcdis = 1024*4;
                    int srccur = -1;
                    for(UInt s=0; s< srcindex; s++) {
                           int curdis = 0;
                           
                           curdis += getG0Bits( (nx-srclist[0][s]));
                           curdis += getG0Bits( (ny-srclist[1][s]));
 //                          curdis += getG0Bits( (pel-srclist[2][s]));// getG0Bits can handle -512 && 512

                           if(curdis <srcdis) {
                               srccur = s ;
                               srcdis = curdis;
                           }
                    }
                    if( srccur !=-1) {
                        indexlist[dstindex][0] = nx-srclist[0][srccur] ;
                        indexlist[dstindex][1] = ny-srclist[1][srccur] ;
                        assert(pel !=0);
                           
                            indexlist[dstindex] [2] = ::PaletteIndex(pcCU,compID, *(piOrg +ny*uiOrgStride+nx) );
                            indexlist[dstindex] [3] = *(piOrg +ny*uiOrgStride+nx) - palette[ indexlist[dstindex] [2] ];

                            
                            indexlist[dstindex][4] = srccur ;
                       
                        dstindex++;
                        cursrclistindex = srccur ;
                    } else {
                        assert(true);
                    }
				    lx = nx; ly = ny; lv = pel;
				     *(piResi +ny*uiStride+nx) =0;
			} else {
				hasleft = 0;
			}
		}

        pcTemp->destroy();
        delete pcTemp;
        pcTemp = NULL;

        if(dstindex==0) {
            assert(bits==0);
            return bits;
        }
        qsort(indexlist,dstindex,sizeof(int)*5,compare5);
        
        for(UInt x=dstindex-1;x>0;x--) {
            indexlist[x][4] -= indexlist[x-1][4] ;            
        }

        //bits += getG0Bits( (indexlist[0][0]));
        //bits += getG0Bits( (indexlist[0][1]));

        int maxlength = 0 ;
        int truebits = 0;
        bool  vlcf[3] = {false,false,false};// dx & dy   residual  srcindex
        int z01 = 0 ;
        for(UInt x=1; x<dstindex; x++) {
            if( indexlist[x][0]== indexlist[x-1][0] &&indexlist[x][1]== indexlist[x-1][1]) {
                maxlength++;
            }
            else {
                bits += getG0Bits( (maxlength));
                bits += getG0Bits( (indexlist[x-1][0]));
                bits += getG0Bits( (indexlist[x-1][1]));
                maxlength = 0;
            }
            if(indexlist[x][0] == 0 && indexlist[x][1] == 0 ) z01++;

        }

        bits += getG0Bits( (maxlength));
        bits += getG0Bits( (indexlist[dstindex-1][0]));
        bits += getG0Bits( (indexlist[dstindex-1][1]));
        UInt sbits = 0;
        for(UInt x=0;x<dstindex;x++) {
            sbits += getG0Bits( (indexlist[x][0]));
            sbits += getG0Bits( (indexlist[x][1]));
        }
     //   printf("gain %6d position before %6d after %6d\n",sbits-bits,sbits,bits);

        if(sbits<bits)  { 
            vlcf[0] = true;
            bits= sbits ;
        }
        
        sbits = bits +1;
        //bits += getG0Bits( PTable(compID,indexlist[0][2]));
         {
            maxlength = 0 ;
            for(UInt x=1; x<dstindex; x++) {
                if( indexlist[x][2]== indexlist[x-1][2] && indexlist[x][3]==indexlist[x-1][3] ) {
                    maxlength++;
                }
                else {
                    bits += getG0Bits( (maxlength));
                    bits += getG0Bits( indexlist[x-1][3] );
                    
                    bits += getG0Bits( indexlist[x-1][2] );
                    maxlength = 0;
                }
            }        
            bits += getG0Bits( (maxlength));            
            bits += getG0Bits( indexlist[dstindex-1][3] );            
            bits += getG0Bits( (indexlist[dstindex-1][2]));
        } 
        UInt vbits=0;
        sbits = bits - sbits;
        for(UInt x=0;x<dstindex;x++) {
             {
                vbits += getG0Bits( indexlist[x][2] );
                vbits += getG0Bits( indexlist[x][3] );
                
            }
        }
//        printf("gain %6d delta resi before %6d after %6d\n",vbits-sbits,vbits,sbits);
        if(vbits<sbits) {
            vlcf[1] = true;
            bits = bits -sbits + vbits  ;
        }
        sbits = bits + 1;
        //bits += getG0Bits( (indexlist[0][3]));
        Int srcPosIndex;
         
            srcPosIndex = 4 ;
        
        maxlength=0;

        for(UInt x=1; x<dstindex; x++) {
            if( indexlist[x][srcPosIndex]== indexlist[x-1][srcPosIndex] ) {
                maxlength++;
            }
            else {
                bits += getG0Bits( (maxlength));
                bits += getG0Bits( (indexlist[x-1][srcPosIndex]));
                maxlength = 0;
            }            
        }

        bits += getG0Bits( (maxlength));
        bits += getG0Bits( (indexlist[dstindex-1][srcPosIndex]));
             
        sbits = bits - sbits;
        vbits = 0 ;
        for(UInt x=0;x<dstindex;x++) {            
            vbits += getG0Bits( (indexlist[x][srcPosIndex]));
        }
//        printf("gain %6d delta index before %6d after %6d\n",vbits-sbits,vbits,sbits);
        if(vbits<sbits) {
            vlcf[2] = true;
            bits = bits -sbits + vbits  ;
        }
//#if INTRA_PR_DEBUG
//        if( pcCU->getAddr()==INTRA_PR_CUNUM )
//            printf("position distance zero  %6d of %6d  total bits %6d\n",z01,dstindex,bits+1);
//#endif
        truebits = 0 ;
        ExpGolombEncode(uiAIndex,pcCoeff,truebits);
        //--------------encode  srcindex

        if(vlcf[2]) {
            ExpGolombEncode(0,pcCoeff,truebits);
            for(UInt x=0;x<dstindex;x++) {
                //cout<<"  "<<indexlist[x][3]  ;
                ExpGolombEncode( (indexlist[x][srcPosIndex]),pcCoeff,truebits);
            }            
        }
        else{
            ExpGolombEncode(1,pcCoeff,truebits);
            maxlength=0;
            for(UInt x=1; x<dstindex; x++) {
                if( indexlist[x][srcPosIndex]== indexlist[x-1][srcPosIndex] ) {
                    maxlength++;
                }
                else {
                    ExpGolombEncode( (maxlength),pcCoeff,truebits);
                    ExpGolombEncode( (indexlist[x-1][srcPosIndex]),pcCoeff,truebits);
                    maxlength = 0;
                }            
            }
            assert( maxlength>-1) ;
            ExpGolombEncode( (maxlength),pcCoeff,truebits);
            ExpGolombEncode( (indexlist[dstindex-1][srcPosIndex]),pcCoeff,truebits);
        }
        ExpGolombEncode(-1,pcCoeff,truebits);
        //---------------encode  residual
        
         if(vlcf[1]) {
            ExpGolombEncode(0,pcCoeff,truebits);
            for(UInt x=0;x<dstindex;x++) {  
                 {
                    ExpGolombEncode( indexlist[x][2],pcCoeff,truebits);
                  //  if( !bNoResidual) 
                  //      ExpGolombEncode( indexlist[x][3],pcCoeff,truebits);
                    
                }
            }            
        }
        else{
            ExpGolombEncode(1,pcCoeff,truebits);
            //ExpGolombEncode( (indexlist[0][2]),pcCoeff,truebits);
            maxlength=0;
            {
                for(UInt x=1; x<dstindex; x++) {
                    if( indexlist[x][2]== indexlist[x-1][2] && indexlist[x][3]== indexlist[x-1][3] ) {
                        maxlength++;
                    }
                    else {
                        ExpGolombEncode( (maxlength),pcCoeff,truebits);
                        //assert( (maxlength)>=0);
                        ExpGolombEncode( indexlist[x-1][2],pcCoeff,truebits);
                     //   if( !bNoResidual) 
                     //       ExpGolombEncode( indexlist[x-1][3],pcCoeff,truebits);
                        
                        maxlength = 0;
                    }            
                }
                ExpGolombEncode( (maxlength),pcCoeff,truebits);
                ExpGolombEncode( indexlist[dstindex-1][2] ,pcCoeff,truebits);
                //if( !bNoResidual) 
                //    ExpGolombEncode( indexlist[dstindex-1][3] , pcCoeff, truebits);
            } 
        }
        //--------------encode dx and dy -----------
        if(vlcf[0])  {           
            ExpGolombEncode(0,pcCoeff,truebits);
            for(UInt x=0;x<dstindex;x++) {
              ExpGolombEncode( (indexlist[x][0]),pcCoeff,truebits);
              ExpGolombEncode( (indexlist[x][1]),pcCoeff,truebits);
            }
        }
        else{
            ExpGolombEncode(1,pcCoeff,truebits);
            //ExpGolombEncode( (indexlist[0][0]),pcCoeff,truebits);
            //ExpGolombEncode( (indexlist[0][1]),pcCoeff,truebits);

            maxlength = 0 ;

            for(UInt x=1; x<dstindex; x++) {
                if( indexlist[x][0]== indexlist[x-1][0] &&indexlist[x][1]== indexlist[x-1][1]) {
                    maxlength++;
                }
                else {
                    ExpGolombEncode( (maxlength),pcCoeff,truebits);
                    ExpGolombEncode( (indexlist[x-1][0]),pcCoeff,truebits);
                    ExpGolombEncode( (indexlist[x-1][1]),pcCoeff,truebits);
                    maxlength = 0;
                }   
            }
            ExpGolombEncode( (maxlength),pcCoeff,truebits);
            ExpGolombEncode( (indexlist[dstindex-1][0]),pcCoeff,truebits);
            ExpGolombEncode( (indexlist[dstindex-1][1]),pcCoeff,truebits);
        }
		return   truebits;/* bits +1*/;

}


UInt  PositionCode_All(TComYuv *pcResiYuv,TComTU&     rTu,const ComponentID compID , TCoeff* pcCoeff,Bool bY/*,Bool bMV,bool bNoResidual*/ )
{
    if (!rTu.ProcessComponentSection(compID)) return -1;
        const Bool       bIsLuma = isLuma(compID);
        const TComRectangle &rect= rTu.getRect(compID);
        TComDataCU *pcCU=rTu.getCU();
        UInt  uiCUAddr = pcCU->getCtuRsAddr() ;
        UInt  uiFrameWidthInCU =  pcCU->getPic()->getFrameWidthInCtus() ;
        UInt  uiCUAboveAddr = uiCUAddr - uiFrameWidthInCU;
        UInt  uiCULeftAddr = uiCUAddr - 1 ;
        UInt  uiCUAboveLeftAddr   = uiCUAddr -1 - uiFrameWidthInCU;
        UInt  uiCUAboveRightAddr = uiCUAddr +1 - uiFrameWidthInCU;
        UInt  uiCUArray[4] = {uiCULeftAddr,uiCUAboveAddr,uiCUAboveLeftAddr,uiCUAboveRightAddr};
        UInt  uiCUHandleAddr ;
  
        TComPicYuv *pcPicYuvResi = g_pcYuvResi;
        if(pcPicYuvResi == NULL)  return -1;
        const UInt uiAbsPartIdx=rTu.GetAbsPartIdxTU();
        const UInt  uiZOrder      = pcCU->getZorderIdxInCtu() +uiAbsPartIdx;
        const UInt uiTrDepth=rTu.GetTransformDepthRelAdj(compID);
        const UInt uiFullDepth   = rTu.GetTransformDepthTotal();
        const UInt uiLog2TrSize  = rTu.GetLog2LumaTrSize();
        const ChromaFormat chFmt = pcCU->getPic()->getChromaFormat();
        const ChannelType chType = toChannelType(compID);

        const UInt    uiWidth          = rect.width;
        const UInt    uiHeight         = rect.height;
    
        const UInt   uiSrcStride      = pcResiYuv->getStride(compID) ;
   
        TComYuv  *pcTempA= new TComYuv;

        pcTempA->create( uiWidth , uiHeight, chFmt) ;
        const UInt    uiAStride      = pcTempA->getStride (compID);
        const UInt    uiResiStride =  pcPicYuvResi->getStride(compID);
        UInt  uiCUPelX,uiCUPelY;
        uiCUPelX= pcCU->getCUPelX() +  g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
        uiCUPelY= pcCU->getCUPelY() +  g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ]; 
        //cout<<compID<<" "<<uiCUPelX<<" "<<uiCUPelY<<" "<<uiAbsPartIdx<<" "<<uiWidth<<" "<<uiHeight<<endl;
         if( bY && compID!=COMPONENT_Y) {
             UInt  uiSrc1Stride = pcPicYuvResi->getStride(COMPONENT_Y);
             Pel  *pSrc1 = pcPicYuvResi->getAddr(COMPONENT_Y) +uiCUPelY * uiSrc1Stride + uiCUPelX;
             Pel  *pDest =  pcTempA->getAddr(compID);
             
            for (Int y =  0; y < uiHeight; y++ )
            {
                for(Int x=0;x < uiWidth ; x++)
                {
                    assert(pSrc1[x]>-256&& pSrc1[x]<256);
                    pDest[x] = pSrc1[x] ;                    
                }
                pDest += uiAStride;
                pSrc1 += uiSrc1Stride;                
            }
        }
        else{

            for(UInt  index =0 ; index< 4; index++) {
                if( (int) uiCUArray[index]<0) continue;
                 uiCUHandleAddr = uiCUArray[index] ;
            
                //uiCUPelX           = ( uiCUHandleAddr % pcCU->getPic()->getFrameWidthInCU() ) * g_uiMaxCUWidth;
                //uiCUPelY           = ( uiCUHandleAddr /   pcCU->getPic()->getFrameWidthInCU() ) * g_uiMaxCUHeight;
                //uiCUPelX= uiCUPelX +  g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
                //uiCUPelY= uiCUPelY +  g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];             
                // 
                // Pel *pSrc1 = pcPicYuvResi->getAddr(compID) +uiCUPelY * uiResiStride + uiCUPelX;
                 Pel *pSrc1 = pcPicYuvResi->getAddr(compID, uiCUHandleAddr,uiAbsPartIdx + pcCU->getZorderIdxInCtu());
                 Pel  *pDest =  pcTempA->getAddr(compID);
                for (Int y =  0; y < uiHeight; y++ )
                {
                    for(Int x=0;x < uiWidth ; x++)
                    {
                        assert(*(pSrc1+x)>-256&&*(pSrc1+x)<256);
                        *(pDest+x) = *(pSrc1+x) ;                    
                    }
                    pDest += uiAStride;
                    pSrc1 += uiResiStride;                
                }
         
            }
        }
        TComPicYuv  * pcPicYuvOrg= pcCU->getPic()->getPicYuvOrg() ;
        Pel*  piOrg = pcPicYuvOrg->getAddr(compID, pcCU->getCtuRsAddr(), pcCU->getZorderIdxInCtu()+ uiAbsPartIdx );
        const UInt   uiOrgStride = pcPicYuvOrg->getStride(compID);
        Pel srcpel;
        Pel*  piResi   = pcResiYuv->getAddr( compID, uiAbsPartIdx );
        Pel*  pResi    = piResi;
        Pel *palette = pcCU->getCtuPalette(compID);
        int temp;
        int  bitsnt;
		int bits = 0;
        int  truebits = 0;
        int  tempanum=0;
        for (UInt y=0; y<uiHeight; y++) {
             for (UInt x=0; x<uiWidth; x++) {
                    if( *(pcTempA->getAddr(compID)+y*uiAStride+x) ) {
                        srcpel =  *(piResi +y*uiSrcStride+x) ;	
                        //assert(srcpel>-256 && srcpel <256);
                        bitsnt = truebits ;

                        {  
                            if( srcpel!=0) {
                                temp =  *(piOrg + y* uiOrgStride+x ) ;
                                temp = ::PaletteIndex(pcCU,compID, *(piOrg + y* uiOrgStride+x ) );
                                bits+= ExpGolombEncode(temp,pcCoeff,truebits);
                                temp = *(piOrg + y* uiOrgStride+x ) - palette[temp];
#if LOSSY
                                temp = QLossyValue(temp);
#endif
                                //if(!bNoResidual)
                                //bits+= ExpGolombEncode(temp,pcCoeff,truebits);
                            }else {
                                bits+=ExpGolombEncode(-1,pcCoeff,truebits);
                            }
                        }

                        //cout<< x << " "<< y <<" "<<*(pcTempA->getAddr(compID)+y*uiAStride+x)<<" "<< *(piResi +y*uiSrcStride+x) <<" "<<*(piOrg + y* uiOrgStride+x )<<endl;
                        tempanum++;
                        *(piResi +y*uiSrcStride+x) = 0;
                    }
		      }
              
	        }
		int contz = 0;
		int contnz = 0;
		int cs = 0;
        int srclx = 0; int srcly = 0; int srclv = 0;
		int srchasleft = 1;
        ////
        uiCUPelX= pcCU->getCUPelX() +  g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
        uiCUPelY= pcCU->getCUPelY() +  g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ]; 
        ////
        int srcindex = 0 ;
        Pel* piSrc   = pcResiYuv->getAddr( compID, uiAbsPartIdx );
        Pel* pSrc;
        int  indexlist[4096][4];
        memset(indexlist,0,sizeof(int)*4096*4);
        //found the source list
        while (srchasleft) {
			int ndis = 1000;
			int nx = -1; int ny = -1;
            pSrc = piSrc ;
			for (UInt y=0; y<uiHeight; y++) {
				for (UInt x=0; x<uiWidth; x++) {
					if (pSrc[x]!=0) {
						int dis = 0;
						dis+=getG0Bits( (x-srclx));
						dis+=getG0Bits( (y-srcly));
						if (dis < ndis) {
							nx = x;
							ny = y;
                            ndis = dis ;
						}
					}
				}
                pSrc += uiSrcStride;
			}
            if(nx!=-1 && ny!=-1) {
                srcpel =  *(piResi +ny*uiSrcStride+nx) ;				

                
                { 
                    temp = *(piOrg + ny* uiOrgStride+nx ) ;
                    temp = ::PaletteIndex(pcCU,compID, *(piOrg + ny* uiOrgStride+nx ) );
                    indexlist[srcindex][0] = nx-srclx;
                    indexlist[srcindex][1] = ny-srcly;
                    indexlist[srcindex][2] = temp;
                    indexlist[srcindex][3] = *(piOrg + ny* uiOrgStride+nx ) - palette[temp] ;

                }
                      
                
			    *(piResi +ny*uiSrcStride+nx) =0;
                srcindex ++;
                srclx = nx; srcly = ny; srclv = srcpel;
            }else {
                srchasleft = 0;
            }
        }

        pcTempA->destroy();
        delete pcTempA;
        pcTempA = NULL ;



         {
            ExpGolombEncode(srcindex,pcCoeff,truebits);
            for(int i =0  ; i< srcindex ; i++)  {
                ExpGolombEncode(indexlist[i][0],pcCoeff,truebits);
                ExpGolombEncode(indexlist[i][1],pcCoeff,truebits);
                ExpGolombEncode(indexlist[i][2],pcCoeff,truebits);
               // if(!bNoResidual)
              //  ExpGolombEncode(indexlist[i][3],pcCoeff,truebits);
            }
        }
		return truebits;


}
Int compareint (const void * a, const void * b)
{
  return ( *(Int*)a - *(Int*)b );
}

UInt  GolombCode_Predict(TComYuv *pcResiYuv,TComTU&     rTu,const ComponentID compID, TCoeff* pcCoeff ,Bool bMV,bool bNoResidual)
{
    if (!rTu.ProcessComponentSection(compID)) return -1;
    
    TComDataCU *pcCU=rTu.getCU();
    UInt  uiCUAddr = pcCU->getCtuRsAddr() ;
    UInt  uiFrameWidthInCU =  pcCU->getPic()->getFrameWidthInCtus() ;
    UInt  uiCUAboveAddr = uiCUAddr - uiFrameWidthInCU;
    UInt  uiCULeftAddr = uiCUAddr - 1 ;
    UInt  uiCUAboveLeftAddr   = uiCUAddr -1 - uiFrameWidthInCU;
    UInt  uiCUAboveRightAddr = uiCUAddr +1 - uiFrameWidthInCU;
    UInt  uiCUArray[4] = {uiCULeftAddr,uiCUAboveAddr,uiCUAboveLeftAddr,uiCUAboveRightAddr};
    UInt  uiCUHandleAddr ;
    Int curindex,minibits,temp/*,truebits*/ ;
    TCoeff  pcCoeffTemp[MAX_BUFFER ];
    TComYuv *pcComYuv = new TComYuv;
    const TComRectangle &rect= rTu.getRect(compID);
      UInt uiTempStride,uiSrcStride;
      
      
      uiSrcStride = pcResiYuv->getStride(compID);
      
      UInt uiHeight = pcResiYuv->getHeight(compID);
      UInt uiWidth =  pcResiYuv->getWidth(compID);
      pcComYuv->create(uiWidth,uiHeight, pcCU->getPic()->getChromaFormat());
      uiTempStride = pcComYuv->getStride(compID);
      //memset(pcCoeffTemp,0,sizeof(TCoeff)*MAX_BUFFER);
      minibits = MAX_INT ;
      for(UInt index=0;index<4;index++) {
          uiCUHandleAddr = uiCUArray[ index] ;
           if ((int) uiCUHandleAddr<0) continue;
          pcComYuv->clear();
          for (UInt y=0; y<uiHeight; y++) {
				    for (UInt x=0; x<uiWidth; x++) {
                        *(pcComYuv->getAddr(compID)+y*uiTempStride+x) = *(pcResiYuv->getAddr(compID)+y*uiSrcStride+x);
				    }               
	        }
          //pcResiYuv->copyToPartComponent ( compID, pcComYuv, 0);
           
            memset(pcCoeffTemp,0,sizeof(TCoeff)*MAX_BUFFER);
          //_CrtMemState s1,s2,s3;
          //_CrtMemCheckpoint( &s1 );
           temp = GolombCode_Predict_SingleNeighbor(pcComYuv,rTu,compID,uiCUHandleAddr,index,pcCoeffTemp);
           if(temp> -1 && temp < minibits) {
               curindex = index;//uiCUHandleAddr;
                
               minibits = temp ;

               //truebits = minibits  / GOLOMB_EXP_BUFFER + ( (minibits % GOLOMB_EXP_BUFFER==0) ? 0:1  );
               memcpy(pcCoeff,pcCoeffTemp,sizeof(TCoeff)*rect.height*rect.width/* truebits*/); 
           }
          //_CrtMemCheckpoint( &s2 );
          //if ( _CrtMemDifference( &s3, &s1, &s2) )
          //{
          //      _CrtMemDumpStatistics( &s3 ); 
          //       _CrtDumpMemoryLeaks();
          //}
      }    
      pcComYuv->destroy();
      delete pcComYuv;
      pcComYuv = NULL ;
      
      if(minibits!= MAX_INT) {
          assert( uiCUArray[ curindex]>=0);          
          return minibits;
      }

       return MAX_UINT;
}

UInt  PositionCode_Predict(TComYuv *pcResiYuv,TComTU&     rTu,const ComponentID compID, TCoeff* pcCoeff )
{
    if (!rTu.ProcessComponentSection(compID)) return -1;
    
    TComDataCU *pcCU=rTu.getCU();
    UInt  uiCUAddr = pcCU->getCtuRsAddr() ;
    UInt  uiFrameWidthInCU =  pcCU->getPic()->getFrameWidthInCtus() ;
    UInt  uiCUAboveAddr = uiCUAddr - uiFrameWidthInCU;
    UInt  uiCULeftAddr = uiCUAddr - 1 ;
    UInt  uiCUAboveLeftAddr   = uiCUAddr -1 - uiFrameWidthInCU;
    UInt  uiCUAboveRightAddr = uiCUAddr +1 - uiFrameWidthInCU;
    UInt  uiCUArray[4] = {uiCULeftAddr,uiCUAboveAddr,uiCUAboveLeftAddr,uiCUAboveRightAddr};
    UInt  uiCUHandleAddr ;
    Int curindex,minibits,temp/*,truebits*/ ;
    TCoeff  pcCoeffTemp[MAX_BUFFER ];
    TComYuv *pcComYuv = new TComYuv;
    const TComRectangle &rect= rTu.getRect(compID);
      UInt uiTempStride,uiSrcStride;
      
      
      uiSrcStride = pcResiYuv->getStride(compID);
      
      UInt uiHeight = pcResiYuv->getHeight(compID);
      UInt uiWidth =  pcResiYuv->getWidth(compID);
      pcComYuv->create(uiWidth,uiHeight, pcCU->getPic()->getChromaFormat());
      uiTempStride = pcComYuv->getStride(compID);
      //memset(pcCoeffTemp,0,sizeof(TCoeff)*MAX_BUFFER);
      minibits = MAX_INT ;
      for(UInt index=0;index<4;index++) {
          uiCUHandleAddr = uiCUArray[ index] ;
           if ((int) uiCUHandleAddr<0) continue;
          pcComYuv->clear();
          for (UInt y=0; y<uiHeight; y++) {
				    for (UInt x=0; x<uiWidth; x++) {
                        *(pcComYuv->getAddr(compID)+y*uiTempStride+x) = *(pcResiYuv->getAddr(compID)+y*uiSrcStride+x);
				    }               
	        }
          //pcResiYuv->copyToPartComponent ( compID, pcComYuv, 0);
           
            memset(pcCoeffTemp,0,sizeof(TCoeff)*MAX_BUFFER);
          //_CrtMemState s1,s2,s3;
          //_CrtMemCheckpoint( &s1 );
           temp = GolombCode_Predict_SingleNeighbor(pcComYuv,rTu,compID,uiCUHandleAddr,index,pcCoeffTemp);
           if(temp> -1 && temp < minibits) {
               curindex = index;//uiCUHandleAddr;
                
               minibits = temp ;

               //truebits = minibits  / GOLOMB_EXP_BUFFER + ( (minibits % GOLOMB_EXP_BUFFER==0) ? 0:1  );
               memcpy(pcCoeff,pcCoeffTemp,sizeof(TCoeff)*rect.height*rect.width/* truebits*/); 
           }
          //_CrtMemCheckpoint( &s2 );
          //if ( _CrtMemDifference( &s3, &s1, &s2) )
          //{
          //      _CrtMemDumpStatistics( &s3 ); 
          //       _CrtDumpMemoryLeaks();
          //}
      }    
      pcComYuv->destroy();
      delete pcComYuv;
      pcComYuv = NULL ;
      
      if(minibits!= MAX_INT) {
          assert( uiCUArray[ curindex]>=0);          
          return minibits;
      }

       return MAX_UINT;
}