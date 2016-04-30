#include "BlockTemplate.h"
#include <TLibCommon\TComDataCU.h>
#include <TLibCommon\TComPicYuv.h>
#include <TLibCommon\TComPic.h>
#include <TLibCommon\TComTU.h>
#include <TLibCommon\TComYuv.h>
#include "TypeDef.h"
#include "PixelPrediction.h"
#include <TLibCommon\TComHash.h>

vector<UInt> g_auiBlockOrgToRsmpld[MAX_NUM_COMPONENT][2];
vector<UInt> g_auiBlockRsmpldToOrg[MAX_NUM_COMPONENT][2];

Void initCoordinateMap(UInt uiSourceWidth, UInt uiSourceHeight, UInt uiMaxCUWidth, UInt uiMaxCUHeight, UInt uiBlockSize, ChromaFormat chromaFormatIDC)
{
	assert(uiMaxCUWidth % uiBlockSize == 0 && uiMaxCUHeight % uiBlockSize == 0);

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

		UInt uiBlockNumInCUWidth = uiMaxCUWidth / uiBlockSize;
		UInt uiBlockNumInCUHeight = uiMaxCUHeight / uiBlockSize;

		UInt uiBlockNumInPicWidth = uiPicWidth / uiBlockSize;
		UInt uiBlockNumInPicHeight = uiPicHeight / uiBlockSize;

		g_auiBlockOrgToRsmpld[cId][0].resize(uiBlockNumInPicWidth, -1);		// orginal to resampled x
		g_auiBlockOrgToRsmpld[cId][1].resize(uiBlockNumInPicHeight, -1);		// original to resampled y
		
		g_auiBlockRsmpldToOrg[cId][0].resize(uiBlockNumInPicWidth, -1);		// resampled to original x
		g_auiBlockRsmpldToOrg[cId][1].resize(uiBlockNumInPicHeight, -1);		// resampled to original y

		UInt uiStrideX = uiPicWidth / uiMaxCUWidth;			// sample stride in horizontal direction as well as the number of intact CUs in a row
		UInt uiStrideY = uiPicHeight / uiMaxCUHeight;		// sample stride in vertical direction as well as the number of intact CUs in a column

		UInt uiStrideXplus1 = uiStrideX + 1;
		UInt uiStrideYplus1 = uiStrideY + 1;

		UInt uiNumberUseBiggerStrideX = uiPicWidth % uiMaxCUWidth / uiBlockSize;		// number of bigger strides in x direction
		UInt uiNumberUseBiggerStrideY = uiPicHeight % uiMaxCUHeight / uiBlockSize;	// number of bigger strides in y direction

		UInt uiXLimit = uiNumberUseBiggerStrideX*uiStrideXplus1;
		UInt uiYLimit = uiNumberUseBiggerStrideY*uiStrideYplus1;

		for (UInt uiOrgBlockX = 0; uiOrgBlockX < uiBlockNumInPicWidth; uiOrgBlockX++)
		{
			UInt uiRsmpldBlockX;
			if (uiOrgBlockX < uiXLimit)
			{
				uiRsmpldBlockX = uiOrgBlockX / uiStrideXplus1 + uiBlockNumInCUWidth * (uiOrgBlockX % uiStrideXplus1);
			}
			else
			{
				uiRsmpldBlockX = uiNumberUseBiggerStrideX + (uiOrgBlockX - uiXLimit) / uiStrideX + uiBlockNumInCUWidth * ((uiOrgBlockX - uiXLimit) % uiStrideX);
			}

			g_auiBlockOrgToRsmpld[cId][0][uiOrgBlockX] = uiRsmpldBlockX;
			g_auiBlockRsmpldToOrg[cId][0][uiRsmpldBlockX] = uiOrgBlockX;
			for (int i = 0; i < uiBlockSize; i++)
			{
				UInt uiOrgX = uiOrgBlockX*uiBlockSize + i;
				UInt uiRsmpldX = uiRsmpldBlockX*uiBlockSize + i;

				g_auiOrgToRsmpld[cId][0][uiOrgX] = uiRsmpldX;
				g_auiRsmpldToOrg[cId][0][uiRsmpldX] = uiOrgX;

			}
		}

		for (UInt uiOrgBlockY = 0; uiOrgBlockY < uiBlockNumInPicHeight; uiOrgBlockY++)
		{
			UInt uiRsmpldBlockY;
			if (uiOrgBlockY < uiYLimit)
			{
				uiRsmpldBlockY = uiOrgBlockY / uiStrideYplus1 + uiBlockNumInCUHeight * (uiOrgBlockY % uiStrideYplus1);
			}
			else
			{
				uiRsmpldBlockY = uiNumberUseBiggerStrideY + (uiOrgBlockY - uiYLimit) / uiStrideY + uiBlockNumInCUHeight * ((uiOrgBlockY - uiYLimit) % uiStrideY);
			}

			g_auiBlockOrgToRsmpld[cId][1][uiOrgBlockY] = uiRsmpldBlockY;
			g_auiBlockRsmpldToOrg[cId][1][uiRsmpldBlockY] = uiOrgBlockY;

			for (int i = 0; i < uiBlockSize; i++)
			{
				UInt uiOrgY = uiOrgBlockY*uiBlockSize + i;
				UInt uiRsmpldY = uiRsmpldBlockY*uiBlockSize + i;

				g_auiOrgToRsmpld[cId][1][uiOrgY] = uiRsmpldY;
				g_auiRsmpldToOrg[cId][1][uiRsmpldY] = uiOrgY;
			}
		}
	}
}

