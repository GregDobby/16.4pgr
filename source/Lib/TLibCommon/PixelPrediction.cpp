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

UInt getSerialIndex(UInt uiX, UInt uiY, UInt uiPicWidth)
{
	return (uiY + EXTEG)*(uiPicWidth + 2 * EXTEG) + uiX + EXTEG;
}


// get the 21 neighbors
Void getNeighbors(UInt uiX, UInt uiY, UInt uiPicWidth, Pixel* pPixel, vector<Pixel>& vTemplate)
{
	UInt uiIndex;
	// 1
	uiIndex = getSerialIndex(uiX - 1, uiY, uiPicWidth);
	vTemplate.push_back(pPixel[uiIndex]);
	// 2
	uiIndex = getSerialIndex(uiX - 1, uiY - 1, uiPicWidth);
	vTemplate.push_back(pPixel[uiIndex]);
	// 3
	uiIndex = getSerialIndex(uiX, uiY - 1, uiPicWidth);
	vTemplate.push_back(pPixel[uiIndex]);
	// 4
	uiIndex = getSerialIndex(uiX + 1, uiY - 1, uiPicWidth);
	vTemplate.push_back(pPixel[uiIndex]);
	// 5
	uiIndex = getSerialIndex(uiX + 2, uiY - 1, uiPicWidth);
	vTemplate.push_back(pPixel[uiIndex]);
	// 6
	uiIndex = getSerialIndex(uiX - 2, uiY, uiPicWidth);
	vTemplate.push_back(pPixel[uiIndex]);
	// 7
	uiIndex = getSerialIndex(uiX - 2, uiY - 1, uiPicWidth);
	vTemplate.push_back(pPixel[uiIndex]);
	// 8 
	uiIndex = getSerialIndex(uiX - 2, uiY - 2, uiPicWidth);
	vTemplate.push_back(pPixel[uiIndex]);
	// 9
	uiIndex = getSerialIndex(uiX - 1, uiY - 2, uiPicWidth);
	vTemplate.push_back(pPixel[uiIndex]);
	// 10
	uiIndex = getSerialIndex(uiX, uiY - 2, uiPicWidth);
	vTemplate.push_back(pPixel[uiIndex]);
	// 11
	uiIndex = getSerialIndex(uiX + 1, uiY - 2, uiPicWidth);
	vTemplate.push_back(pPixel[uiIndex]);
	// 12
	uiIndex = getSerialIndex(uiX + 2, uiY - 2, uiPicWidth);
	vTemplate.push_back(pPixel[uiIndex]);
	// 13
	uiIndex = getSerialIndex(uiX - 3, uiY, uiPicWidth);
	vTemplate.push_back(pPixel[uiIndex]);
	// 14
	uiIndex = getSerialIndex(uiX - 3, uiY - 1, uiPicWidth);
	vTemplate.push_back(pPixel[uiIndex]);
	// 15
	uiIndex = getSerialIndex(uiX - 3, uiY - 2, uiPicWidth);
	vTemplate.push_back(pPixel[uiIndex]);
	// 16
	uiIndex = getSerialIndex(uiX - 3, uiY - 3, uiPicWidth);
	vTemplate.push_back(pPixel[uiIndex]);
	// 17
	uiIndex = getSerialIndex(uiX - 2, uiY - 3, uiPicWidth);
	vTemplate.push_back(pPixel[uiIndex]);
	// 18
	uiIndex = getSerialIndex(uiX - 1, uiY - 3, uiPicWidth);
	vTemplate.push_back(pPixel[uiIndex]);
	// 19
	uiIndex = getSerialIndex(uiX, uiY - 3, uiPicWidth);
	vTemplate.push_back(pPixel[uiIndex]);
	// 20
	uiIndex = getSerialIndex(uiX + 1, uiY - 3, uiPicWidth);
	vTemplate.push_back(pPixel[uiIndex]);
	// 21
	uiIndex = getSerialIndex(uiX + 2, uiY - 3, uiPicWidth);
	vTemplate.push_back(pPixel[uiIndex]);
}

// get 24bit hash value
UInt getHashValue(UInt uiX, UInt uiY, UInt uiPicWidth, Pixel* pPixel)
{
	vector<Pixel> vTemplate;
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
	vector<Pixel> vTemplate, vCTemplate;
	// get the 21 neighbors
	getNeighbors(uiX, uiY, uiPicWidth, pPixel, vTemplate);
	getNeighbors(uiCX, uiCY, uiPicWidth, pPixel, vCTemplate);

	bool bContinue = true;
	UInt uiTotalAbsDiff = 0;
	UInt uiNumValidPoints = 0;
	UInt uiNumMatchPoints = 0;
	for (int i = 0; i < 3; i++)
	{
		if (vTemplate[i].m_bIsRec)
		{
			uiNumValidPoints++;
			if (vCTemplate[i].m_bIsRec)
			{
				UInt uiDiff = abs((int)(vTemplate[i].m_uiReco - vCTemplate[i].m_uiReco));
				uiTotalAbsDiff += uiDiff;
				if (uiDiff == 0)
					uiNumMatchPoints++;
			}
			else
			{
				uiTotalAbsDiff += 255;
			}
		}
	}
	if (uiNumMatchPoints < 3)
		bContinue = false;
	for (int i = 3; i < 12 && bContinue; i++)
	{
		if (vTemplate[i].m_bIsRec)
		{
			uiNumValidPoints++;
			if (vCTemplate[i].m_bIsRec)
			{
				UInt uiDiff = abs((int)(vTemplate[i].m_uiReco - vCTemplate[i].m_uiReco));
				uiTotalAbsDiff += uiDiff;
				if (uiDiff == 0)
					uiNumMatchPoints++;
			}
			else
			{
				uiTotalAbsDiff += 255;
			}
		}
	}
	if (uiNumMatchPoints < 8)
		bContinue = false;
	for (int i = 12; i < 21 && bContinue; i++)
	{
		if (vTemplate[i].m_bIsRec)
		{
			uiNumValidPoints++;
			if (vCTemplate[i].m_bIsRec)
			{
				UInt uiDiff = abs((int)(vTemplate[i].m_uiReco - vCTemplate[i].m_uiReco));
				uiTotalAbsDiff += uiDiff;
				if (uiDiff == 0)
					uiNumMatchPoints++;
			}
			else
			{
				uiTotalAbsDiff += 255;
			}
		}
	}
	mmMatchMetric.m_uiX = uiCX;
	mmMatchMetric.m_uiY = uiCY;
	mmMatchMetric.m_uiAbsDiff = uiNumValidPoints == 0 ? 255 : (uiTotalAbsDiff / uiNumValidPoints);
	mmMatchMetric.m_uiNumValidPoints = uiNumValidPoints;
	mmMatchMetric.m_uiNumValidPoints = uiNumMatchPoints;
}