Void getPelValue(vector<vector<Pel>>& matrix, UInt uiX, UInt uiY,UInt uiBlockSize, UInt uiStride, Pel* pRecoBuffer)
{
	matrix.resize(uiBlockSize, vector<Pel>(uiBlockSize, 0));
	Pel* p = pRecoBuffer + uiX + uiY * uiStride;
	for (UInt y = 0; y < uiBlockSize; y++)
	{
		for (UInt x = 0; x < uiBlockSize; x++)
		{
			matrix[y][x] = p[x];
		}
		p += uiStride;
	}
}

UInt getAverageGradient(vector<vector<Pel>>& matrix)
{
	UInt uiRow = matrix.size();
	if (uiRow == 0)
		return 0;
	UInt uiCol = matrix[0].size();
	UInt ans = 0;
	UInt count = 0;
	for (UInt y = 1; y < uiRow - 1; y++)
	{
		for (UInt x = 1; x < uiCol - 1; x++)
		{
			int gradX = (matrix[y - 1][x + 1] + matrix[y][x + 1] * 2 + matrix[y + 1][x + 1]) - (matrix[y - 1][x - 1] + matrix[y][x - 1] * 2 + matrix[y + 1][x - 1]);
			int gradY = (matrix[y - 1][x - 1] + matrix[y - 1][x] * 2 + matrix[y - 1][x + 1]) - (matrix[y + 1][x - 1] + matrix[y + 1][x] * 2 + matrix[y + 1][x + 1]);
			ans += (abs(gradX) + abs(gradY)) / 10;
			count++;
		}
	}
	if(count != 0)
		ans /= count;
	return ans;
}

UInt getDC(vector<vector<Pel>>& matrix)
{
	UInt uiRow = matrix.size();
	if (uiRow == 0)
		return 0;
	UInt uiCol = matrix[0].size();
	UInt ans = 0;
	for (UInt y = 0; y < uiRow; y++)
	{
		for (UInt x = 0; x < uiCol; x++)
		{
			ans += matrix[y][x];
		}
	}
	ans /= uiRow*uiCol;
	return ans;
}

Void getTemplate(vector<vector<Pel>> vTemplate[], ComponentID cId, UInt uiX, UInt uiY, UInt uiBlockSize, UInt uiStride, Pel * pRecoBuffer)
{
	if (uiX  < uiBlockSize || uiY  < uiBlockSize)
	{
		vTemplate[2].resize(uiBlockSize, vector<Pel>(uiBlockSize, 0));

		if (uiX < uiBlockSize)
		{
			vTemplate[0].resize(uiBlockSize, vector<Pel>(uiBlockSize, 0));
		}
		else
		{
			getPelValue(vTemplate[0], g_auiOrgToRsmpld[cId][0][uiX - uiBlockSize], g_auiOrgToRsmpld[cId][1][uiY], uiBlockSize, uiStride, pRecoBuffer);
		}

		if (uiY < uiBlockSize)
		{
			vTemplate[1].resize(uiBlockSize, vector<Pel>(uiBlockSize, 0));
		}
		else
		{
			getPelValue(vTemplate[1], g_auiOrgToRsmpld[cId][0][uiX], g_auiOrgToRsmpld[cId][1][uiY - uiBlockSize], uiBlockSize, uiStride, pRecoBuffer);
		}
	}
	else
	{
		getPelValue(vTemplate[0], g_auiOrgToRsmpld[cId][0][uiX - uiBlockSize], g_auiOrgToRsmpld[cId][1][uiY], uiBlockSize, uiStride, pRecoBuffer);
		getPelValue(vTemplate[1], g_auiOrgToRsmpld[cId][0][uiX], g_auiOrgToRsmpld[cId][1][uiY - uiBlockSize], uiBlockSize, uiStride, pRecoBuffer);
		getPelValue(vTemplate[2], g_auiOrgToRsmpld[cId][0][uiX - uiBlockSize], g_auiOrgToRsmpld[cId][1][uiY - uiBlockSize], uiBlockSize, uiStride, pRecoBuffer);
	}
	
}

UInt calcTemplateDiff(vector<vector<Pel>> vTemplate1[], vector<vector<Pel>> vTemplate2[])
{
	UInt ans = 0;
	UInt uiRow = vTemplate1[0].size();
	UInt uiCol = vTemplate1[0][0].size();
	for (int i = 0; i < BLOCK_NUM_IN_TEMPLATE; i++)
	{
		UInt curDiff = 0;
		for (UInt y = 0; y < uiRow; y++)
		{
			for (UInt x = 0; x < uiCol; x++)
			{
				curDiff += abs(vTemplate1[i][y][x] - vTemplate2[i][y][x]);
			}
		}
		ans += curDiff / (uiRow*uiCol);
	}
	ans /= BLOCK_NUM_IN_TEMPLATE;
	return ans;
}

UInt getHashvalue(vector<vector<Pel>> vTemplate[])
{
	UInt uiHashvalue = 0;
	for (int i = 0; i < BLOCK_NUM_IN_TEMPLATE; i++)
	{
		UInt uiAverageGradient = getAverageGradient(vTemplate[i]);
		UInt uiDC = getDC(vTemplate[i]);
		UInt mask = 0xf0;
		uiHashvalue = uiHashvalue << 8;
		uiHashvalue += (uiDC & mask) + ((uiAverageGradient & mask) >> 4);
	}

	return uiHashvalue;
}


Void matchTemplate(TComDataCU * rpcTempCU, UInt uiBlockSize)
{
	UInt uiCUPelX = rpcTempCU->getCUPelX();			// x of upper left corner of the cu
	UInt uiCUPelY = rpcTempCU->getCUPelY();			// y of upper left corner of the cu

	UInt uiMaxCUWidth = rpcTempCU->getSlice()->getSPS()->getMaxCUWidth();		// max cu width
	UInt uiMaxCUHeight = rpcTempCU->getSlice()->getSPS()->getMaxCUHeight();		// max cu height

																				// pic
	TComPic* pcPic = rpcTempCU->getPic();
	TComPicYuv* pcPredYuv = pcPic->getPicYuvPred();
	TComPicYuv* pcResiYuv = pcPic->getPicYuvResi();
	TComPicYuv* pcRecoYuv = pcPic->getPicYuvRec();
	TComPicYuv* pcOrgYuv = pcPic->getPicYuvOrg();


	UInt uiNumValidCopmonent = pcPic->getNumberValidComponents();
	vector<pair<int, int>> inserList;
	vector<UInt> insertHashvalues;
	//cout << "<===========================================================>" << endl;
	//cout << "CTU:\t" << uiCUPelX << "\t" << uiCUPelY << endl;

	// component loop
	for (UInt ch = 0; ch < uiNumValidCopmonent; ch++)
	{
		ComponentID cId = ComponentID(ch);
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

		Pel* pPredBuffer = pcPredYuv->getAddr(cId);
		Pel* pResiBuffer = pcResiYuv->getAddr(cId);
		Pel* pRecoBuffer = pcRecoYuv->getAddr(cId);
		Pel* pOrgBuffer = pcOrgYuv->getAddr(cId);

		// block idx range
		UInt uiTopBlockIdX = uiTopX / uiBlockSize;
		UInt uiTopBlockIdY = uiTopY / uiBlockSize;
		UInt uiBottomBlockIdX = uiBottomX / uiBlockSize;
		UInt uiBottomBlockIdY = uiBottomY / uiBlockSize;

		//cout << ">> Component:\t" << cId << endl;

		int searchLength = 0;
		int start = clock();
		

		for (UInt uiBlockIdY = uiTopBlockIdY; uiBlockIdY < uiBottomBlockIdY; uiBlockIdY++)
		{
			for (UInt uiBlockIdX = uiTopBlockIdX; uiBlockIdX < uiBottomBlockIdX; uiBlockIdX++)
			{
				UInt uiOrgBlcokIdX = g_auiBlockRsmpldToOrg[cId][0][uiBlockIdX];
				UInt uiOrgBlcokIdY = g_auiBlockRsmpldToOrg[cId][1][uiBlockIdY];

				// get template and hashvalue
				vector<vector<Pel>> vTemplate[BLOCK_NUM_IN_TEMPLATE];
				getTemplate(vTemplate, cId, uiOrgBlcokIdX*uiBlockSize, uiOrgBlcokIdY*uiBlockSize, uiBlockSize, uiStride, pRecoBuffer);
				UInt uiHashvalue = getHashvalue(vTemplate);

				// find candidate list
				vector<Template**> can_list;
				bool flag = false;
				g_quadTree[cId]->find(uiHashvalue, uiOrgBlcokIdX, uiOrgBlcokIdY, can_list, flag);  // candidates list

				UInt uiMinDiff = numeric_limits<UInt>::max();
				int uiMatchX = -1, uiMatchY = -1;

				int cstart = clock();
				Template* match = NULL;

				// find best matched candidate
				for (vector<Template**>::iterator it = can_list.begin(); it != can_list.end(); it++)
				{
					// loop list
					Template* p = **it;
					Template** pre = *it;
					while (p != NULL)
					{

						searchLength++;

						UInt uiCanX = p->x;
						UInt uiCanY = p->y;

						vector<vector<Pel>> vCanTemplate[BLOCK_NUM_IN_TEMPLATE];
						getTemplate(vCanTemplate, cId, uiCanX*uiBlockSize, uiCanY*uiBlockSize, uiBlockSize, uiStride, pRecoBuffer);

						assert(uiCanX >= 0 && uiCanX < uiPicWidth);
						assert(uiCanY >= 0 && uiCanY < uiPicHeight);

						UInt uiAbsDiff = calcTemplateDiff(vTemplate, vCanTemplate);

						if (uiAbsDiff < uiMinDiff)
						{
							uiMinDiff = uiAbsDiff;
							uiMatchX = uiCanX;
							uiMatchY = uiCanY;
							match = p;
						}
						else

							//if(uiAbsDiff > 5)
						{
							p->t++;
							if (p->t > 64)
							{
								Template* tmp = p;
								p = p->next;
								*pre = p;
								delete tmp;
								continue;
							}
						}
						pre = &(p->next);
						p = p->next;
					}
				}
				// set predicted values and calculate residuals
				vector<vector<Pel>> pred(uiBlockSize, vector<Pel>(uiBlockSize, 0));
				if (uiMatchX != -1)
				{
					match->t = 0;
					uiMatchX = g_auiOrgToRsmpld[cId][0][uiMatchX*uiBlockSize];
					uiMatchY = g_auiOrgToRsmpld[cId][1][uiMatchY*uiBlockSize];
					

					vector<vector<Pel>> vNeighborTemplate[BLOCK_NUM_IN_TEMPLATE];
					int bestX = uiMatchX, bestY = uiMatchY;
					//for (UInt y = max(uiMatchY - 3, 0); y < uiMatchY; y++)
					//{
					//	for (UInt x = max(uiMatchX - 3, 0); x < uiMatchX; x++)
					//	{
					//		getTemplate(vNeighborTemplate, cId, x, y, uiBlockSize, uiStride, pRecoBuffer);
					//		UInt diff = calcTemplateDiff(vNeighborTemplate, vTemplate);
					//		if (diff < uiMinDiff)
					//		{
					//			bestX = x;
					//			bestY = y;
					//		}
					//	}
					//}

					getPelValue(pred, bestX, bestY, uiBlockSize, uiStride, pRecoBuffer);

				}

				Pel* pPred = pPredBuffer + uiBlockIdX*uiBlockSize + uiBlockIdY * uiBlockSize * uiStride;
				Pel* pResi = pResiBuffer + uiBlockIdX*uiBlockSize + uiBlockIdY * uiBlockSize * uiStride;
				Pel* pOrg = pOrgBuffer + uiBlockIdX*uiBlockSize + uiBlockIdY * uiBlockSize * uiStride;
				Pel* pgResi = g_pcYuvResi->getAddr(cId) + uiBlockIdX*uiBlockSize + uiBlockIdY * uiBlockSize * uiStride;

				for (UInt y = 0; y < uiBlockSize; y++)
				{
					for (UInt x = 0; x < uiBlockSize; x++)
					{
						pPred[x] = pred[y][x];
						pgResi[x] = pResi[x] = pOrg[x] - pPred[x];
					}
					pPred += uiStride;
					pResi += uiStride;
					pOrg += uiStride;
					pgResi += uiStride;
				}
				//if (can_list.empty() && g_newHashValues[cId].count(uiHashvalue) == 0)
				//{
				//	g_newHashValues[cId].insert(uiHashvalue);
				//	continue;
				//}
				if (uiMinDiff > 5)
				{
					inserList.push_back(make_pair(uiOrgBlcokIdX, uiOrgBlcokIdY));
					insertHashvalues.push_back(uiHashvalue);
				}
			} // end x
		} // end y

		for (int i = 0; i < inserList.size(); i++)
		{
			g_quadTree[cId]->insertPoint(inserList[i].first, inserList[i].second, insertHashvalues[i]);
		}

		int end = clock();
		//cout << "Time:\t" << end - start << endl;
		//cout << "Total Search Length:\t" << searchLength << endl;
		//cout << "Average Search Length:\t" << searchLength / 16 / 16 << endl;
		//cout << "Insert Count:\t" << inserList.size() << endl;

		inserList.clear();
		insertHashvalues.clear();
	}
}


