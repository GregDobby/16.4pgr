#include <TLibCommon\TComDataCU.h>
#include <TLibCommon\TComPicYuv.h>
#include <TLibCommon\TComPic.h>
#include <TLibCommon\TComTU.h>
#include <TLibCommon\TComYuv.h>
#include "TypeDef.h"
#include "PixelPrediction.h"
#include <TLibCommon\TComHash.h>

#include <fstream>
#include <algorithm>
using namespace std;

vector<UInt> g_auiOrgToRsmpld[MAX_NUM_COMPONENT][2];
vector<UInt> g_auiRsmpldToOrg[MAX_NUM_COMPONENT][2];

CQuadTree* g_quadTree[MAX_NUM_COMPONENT];

unordered_set<UInt> g_newHashValues[MAX_NUM_COMPONENT];
UInt g_window = 0;


//vector<PixelTemplate>** g_vLookupTable[MAX_NUM_COMPONENT];
//PixelTemplate* g_pLookupTable[MAX_NUM_COMPONENT][MAX_PT_NUM];

//PixelTemplate*	g_pPixelTemplate[MAX_NUM_COMPONENT][MAX_PT_NUM];	        ///< hash table
//vector<PixelTemplate*>	g_pPixelTemplatePool;								///< convinient for releasing memory

int g_auiTemplateOffset[21][2] = { { -1, 0 }, { -1, -1 }, { 0, -1 }, { 1, -1 }, { 2, -1 }, { -2, 0 }, { -2, -1 }, { -2, -2 }, { -1, -2 }, { 0, -2 }, { 1, -2 },
{ 2, -2 }, { -3, 0 }, { -3, -1 }, { -3, -2 }, { -3, -3 }, { -2, -3 }, { -1, -3 }, { 0, -3 }, { 1, -3 }, { 2, -3 } };

//TComPicYuv* g_pcYuvPred = NULL;
TComPicYuv* g_pcYuvResi = NULL;
TComPicYuv* g_pcYuvAbnormalResi = NULL;


UInt g_uiMaxCUWidth = 64;
UInt g_uiMaxCUHeight = 64;

Palette g_ppPalette[MAX_NUM_COMPONENT];
Palette g_ppCTUPalette[MAX_NUM_COMPONENT];;

//Void matchTemplate(TComDataCU*& rpcTempCU, Pixel** ppPixel)
//{
//	// template matching
//	UInt uiCUPelX = rpcTempCU->getCUPelX();			// x of upper left corner of the cu
//	UInt uiCUPelY = rpcTempCU->getCUPelY();			// y of upper left corner of the cu
//
//	UInt uiMaxCUWidth = rpcTempCU->getSlice()->getSPS()->getMaxCUWidth();		// max cu width
//	UInt uiMaxCUHeight = rpcTempCU->getSlice()->getSPS()->getMaxCUHeight();		// max cu height
//
//	// pic
//	TComPic* pcPic = rpcTempCU->getPic();
//	TComPicYuv* pcPredYuv = pcPic->getPicYuvPred();
//	TComPicYuv* pcResiYuv = pcPic->getPicYuvResi();
//
//	UInt uiNumValidCopmonent = pcPic->getNumberValidComponents();
//
//	vector<PixelTemplate> vInsertList;
//
//	for (UInt ch = 0; ch < uiNumValidCopmonent; ch++)
//	{
//		int all = 0;
//		int average = 0;
//		int afind = 0;
//		int maxfind = 0, minfind = INT_MAX;
//		int ax = 0, ay = 0;
//		int adiff = 0;
//
//		ComponentID cId = ComponentID(ch);
//		// picture description
//		UInt uiStride = pcPredYuv->getStride(cId);									// stride for a certain component
//		UInt uiPicWidth = pcPredYuv->getWidth(cId);									// picture width for a certain component
//		UInt uiPicHeight = pcPredYuv->getHeight(cId);								// picture height for a certain component
//
//		UInt uiCBWidth = uiMaxCUWidth >> (pcPredYuv->getComponentScaleX(cId));		// code block width for a certain component
//		UInt uiCBHeight = uiMaxCUHeight >> (pcPredYuv->getComponentScaleY(cId));	// code block height for a certain component
//
//		// rectangle of the code block
//		UInt uiTopX = Clip3((UInt)0, uiPicWidth, uiCUPelX);
//		UInt uiTopY = Clip3((UInt)0, uiPicHeight, uiCUPelY);
//		UInt uiBottomX = Clip3((UInt)0, uiPicWidth, uiCUPelX + uiCBWidth);
//		UInt uiBottomY = Clip3((UInt)0, uiPicHeight, uiCUPelY + uiCBHeight);
//
//
//		for (UInt uiY = uiTopY; uiY < uiBottomY; uiY++)
//		{
//			for (UInt uiX = uiTopX; uiX < uiBottomX; uiX++)
//			{
//				UInt uiOrgX, uiOrgY;
//				uiOrgX = g_auiRsmpldToOrg[cId][0][uiX];
//				uiOrgY = g_auiRsmpldToOrg[cId][1][uiY];
//
//
//				// template match
//				UInt uiHashValue1, uiHashValue2;
//
//				// get hash values
//				getHashValue(uiOrgX, uiOrgY, uiPicWidth, ppPixel[cId],uiHashValue1,uiHashValue2);
//
//				Pixel* pCurPixel = ppPixel[cId] + getSerialIndex(uiOrgX, uiOrgY, uiPicWidth);
//
//				//pCurPixel->m_uiHashValue = uiHashValue1;
//
//				assert(uiHashValue1 >= 0 && uiHashValue1 < MAX_PT_NUM);
//				
//				// lookup table
//				PixelTemplate* pLookupTable = g_pLookupTable[cId][uiHashValue1];
//				// number of available template pixels
//				UInt uiNumTemplate = getNumTemplate(uiOrgX, uiOrgY, uiPicWidth, ppPixel[cId]);
//
//				// if uiNumTemplate < 1, predict target with default value and do not insert template
//				if (uiNumTemplate < 1)
//				{
//					UInt uiIdx = uiY * uiStride + uiX;
//					pcPredYuv->getAddr(cId)[uiIdx] = pCurPixel->m_uiPred;
//					pcResiYuv->getAddr(cId)[uiIdx] = pCurPixel->m_iResi = pCurPixel->m_uiOrg - pCurPixel->m_uiPred;
//					g_pcYuvResi->getAddr(cId)[uiIdx] = abs(pCurPixel->m_iResi);
//					continue;
//				}
//
//				// if lookuptable is empty, predict target with default value and insert template
//				if (pLookupTable == NULL)
//				{
//					//
//					vInsertList.push_back(PixelTemplate(uiOrgX, uiOrgY, uiHashValue1,uiHashValue2,uiNumTemplate,NEW));
//					UInt uiIdx = uiY*uiStride + uiX;
//					pcPredYuv->getAddr(cId)[uiIdx] = pCurPixel->m_uiPred;
//					pcResiYuv->getAddr(cId)[uiIdx] = pCurPixel->m_iResi = pCurPixel->m_uiOrg - pCurPixel->m_uiPred;
//					g_pcYuvResi->getAddr(cId)[uiIdx] = abs(pCurPixel->m_iResi);
//
//					continue;
//				}
//
//				MatchMetric mmBestMetric;
//				UInt uiListLength = 0;
//				PixelTemplate* pBestMatch = NULL;
//				PixelTemplate* pPixelTemplate = pLookupTable;
//#if PGR_DEBUG
//				int length = 0;
//				int a = 0;
//				int find = 0;
//				int fx = 0, fy = 0;
//				int diff = 0;
//#endif
//				UInt uiRemoved = 0;
//				// find best matched template
//				while (pPixelTemplate != NULL)
//				{
//					UInt uiCX = pPixelTemplate->m_PX;
//					UInt uiCY = pPixelTemplate->m_PY;
//					MatchMetric mmTmp;
//#if PGR_DEBUG
//					length++;
//#endif
//					
//					tryMatch(uiOrgX, uiOrgY, uiCX, uiCY, mmTmp, uiPicWidth, ppPixel[cId]);
//					// set best matched template
//					if (mmTmp.m_uiAbsDiff < mmBestMetric.m_uiAbsDiff)
//						//|| 
//						//(mmTmp.m_uiAbsDiff == mmBestMetric.m_uiAbsDiff)&&(mmTmp.m_uiNumValidPoints > mmBestMetric.m_uiNumValidPoints))
//					{
//						mmBestMetric = mmTmp;
//						pBestMatch = pPixelTemplate;
//						find = length; 
//						fx = abs(int(uiOrgX - uiCX)) / 64;
//						fy = abs(int(uiOrgY - uiCY)) / 64;
//						diff = mmTmp.m_uiAbsDiff;
//
//					}
//
//					// replace useless template
//					//if (mmTmp.m_uiAbsDiff < INSERT_LIMIT && (uiNumTemplate > pPixelTemplate->m_uiNumTemplate || uiNumTemplate == 21) && pPixelTemplate->m_uiState != TO_BE_REMOVED)
//					//{
//					//	pPixelTemplate->m_uiState = TO_BE_REMOVED;
//					//	uiRemoved++;
//					//}
//
//					pPixelTemplate = pPixelTemplate->m_pptNext;
//				}
//
//				// predict target with best matched candidate
//				if (pBestMatch != NULL)
//				{
//					UInt uiCX = pBestMatch->m_PX;
//					UInt uiCY = pBestMatch->m_PY;
//					if ((ppPixel[cId] + getSerialIndex(uiCX, uiCY, uiPicWidth))->m_bIsRec)
//					{
//						pCurPixel->m_mmMatch = mmBestMetric;
//						pBestMatch->m_uiNumUsed++;
//						Pixel* pRefPixel = ppPixel[cId] + getSerialIndex(mmBestMetric.m_uiX, mmBestMetric.m_uiY, uiPicWidth);
//						pCurPixel->m_uiPred = pRefPixel->m_uiReco;								// prediction
//						pCurPixel->m_iResi = pCurPixel->m_uiOrg - pCurPixel->m_uiPred;			// residue
//					}
//				}
//				
//				// insert new template
//				//if (mmBestMetric.m_uiAbsDiff > INSERT_LIMIT)
//				//	vInsertList.push_back(PixelTemplate(uiOrgX, uiOrgY, uiHashValue1,uiHashValue2,uiNumTemplate,NEW));
//				// replace old template 
//				//if(uiNumTemplate < 21 && uiRemoved>0 
//				//	|| uiNumTemplate == 21 && uiRemoved > 1)
//				//	vInsertList.push_back(PixelTemplate(uiOrgX, uiOrgY, uiHashValue1, uiHashValue2, uiNumTemplate, DISPLACE));
//
//				UInt uiIdx = uiY*uiStride + uiX;
//				pcPredYuv->getAddr(cId)[uiIdx] = pCurPixel->m_uiPred;
//				pcResiYuv->getAddr(cId)[uiIdx] = pCurPixel->m_iResi = pCurPixel->m_uiOrg - pCurPixel->m_uiPred;
//				g_pcYuvResi->getAddr(cId)[uiIdx] = abs(pCurPixel->m_iResi);
//
//
//
//#if PGR_DEBUG
//				all += length;
//				afind += find;
//				ax += fx;
//				ay += fy;
//				adiff += diff;
//				minfind = min(minfind, find);
//				maxfind = max(maxfind, find);
//#endif
//			}// end for x
//		}// end for y
//		//fc.close();
//#if PGR_DEBUG
//		cout <<"search:" <<all / 4096 << endl;
//		cout << "find:" << afind / 4096 << endl;
//		cout << "max:" << maxfind << "\t" << "min:" << minfind << endl;
//		cout << "x:" << ax/4096 << "\t" << "y:" << ay/4096 << endl;
//		cout << "diff:" << adiff / 4096 << endl;
//#endif
//
//
//		// insert template
//		for (vector<PixelTemplate>::iterator it = vInsertList.begin(); it != vInsertList.end(); it++)
//		{
//			UInt uiHashValue = it->m_uiHashValue1;
//			if (it->m_uiState == DISPLACE)
//			{
//				PixelTemplate* p = g_pLookupTable[cId][uiHashValue];
//				PixelTemplate* pre = NULL;
//				while (p != NULL)
//				{
//					if (p->m_uiState == TO_BE_REMOVED)
//					{
//						PixelTemplate* q = p;
//						if (pre == NULL)
//						{
//							g_pLookupTable[cId][uiHashValue] = p = p->m_pptNext;
//						}
//						else
//						{
//							pre->m_pptNext = p = p->m_pptNext;
//						}
//						delete q;
//						q = NULL;
//					}
//					else
//					{
//						pre = p;
//						p = p->m_pptNext;
//					}
//				}
//			}
//
//			it->m_uiState = NEW;
//			PixelTemplate* pNewTemplate = new PixelTemplate(*it);
//			pNewTemplate->m_pptNext = g_pLookupTable[cId][uiHashValue];
//			g_pLookupTable[cId][uiHashValue] = pNewTemplate;
//		}
//
//		vInsertList.clear();
//	}// end for ch
//}

int getDirection(int gx, int gy)
{
	int direction = -1;
	int absGx = abs(gx);
	int absGy = abs(gy);

	if (absGx == 0 || absGy == 0)
	{
		if (absGx != 0)
		{
			direction = gx > 0 ? 1: 2;
		}
		else if (absGy != 0)
		{
			direction = gy > 0 ? 3 : 4;
		}
		else
		{
			direction = 0;
		}
	}
	else if (absGx + absGy < 10)
	{
		direction = 0;
	}
	else
	{
		double tan = absGy*1.0 / absGx;
		if (tan < 0.414)
		{
			direction = gx > 0 ? 1 : 2;
		}
		else if (tan > 2.414)
		{
			direction = gy > 0 ? 3 : 4;
		}
		else
		{
			if (gx > 0)
				direction = gy > 0 ? 5 : 7;
			else if (gx < 0)
				direction = gy > 0 ? 7 : 8;
		}
	}
	
	return direction;
}

int getGradient(UInt aTemplate[])
{
	int ret_ = -1;
	int gx, gy;
	int gradient[9] = { 0,0,0,0,0,0,0,0,0 }; 
	// 6
	gx = (aTemplate[8] + 2 * aTemplate[1] + aTemplate[0]) - (aTemplate[14] + 2 * aTemplate[13] + aTemplate[12]);
	gy = (aTemplate[14] + 2 * aTemplate[7] + aTemplate[8]) - (aTemplate[12] + 2 * aTemplate[5] + aTemplate[0]);
	gradient[getDirection(gx, gy)]++;
	// 7
	gx = (aTemplate[17] + 2 * aTemplate[8] + aTemplate[1]) - (aTemplate[15] + 2 * aTemplate[14] + aTemplate[13]);
	gy = (aTemplate[15] + 2 * aTemplate[16] + aTemplate[17]) - (aTemplate[13] + 2 * aTemplate[6] + aTemplate[1]);
	gradient[getDirection(gx, gy)]++;
	// 8
	gx = (aTemplate[18] + 2 * aTemplate[9] + aTemplate[2]) - (aTemplate[16] + 2 * aTemplate[7] + aTemplate[6]);
	gy = (aTemplate[16] + 2 * aTemplate[17] + aTemplate[18]) - (aTemplate[6] + 2 * aTemplate[1] + aTemplate[2]);
	gradient[getDirection(gx, gy)]++;
	// 9
	gx = (aTemplate[19] + 2 * aTemplate[10] + aTemplate[3]) - (aTemplate[17] + 2 * aTemplate[8] + aTemplate[1]);
	gy = (aTemplate[17] + 2 * aTemplate[18] + aTemplate[19]) - (aTemplate[1] + 2 * aTemplate[2] + aTemplate[3]);
	gradient[getDirection(gx, gy)]++;
	// 10
	gx = (aTemplate[20] + 2 * aTemplate[11] + aTemplate[4]) - (aTemplate[18] + 2 * aTemplate[9] + aTemplate[2]);
	gy = (aTemplate[18] + 2 * aTemplate[19] + aTemplate[20]) - (aTemplate[2] + 2 * aTemplate[3] + aTemplate[4]);
	gradient[getDirection(gx, gy)]++;

	for (int i = 0; i < 9; i++)
	{
		if (gradient[i] >= 3)
		{
			ret_ = i;
			break;
		}
	}
		
	return ret_;
}

Void matchTemplateQuadTree(TComDataCU* rpcTempCU)
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

	fstream f;
	char curFilename[64];
	sprintf(curFilename, "predinfo_%d.txt", rpcTempCU->getCtuRsAddr());
	f.open(curFilename, ios::app);
	// component loop
	for (UInt ch = 0; ch < uiNumValidCopmonent; ch++)
	{
		ComponentID cId = ComponentID(ch);
		UInt uiStride = pcPredYuv->getStride(cId);									// stride for a certain component
		UInt uiPicWidth = pcPredYuv->getWidth(cId);									// picture width for a certain component
		UInt uiPicHeight = pcPredYuv->getHeight(cId);								// picture height for a certain component
		
		UInt uiCBWidth = (uiMaxCUWidth >> (pcPredYuv->getComponentScaleX(cId)))>> rpcTempCU->getDepth()[0];;		// code block width for a certain component
		UInt uiCBHeight = (uiMaxCUHeight >> (pcPredYuv->getComponentScaleY(cId))) >> rpcTempCU->getDepth()[0];;	// code block height for a certain component

																					// rectangle of the code block
		UInt uiTopX = Clip3((UInt)0, uiPicWidth, uiCUPelX);
		UInt uiTopY = Clip3((UInt)0, uiPicHeight, uiCUPelY);
		UInt uiBottomX = Clip3((UInt)0, uiPicWidth, uiCUPelX + uiCBWidth);
		UInt uiBottomY = Clip3((UInt)0, uiPicHeight, uiCUPelY + uiCBHeight);

		Pel* pPredBuffer = pcPredYuv->getAddr(cId);
		Pel* pResiBuffer = pcResiYuv->getAddr(cId);
		Pel* pRecoBuffer = pcRecoYuv->getAddr(cId);
		Pel* pOrgBuffer = pcOrgYuv->getAddr(cId);

		int length = 0;
		int start = clock();
		int search = 0;
		int gpn = 0;
		int c = 0;
		int deleted = 0;

		if (g_window % 4 == 0)
			g_newHashValues[cId].clear();

		for (UInt uiY = uiTopY; uiY < uiBottomY; uiY++)
		{
			for (UInt uiX = uiTopX; uiX < uiBottomX; uiX++)
			{
				UInt uiOrgX, uiOrgY;
  				uiOrgX = g_auiRsmpldToOrg[cId][0][uiX];
				uiOrgY = g_auiRsmpldToOrg[cId][1][uiY];



				UInt curTemplate[21];
				getTemplate(curTemplate, pcRecoYuv, cId, uiOrgX, uiOrgY);
				//if (uiY >= 192 && uiX >= 192)
				//{

				//	int direction = getGradient(curTemplate);
				//	if (direction != -1)
				//	{
				//		if (direction == 0)
				//		{
				//			UInt sum = 0;

				//			for (int i = 0; i < 21; i++)
				//				sum += curTemplate[i];
				//			UInt average = sum / 21;

				//			for (int i = 0; i < 21; i++)
				//				sum += abs((int)curTemplate[i] - (int)average);
				//			sum /= 21;
				//			if (sum < 5)
				//			{
				//				gpn++;
				//				UInt uiTargetIdx = uiY*uiStride + uiX;              // resampled coordinates
				//				pPredBuffer[uiTargetIdx] = average;
				//				g_pcYuvResi->getAddr(cId)[uiTargetIdx] = pResiBuffer[uiTargetIdx] = pOrgBuffer[uiTargetIdx] - pPredBuffer[uiTargetIdx];
				//				continue;
				//			}
				//		}
				//		else
				//		{
				//			gpn++;
				//			UInt pred = 0;
				//			if (direction == 1)
				//			{
				//				pred = (curTemplate[2] + curTemplate[9] + curTemplate[18]) / 3;
				//			}
				//			else if (direction == 2)
				//			{
				//				pred = (curTemplate[0] + curTemplate[5] + curTemplate[12]) / 3;
				//			}
				//			else if (direction == 3)
				//			{
				//				pred = (curTemplate[1] + curTemplate[7] + curTemplate[15]) / 3;
				//			}
				//			else if (direction == 4)
				//			{
				//				pred = (curTemplate[0] + curTemplate[2]) / 2;
				//			}

				//			UInt uiTargetIdx = uiY*uiStride + uiX;              // resampled coordinates
				//			pPredBuffer[uiTargetIdx] = pred;
				//			g_pcYuvResi->getAddr(cId)[uiTargetIdx] = pResiBuffer[uiTargetIdx] = pOrgBuffer[uiTargetIdx] - pPredBuffer[uiTargetIdx];
				//			continue;
				//		}
				//		
				//	}	
				//}
				
				UInt uiHashvalue = getHashvalue(curTemplate);
				
				int findstart = clock();
				bool flag = true;
				vector<Template**> can_list;
				g_quadTree[cId]->find(uiHashvalue, uiOrgX, uiOrgY, can_list, flag);  // candidates list
				int findend = clock();
				search += findend - findstart;

				UInt uiMinDiff = numeric_limits<UInt>::max();
				int uiMatchX = -1, uiMatchY = -1;

				int cstart = clock();
				Template* match = NULL;

				UInt cmpTemplate[21];
				for (vector<Template**>::iterator it = can_list.begin(); it != can_list.end(); it++)
				{
					// loop list
					Template* p = **it;
					Template** pre = *it;
					while (p != NULL)
					{
						length++;
						UInt uiCanX = p->x;
						UInt uiCanY = p->y;

						getTemplate(cmpTemplate, pcRecoYuv, cId, uiCanX, uiCanY);
						assert(uiCanX >= 0 && uiCanX < uiPicWidth);
						assert(uiCanY >= 0 && uiCanY < uiPicHeight);

						UInt uiAbsDiff = getTemplateDiff(curTemplate, cmpTemplate);

						if (uiAbsDiff < uiMinDiff)
						{
							uiMinDiff = uiAbsDiff;
							uiMatchX = uiCanX;
							uiMatchY = uiCanY;
						}else

						//if(uiAbsDiff > 5)
						{
							p->t++;
							if (p->t > 64)
							{
								Template* tmp = p;
								p = p->next;
								*pre = p;
								delete tmp;
								deleted++;
								continue;
							}
						}
						pre = &(p->next);
						p = p->next;
					}
				}
				
				int cend = clock();

				c += cend - cstart;
				f << cId << " " << uiOrgX << " " << uiOrgY;
				for (int i = 0; i < 21; i++)
					f << " " << curTemplate[i];
				f << " " << uiMatchX << " " << uiMatchY;
				for (int i = 0; i < 21; i++)
					f << " " << cmpTemplate[i];
				f << " " << uiHashvalue;
				int pred = 0;
				int resi = 0;
				UInt uiTargetIdx = uiY*uiStride + uiX;              // resampled coordinates
				if (uiMatchX == -1)
				{
					pPredBuffer[uiTargetIdx] = 0;
					resi = g_pcYuvResi->getAddr(cId)[uiTargetIdx] = pResiBuffer[uiTargetIdx] = pOrgBuffer[uiTargetIdx];
				}
				else
				{
					uiMatchX = g_auiOrgToRsmpld[cId][0][uiMatchX];
					uiMatchY = g_auiOrgToRsmpld[cId][1][uiMatchY];

					UInt uiPredIdx = uiMatchY*uiStride + uiMatchX;
					pred = pPredBuffer[uiTargetIdx] = pRecoBuffer[uiPredIdx];
					pResiBuffer[uiTargetIdx] = pOrgBuffer[uiTargetIdx] - pPredBuffer[uiTargetIdx];
					resi = g_pcYuvResi->getAddr(cId)[uiTargetIdx] = abs(pResiBuffer[uiTargetIdx]);
					assert(pResiBuffer[uiTargetIdx] + pPredBuffer[uiTargetIdx] == pOrgBuffer[uiTargetIdx]);
				}
				f << " " << pred<<" "<<resi;
				f << endl;
				//if (can_list.empty() && g_newHashValues[cId].count(uiHashvalue) == 0)
				//{
				//	g_newHashValues[cId].insert(uiHashvalue);
				//	continue;
				//}
				if ( uiMinDiff > 5)
				{
					inserList.push_back(make_pair(uiOrgX, uiOrgY));
					insertHashvalues.push_back(uiHashvalue);
				}
			}
		}
		int end = clock();

		for (int i = 0; i < inserList.size(); i++)
		{
			g_quadTree[cId]->insertPoint(inserList[i].first, inserList[i].second, insertHashvalues[i]);
		}

		cout << "Time:" << end - start << endl;
		cout << "Search:" << search << endl;
		cout << "Compare:" << c << endl;
		cout << "insert count:" << inserList.size() << endl;
		cout << "delete count:" << deleted << endl;
		cout << "gradient prediction:" << gpn << endl;
		cout << length / 4096 << endl;

		inserList.clear();
		insertHashvalues.clear();

	}
	f.close();

	g_window++;
}

inline UInt getTemplateDiff(UInt t1[], UInt t2[])
{
	UInt uiAbsDiff = 0;
	for (int i = 0; i < 21; i++)
	{
		uiAbsDiff += abs((int)t1[i] -(int)t2[i]);
	}
	// 0-4
	for (int i = 0; i < 5; i++)
	{
		uiAbsDiff += abs((int)t1[i] - (int)t2[i])<<3;
	}

	// 5-11
	for (int i = 5; i < 12; i++)
	{
		uiAbsDiff += abs((int)t1[i] - (int)t2[i])<<2;
	}
	// 12-20
	for (int i = 12; i < 21; i++)
	{
		uiAbsDiff += abs((int)t1[i] - (int)t2[i])<<1;
	}

	uiAbsDiff /= (5 << 3) + (7 << 2) + (9 << 1);
	return uiAbsDiff;
}

Void updateQuadTree(TComDataCU* pCtu)
{
	//UInt uiCUPelX = pCtu->getCUPelX();			// x of upper left corner of the cu
	//UInt uiCUPelY = pCtu->getCUPelY();			// y of upper left corner of the cu

	//UInt uiMaxCUWidth = pCtu->getSlice()->getSPS()->getMaxCUWidth();		// max cu width
	//UInt uiMaxCUHeight = pCtu->getSlice()->getSPS()->getMaxCUHeight();		// max cu height

	//																			// pic
	//TComPic* pcPic = pCtu->getPic();
	//TComPicYuv* pcRecoYuv = pcPic->getPicYuvRec();


	//UInt uiNumValidCopmonent = pcPic->getNumberValidComponents();

	//int insertstart = clock();

	//// component loop
	//for (UInt ch = 0; ch < uiNumValidCopmonent; ch++)
	//{
	//	ComponentID cId = ComponentID(ch);
	//	UInt uiStride = pcRecoYuv->getStride(cId);									// stride for a certain component
	//	UInt uiPicWidth = pcRecoYuv->getWidth(cId);									// picture width for a certain component
	//	UInt uiPicHeight = pcRecoYuv->getHeight(cId);								// picture height for a certain component

	//	UInt uiCBWidth = uiMaxCUWidth >> (pcRecoYuv->getComponentScaleX(cId));		// code block width for a certain component
	//	UInt uiCBHeight = uiMaxCUHeight >> (pcRecoYuv->getComponentScaleY(cId));	// code block height for a certain component

	//																				// rectangle of the code block
	//	UInt uiTopX = Clip3((UInt)0, uiPicWidth, uiCUPelX);
	//	UInt uiTopY = Clip3((UInt)0, uiPicHeight, uiCUPelY);
	//	UInt uiBottomX = Clip3((UInt)0, uiPicWidth, uiCUPelX + uiCBWidth);
	//	UInt uiBottomY = Clip3((UInt)0, uiPicHeight, uiCUPelY + uiCBHeight);

	//	for (UInt uiY = uiTopY; uiY < uiBottomY; uiY++)
	//	{
	//		for (UInt uiX = uiTopX; uiX < uiBottomX; uiX++)
	//		{
	//			UInt uiOrgX, uiOrgY;
	//			uiOrgX = g_auiRsmpldToOrg[cId][0][uiX];
	//			uiOrgY = g_auiRsmpldToOrg[cId][1][uiY];

	//			UInt curTemplate[21];
	//			getTemplate(curTemplate, pcRecoYuv, cId, uiOrgX, uiOrgY);
	//			UInt uiHashvalue = getHashvalue(curTemplate);
	//			g_quadTree[cId]->insertPoint(uiOrgX, uiOrgY, uiHashvalue);
	//		} // end x
	//	}// end y
	//}// end loop

	//int insertend = clock();
	//cout << "Insert:" << insertend - insertstart << endl;
}

Void getTemplate(UInt aTemplate[], TComPicYuv* pcRecoYuv, ComponentID cId, UInt uiOrgX, UInt uiOrgY)
{
	Pel* pPicBuffer = pcRecoYuv->getAddr(cId);
	UInt uiStride = pcRecoYuv->getStride(cId);
	UInt uiPicWidth = pcRecoYuv->getWidth(cId);
	UInt uiPicHeight = pcRecoYuv->getHeight(cId);
	memset(aTemplate, 0, sizeof(UInt) * 21);
	UInt uiX, uiY;
	for (int i = 0; i < 21; i++)
	{
		uiX = uiOrgX + g_auiTemplateOffset[i][0];
		uiY = uiOrgY + g_auiTemplateOffset[i][1];
		if (uiX < 0 || uiX >= uiPicWidth || uiY < 0 || uiY >= uiPicHeight)
		{
			continue;
		}
		uiX = g_auiOrgToRsmpld[cId][0][uiX];
		uiY = g_auiOrgToRsmpld[cId][1][uiY];
		aTemplate[i] = pPicBuffer[uiY*uiStride + uiX];
	}
}

UInt getHashvalue(UInt aTemplate[])
{
	UInt uiTmp;
	UInt mask = 0xE0;		// (1110 0000)b
	UInt uiHashValue = 0;
	// 1
	uiTmp = 0;
	uiTmp += aTemplate[0];
	uiHashValue |= ((uiTmp & mask) >> 5) << 21;
	// 2
	uiTmp = 0;
	uiTmp += aTemplate[1];
	uiHashValue |= ((uiTmp & mask) >> 5) << 18;
	// 3
	uiTmp = 0;
	uiTmp += aTemplate[2];
	uiHashValue |= ((uiTmp & mask) >> 5) << 15;
	// 4,5
	uiTmp = 0;
	uiTmp += aTemplate[3];
	uiTmp += aTemplate[4];
	uiTmp /= 2;
	uiHashValue |= ((uiTmp & mask) >> 5) << 12;
	// 6,7,13,14
	uiTmp = 0;
	uiTmp += aTemplate[5];
	uiTmp += aTemplate[6];
	uiTmp += aTemplate[12];
	uiTmp += aTemplate[13];
	uiTmp /= 4;
	uiHashValue |= ((uiTmp & mask) >> 5) << 9;
	// 8,15,16,17
	uiTmp = 0;
	uiTmp += aTemplate[7];
	uiTmp += aTemplate[14];
	uiTmp += aTemplate[15];
	uiTmp += aTemplate[16];
	uiTmp /= 4;
	uiHashValue |= ((uiTmp & mask) >> 5) << 6;
	// 9,10,18,19
	uiTmp = 0;
	uiTmp += aTemplate[8];
	uiTmp += aTemplate[9];
	uiTmp += aTemplate[17];
	uiTmp += aTemplate[18];
	uiTmp /= 4;
	uiHashValue |= ((uiTmp & mask) >> 5) << 3;
	// 11,12,20,21
	uiTmp = 0;
	uiTmp += aTemplate[10];
	uiTmp += aTemplate[11];
	uiTmp += aTemplate[19];
	uiTmp += aTemplate[20];
	uiTmp /= 4;
	uiHashValue |= ((uiTmp & mask) >> 5);
	return uiHashValue;
}


// 更新原图像中的像素信息

Void updatePixel(TComDataCU* pCtu, Pixel** ppPixel)
{
	UInt uiMaxCUWidth = pCtu->getSlice()->getSPS()->getMaxCUWidth();		// max cu width
	UInt uiMaxCUHeight = pCtu->getSlice()->getSPS()->getMaxCUHeight();		// max cu height

	// pic
	TComPic *pcPic = pCtu->getPic();
	//TComPicYuv* pcPredYuv = pcPic->getPicYuvPred();
	//TComPicYuv* pcResiYuv = pcPic->getPicYuvResi();
	TComPicYuv* pcRecoYuv = pcPic->getPicYuvRec();
	UInt uiNumValidCopmonent = pcPic->getNumberValidComponents();

	for (UInt ch = 0; ch < uiNumValidCopmonent; ch++)
	{
		ComponentID cId = ComponentID(ch);
		// picture description
		UInt uiStride = pcRecoYuv->getStride(cId);									// stride for a certain component
		UInt uiPicWidth = pcRecoYuv->getWidth(cId);									// picture width for a certain component
		UInt uiPicHeight = pcRecoYuv->getHeight(cId);								// picture height for a certain component

		UInt uiCUPelX = pCtu->getCUPelX() >> (pcRecoYuv->getComponentScaleX(cId));			// x of upper left corner of the cu
		UInt uiCUPelY = pCtu->getCUPelY() >> (pcRecoYuv->getComponentScaleY(cId));;			// y of upper left corner of the
		UInt uiCBWidth = uiMaxCUWidth >> (pcRecoYuv->getComponentScaleX(cId));		// code block width for a certain component
		UInt uiCBHeight = uiMaxCUHeight >> (pcRecoYuv->getComponentScaleY(cId));	// code block height for a certain component

		// rectangle of the code block
		UInt uiTopX = Clip3((UInt)0, uiPicWidth, uiCUPelX);
		UInt uiTopY = Clip3((UInt)0, uiPicHeight, uiCUPelY);
		UInt uiBottomX = Clip3((UInt)0, uiPicWidth, uiCUPelX + uiCBWidth);
		UInt uiBottomY = Clip3((UInt)0, uiPicHeight, uiCUPelY + uiCBHeight);

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
				pPixel->m_uiReco = pBuffer[uiY*uiStride + uiX];
			}
		}
	}
}

// release hash table memory
Void clearPTHashTable()
{
	//for (UInt ch = 0; ch < MAX_NUM_COMPONENT; ch++)
	//{
	//	ComponentID cId = ComponentID(ch);
	//	if (g_vLookupTable[cId] == NULL)
	//		continue;
	//	for (UInt ui = 0; ui < MAX_PT_NUM; ui++)
	//	{
	//		if (g_vLookupTable[cId][ui] != NULL)
	//		{
	//			g_vLookupTable[cId][ui]->clear();
	//			delete g_vLookupTable[cId][ui];
	//			g_vLookupTable[cId][ui] = NULL;
	//		}
	//	}
	//}

	//for (UInt ch = 0; ch < MAX_NUM_COMPONENT; ch++)
	//{
	//	ComponentID cId = ComponentID(ch);
	//	for (UInt ui = 0; ui < MAX_PT_NUM; ui++)
	//	{
	//		PixelTemplate* p = g_pLookupTable[cId][ui];
	//		while (p != NULL)
	//		{
	//			PixelTemplate* next = p->m_pptNext;
	//			delete p;
	//			p = next;
	//		}
	//	}
	//}
}

// init hash table
Void initPTHashTable()
{
	clearPTHashTable();
	//for (UInt ch = 0; ch < MAX_NUM_COMPONENT; ch++)
	//{
	//	ComponentID cId = ComponentID(ch);
	//	g_vLookupTable[cId] = new vector<PixelTemplate>*[MAX_PT_NUM];
	//	memset(g_vLookupTable[cId], 0, sizeof(vector<PixelTemplate>*)*MAX_PT_NUM);
	//}
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
			UInt uiIdY = uiPicRsmpldY % uiMaxCUHeight;
			UInt uiPicOrgY;

			if (uiIdY < uiNumberUseBiggerStrideY)
				uiPicOrgY = uiPicRsmpldY / uiMaxCUHeight + uiIdY * uiStrideYplus1;	// corresponding Y in the original picture
			else
				uiPicOrgY = uiPicRsmpldY / uiMaxCUHeight + uiNumberUseBiggerStrideY * uiStrideYplus1 + (uiIdY - uiNumberUseBiggerStrideY) * uiStrideY;

			g_auiOrgToRsmpld[cId][1][uiPicOrgY] = uiPicRsmpldY;
			g_auiRsmpldToOrg[cId][1][uiPicRsmpldY] = uiPicOrgY;
		}

		for (UInt uiPicRsmpldX = 0; uiPicRsmpldX < uiPicWidth; uiPicRsmpldX++)
		{
			UInt uiIdX = uiPicRsmpldX % uiMaxCUWidth;
			UInt uiPicOrgX;
			if (uiIdX < uiNumberUseBiggerStrideX)
				uiPicOrgX = uiPicRsmpldX / uiMaxCUWidth + uiIdX * uiStrideXplus1;	// corresponding X in the original picture
			else
				uiPicOrgX = uiPicRsmpldX / uiMaxCUWidth + uiNumberUseBiggerStrideX * uiStrideXplus1 + (uiIdX - uiNumberUseBiggerStrideX) * uiStrideX;

			g_auiOrgToRsmpld[cId][0][uiPicOrgX] = uiPicRsmpldX;
			g_auiRsmpldToOrg[cId][0][uiPicRsmpldX] = uiPicOrgX;
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
	for (int i = 0; i < 21; i++)
	{
		uiIndex = getSerialIndex(uiX + g_auiTemplateOffset[i][0], uiY + g_auiTemplateOffset[i][1], uiPicWidth);
		vTemplate[i] = pPixel[uiIndex];
	}
}

// get 24bit hash value
Void getHashValue(UInt uiX, UInt uiY, UInt uiPicWidth, Pixel* pPixel, UInt& uiHashValue1, UInt& uiHashValue2)
{
	Pixel vTemplate[21];
	// load template data
	getNeighbors(uiX, uiY, uiPicWidth, pPixel, vTemplate);

	// calculate hash value
	UInt uiTmp;
	UInt mask = 0xE0;		// (1110 0000)b


	UInt uiHashValue = 0;
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
	uiHashValue1 = uiHashValue;
}

Void tryMatch(UInt uiX, UInt uiY, UInt uiCX, UInt uiCY, MatchMetric &mmMatchMetric, UInt uiPicWidth, Pixel* pPixel)
{
	Pixel apNeighbor, apCNeighbor;
	
	UInt uiTotalAbsDiff = 0;
	UInt uiNumValidPoints = 0;
	UInt uiNumMatchPoints = 0;
	UInt uiTrueDiff = 0;
	for (int i = 0; i < 21; i++)
	{
		UInt uiIndex;
		uiIndex = getSerialIndex(uiX + g_auiTemplateOffset[i][0], uiY + g_auiTemplateOffset[i][1], uiPicWidth);
		apNeighbor = pPixel[uiIndex];
		uiIndex = getSerialIndex(uiCX + g_auiTemplateOffset[i][0], uiCY + g_auiTemplateOffset[i][1], uiPicWidth);
		apCNeighbor = pPixel[uiIndex];

		if (apNeighbor.m_bIsRec)
		{
			if (apCNeighbor.m_bIsRec)
			{
				uiNumValidPoints++;
				UInt uiDiff = abs((int)(apNeighbor.m_uiReco - apCNeighbor.m_uiReco));
				uiTrueDiff += uiDiff;
				uiTotalAbsDiff += uiDiff;
				if (uiDiff == 0)
					uiNumMatchPoints++;
			} 
			else
			{
				uiTotalAbsDiff += 255;
			}
		}
		//if (uiNumMatchPoints < i)
		//	break;
	}

	mmMatchMetric.m_uiX = uiCX;
	mmMatchMetric.m_uiY = uiCY;
	//mmMatchMetric.m_uiAbsDiff = uiNumValidPoints == 0 ? 255 : (uiTotalAbsDiff / uiNumValidPoints);
	mmMatchMetric.m_uiAbsDiff = uiNumValidPoints == 0 ? 255 : (uiTrueDiff / uiNumValidPoints);
	mmMatchMetric.m_uiNumValidPoints = uiNumValidPoints;
	mmMatchMetric.m_uiNumMatchPoints = uiNumMatchPoints;
}

UInt getNumTemplate(UInt uiX, UInt uiY, UInt uiPicWidth, Pixel* pPixel)
{
	Pixel apNeighbor;
	UInt n = 0;
	for (int i = 0; i < 21; i++)
	{
		UInt uiIndex;
		uiIndex = getSerialIndex(uiX + g_auiTemplateOffset[i][0], uiY + g_auiTemplateOffset[i][1], uiPicWidth);
		apNeighbor = pPixel[uiIndex];
		if (apNeighbor.m_bIsRec)
			n++;
	}
	return n;
}


int GetGroupID(int n)
{
	assert(n > 0);
	//only for n>0
	int k = 2;
	while (n > (1 << k) - 2)
	{
		k++;
	}
	return k - 1;//1,2,3.....
}

int  UnaryEncode(int n, TCoeff *buffer, UInt &pos)
{
	int temp = n;
	while (n > 0) {
		WriteBit(1, buffer, pos);
		n--;
	}
	WriteBit(0, buffer, pos);
	return temp + 1;
}

int  UnaryDecode(int *buffer, UInt &pos)
{
	int n = 0;
	while (ReadBit(buffer, pos) == 1) {
		n++;
	}
	return n;
}

int ReadBits(int len, int *buffer, UInt &pos)
{
	int m, q, r;
	int  n = 0;
	int result = 0;
	while (n < len) {
		q = (pos) / GOLOMB_EXP_BUFFER;
		r = GOLOMB_EXP_BUFFER - 1 - (pos) % GOLOMB_EXP_BUFFER;
		m = (*(buffer + q)) >> r & 1;
		result = result * 2 + m;
		pos++;
		n++;
	}
	return result;
}

int ReadBit(int *buffer, UInt &pos)
{
	int m, q, r;
	{
		q = (pos) / GOLOMB_EXP_BUFFER;
		r = GOLOMB_EXP_BUFFER - 1 - (pos) % GOLOMB_EXP_BUFFER;
		m = (*(buffer + q)) >> r & 1;
		pos++;
	}
	return m;
}

Void WriteBits(TCoeff index, int num, TCoeff *buffer, UInt &pos)
{
	assert(num <= GOLOMB_EXP_BUFFER);
	while (num > 0) {
		WriteBit(index >> (num - 1) & 1, buffer, pos);
		num--;
	}
}

int ExpGolombEncode(int x, int *buffer, UInt &pos)
{
	int temp;
	if (x <= 0)
		temp = abs(x) * 2;
	else
		temp = abs(x) * 2 - 1;

	if (temp == 0)
	{
		WriteBit(0, buffer, pos);
		return 1;
	}
	else
	{
		int GroupID = GetGroupID(temp);
		int bits = UnaryEncode(GroupID, buffer, pos);
		UInt  Index = temp - ((1 << GroupID) - 1);
		WriteBits(Index, GroupID, buffer, pos);
		return bits + GroupID;
	}
}

int ExpGolombDecode(int *buffer, UInt &pos)
{
	int result;
	int GroupID = UnaryDecode(buffer, pos);
	if (GroupID == 0) {
		result = 0;
	}
	else {
		int Base = (1 << GroupID) - 1;
		int Index = ReadBits(GroupID, buffer, pos);
		result = (Base + Index);
	}
	if (result % 2 == 0)
		return -result / 2;
	else
		return (result + 1) / 2;
}

Void WriteBit(int n, TCoeff *buffer, UInt &pos)
{
	assert(n == 1 || n == 0);
	int  q = (pos) / GOLOMB_EXP_BUFFER;
	int   r = GOLOMB_EXP_BUFFER - 1 - (pos) % GOLOMB_EXP_BUFFER;
	*(buffer + q) &= ~(1 << r);
	*(buffer + q) |= n << r;

	pos++;
}

int getG0Bits(int x)
{
	int bits = 0;
	if (abs(x) == 0) bits = 1;
	else if (abs(x) <= 1) bits = 3;
	else if (abs(x) <= 3) bits = 5;
	else if (abs(x) <= 7) bits = 7;
	else if (abs(x) <= 15) bits = 9;
	else if (abs(x) <= 31) bits = 11;
	else if (abs(x) <= 63) bits = 13;
	else if (abs(x) <= 127) bits = 15;
	else if (abs(x) <= 255) bits = 17;
	else bits = 19;
	return bits;
}

Double GetMinRadius(UInt x1, UInt y1, UInt x2, UInt y2)
{
	UInt temx = x2 - x1;
	UInt temy = y2 - y1;
	return sqrt(temx*temx + temy*temy);
}

Void  PositionCode_PathLeast(TComYuv *pcResiYuv, const ComponentID compID, TCoeff* pcCoeff, UInt* puiPLTIdx, UInt& uiNumIdx)
{
	const ChannelType chType = toChannelType(compID);
	const UInt    uiWidth = pcResiYuv->getWidth(compID);
	const UInt    uiHeight = pcResiYuv->getHeight(compID);
	const UInt    uiStride = pcResiYuv->getStride(compID);
	Pel*    piResi = pcResiYuv->getAddr(compID);
	Pel*    pResi = piResi;

	TCoeff  *buffer = pcCoeff;

	int  indexlist[MAX_BUFFER][2];
	memset(indexlist, 0, sizeof(int)*MAX_BUFFER * 2);

	uiNumIdx = 0;
	UInt uiPreX = 0;
	UInt uiPreY = 0;
	Bool bContinue = true;
	// add in 2016-02-19
	Bool bStopForMark = false;
	Double dMinRadius = 0.0;
	// end

	while (bContinue)
	{
		UInt uiMinDis = 1000;
		UInt uiNextX = -1;
		UInt uiNextY = -1;
		pResi = piResi;
		bStopForMark = false;

		for (UInt uiY = uiPreY; uiY < uiHeight; uiY++)
		{
			if (bStopForMark == true)
			{
				break;
			}
			for (UInt uiX = uiPreX; uiX < uiWidth; uiX++)
			{
				assert(pResi[uiX] >-256 && pResi[uiX] < 256);
				// 原条件：已经扫描的点不扫描，扫描方式逐行逐列
				// if (pResi[uiX] != -1)
				// 更新条件，
				// 1.贪心方式：每找到一个点下一次在此半径内寻找
				// 2.二每次开始搜索条件变为上一次开始时的终点坐标
				if (pResi[uiX] != -1)
				{
					UInt uiDis = 0;									// 初始化当前距离
					uiDis += getG0Bits(uiX - uiPreX);				// 0阶哥伦布编码
					uiDis += getG0Bits(uiY - uiPreY);				// back up uiPreX 2016-02-18
					if (uiDis < uiMinDis)
					{
						uiNextX = uiX;
						uiNextY = uiY;
						uiMinDis = uiDis;
						dMinRadius = GetMinRadius(uiPreX, uiPreY, uiNextX, uiNextY);
					}
				}
				if (uiX >(uiPreX + dMinRadius))
				{
					continue;
				}
				if (uiX > uiNextX && uiY > uiNextY
					&& GetMinRadius(uiPreX, uiPreY, uiX, uiY) > dMinRadius && uiY > (uiPreY + dMinRadius))
				{
					//有用条件：uiY > (uiPreY + dMinRadius)
					bStopForMark = true;
					break;
				}
			}
			pResi += uiStride;										// 指针跳到第二行
		}


		if (uiNextX != -1 && uiNextY != -1)
		{
			*(buffer + (uiNumIdx * 2)) = uiNextX - uiPreX;
			*(buffer + (uiNumIdx * 2 + 1)) = uiNextY - uiPreY;
			uiPreX = uiNextX;
			uiPreY = uiNextY;
			// palette index
			*(puiPLTIdx + (uiNumIdx++)) = *(piResi + uiNextY*uiStride + uiNextX);		//调色板记录异常像素值
			*(piResi + uiNextY*uiStride + uiNextX) = -1;			//本次找到的距离最小的点
		}
		else
		{
			bContinue = false;
		}
	}

	return;
}

//Void  PositionCode_PathLeast(TComYuv *pcResiYuv, const ComponentID compID, TCoeff* pcCoeff, UInt* puiPLTIdx, UInt& uiNumIdx)
//{
//	const ChannelType chType = toChannelType(compID);
//	const UInt    uiWidth = pcResiYuv->getWidth(compID);
//	const UInt    uiHeight = pcResiYuv->getHeight(compID);
//	const UInt    uiStride = pcResiYuv->getStride(compID);
//	Pel*    piResi = pcResiYuv->getAddr(compID);
//	Pel*    pResi = piResi;
//
//	TCoeff  *buffer = pcCoeff;
//
//	int  indexlist[MAX_BUFFER][2];
//	memset(indexlist, 0, sizeof(int)*MAX_BUFFER * 2);
//
//	uiNumIdx = 0;
//	UInt uiPreX = 0;
//	UInt uiPreY = 0;
//	Bool bContinue = true;
//
//	while (bContinue)
//	{
//		UInt uiMinDis = 1000;
//		UInt uiNextX = -1;
//		UInt uiNextY = -1;
//		pResi = piResi;
//		for (UInt uiY = 0; uiY < uiHeight; uiY++)
//		{
//			for (UInt uiX = 0; uiX<uiWidth; uiX++)
//			{
//				assert(pResi[uiX] >-256 && pResi[uiX] < 256);
//				if (pResi[uiX] != -1)
//				{
//					UInt uiDis = 0;
//					uiDis += getG0Bits(uiX - uiPreX);
//					uiDis += getG0Bits(uiY - uiPreX);
//					if (uiDis < uiMinDis)
//					{
//						uiNextX = uiX;
//						uiNextY = uiY;
//						uiMinDis = uiDis;
//					}
//				}
//			}
//			pResi += uiStride;
//		}
//		if (uiNextX != -1 && uiNextY != -1)
//		{
//			buffer[uiNumIdx*2] = uiNextX - uiPreX;
//			buffer[uiNumIdx * 2 + 1] = uiNextY - uiPreY;
//			uiPreX = uiNextX;
//			uiPreY = uiNextY;
//			// palette index
//			puiPLTIdx[uiNumIdx++] = *(piResi + uiNextY*uiStride + uiNextX);
//			*(piResi + uiNextY*uiStride + uiNextX) = -1;
//		}
//		else
//		{
//			bContinue = false;;
//		}
//	}
//	return;
//}

int compare4(const void * a, const void * b)
{
	return (*((int*)a + 3) - *((int*)b + 3));
}
int compare5(const void * a, const void * b)
{
	return (*((int*)a + 4) - *((int*)b + 4));
}
//index== 0 above 1 left 2 above and left 3 above and right
Int  GolombCode_Predict_SingleNeighbor(TComYuv *pcResiYuv, TComTU&     rTu, const ComponentID compID, UInt uiCUHandleAddr, UInt uiAIndex, TCoeff* pcCoeff)
{
	const Bool       bIsLuma = isLuma(compID);
	const TComRectangle &rect = rTu.getRect(compID);
	TComDataCU *pcCU = rTu.getCU();
	UInt  uiCUAddr = pcCU->getCtuRsAddr();

	//if  ((int)uiCUHandleAddr < 0)     return -1;

	TComPicYuv *pcPicYuvResi = pcCU->getPic()->getPicYuvResi();
	if (pcPicYuvResi == NULL)  return -1;
	const UInt uiAbsPartIdx = rTu.GetAbsPartIdxTU();
	//    const UInt  uiZOrder      = pcCU->getZorderIdxInCU() +uiAbsPartIdx;

	const UInt uiTrDepth = rTu.GetTransformDepthRelAdj(compID);
	const UInt uiFullDepth = rTu.GetTransformDepthTotal();
	const UInt uiLog2TrSize = rTu.GetLog2LumaTrSize();
	const ChromaFormat chFmt = pcCU->getPic()->getChromaFormat();

	const UInt    uiWidth = rect.width;
	const UInt    uiHeight = rect.height;
	const UInt    uiStride = pcResiYuv->getStride(compID);

	UInt   uiAddr = pcCU->getCtuRsAddr();

	TComYuv  *pcTemp;
	pcTemp = new TComYuv;
	UInt  uiSrc1Stride = pcPicYuvResi->getStride(compID);
	UInt  CUPelX, CUPelY;
	CUPelX = (uiCUHandleAddr % pcCU->getPic()->getFrameWidthInCtus()) * g_uiMaxCUWidth;
	CUPelY = (uiCUHandleAddr / pcCU->getPic()->getFrameWidthInCtus()) * g_uiMaxCUHeight;
	CUPelX = CUPelX + g_auiRasterToPelX[g_auiZscanToRaster[uiAbsPartIdx]];
	CUPelY = CUPelY + g_auiRasterToPelY[g_auiZscanToRaster[uiAbsPartIdx]];
	//for(int m=0;m<256;m++) cout<<g_auiZscanToRaster[m] <<" ";cout<<endl;
	//for(int m=0;m<256;m++) cout<<g_auiRasterToPelX[m] <<" ";cout<<endl;
	//for(int m=0;m<256;m++) cout<<g_auiRasterToPelY[m] <<" ";cout<<endl;
	//Pel *pSrc1 = pcPicYuvResi->getAddr(compID) +CUPelY * uiSrc1Stride + CUPelX;
	Pel  *pSrc1 = pcPicYuvResi->getAddr(compID, uiCUHandleAddr, uiAbsPartIdx + pcCU->getZorderIdxInCtu());
	/*       if( compID != COMPONENT_Y)
		   {
		   pSrc1 = pcPicYuvResi->getAddr(COMPONENT_Y, uiCUHandleAddr, uiAbsPartIdx + pcCU->getZorderIdxInCU());
		   }*/
	pcTemp->create(uiWidth, uiHeight, chFmt);
	//       pcTemp->copyFromPicComponent(compID,pcPicYuvResi,uiCUHandleAddr, pcCU->getZorderIdxInCU()+uiAbsPartIdx);

	UInt  uiTempStride = pcTemp->getStride(compID);
	Pel  *pTemp = pcTemp->getAddr(compID);
	for (Int y = 0; y < uiHeight; y++)
	{
		for (Int x = 0; x < uiWidth; x++)
		{
			pTemp[x] = pSrc1[x];
		}
		pTemp += uiTempStride;
		pSrc1 += uiSrc1Stride;
	}

	int srclx = 0; int srcly = 0; int srclv = 0;
	int srchasleft = 1;
	Pel  srcpel;
	int  srclist[3][64 * 64];
	int  srcindex = 0;
	memset(srclist, -1, 3 * 64 * 64 * sizeof(int));
	int cursrclistindex = 0;

	Pel*  piSrc = pcTemp->getAddr(compID);
	//Pel*  piSrc     = pcTemp->getAddr(compID, uiAbsPartIdx);
	Pel*  pSrc = piSrc;
	//found the source list
	while (srchasleft) {
		int ndis = 1000;
		int nx = -1; int ny = -1;
		pSrc = piSrc;
		for (UInt y = 0; y < uiHeight; y++) {
			for (UInt x = 0; x<uiWidth; x++) {
				assert(pSrc[x] >-256 && pSrc[x] < 256);
				if (pSrc[x] != 0) {
					int dis = 0;
					dis += getG0Bits((x - srclx));
					dis += getG0Bits((y - srcly));
					if (dis < ndis) {
						nx = x;
						ny = y;
						ndis = dis;
					}
				}
			}
			pSrc += uiTempStride;
		}
		if (nx != -1 && ny != -1) {
			srcpel = *(piSrc + ny*uiTempStride + nx);
			srclx = nx; srcly = ny; srclv = srcpel;
			srclist[0][srcindex] = srclx;
			srclist[1][srcindex] = srcly;
			srclist[2][srcindex] = srcpel;
			srcindex++;
			*(piSrc + ny*uiTempStride + nx) = 0;
		}
		else {
			srchasleft = 0;
		}
	}
	if (srcindex == 0)  {
		pcTemp->destroy();
		delete pcTemp;
		pcTemp = NULL;
		return -1;
	}
	////
	TComPicYuv *pcPicOrg = pcCU->getPic()->getPicYuvOrg();
	Pel*  piOrg = pcPicOrg->getAddr(compID, pcCU->getCtuRsAddr(), pcCU->getZorderIdxInCtu() + uiAbsPartIdx);
	const UInt   uiOrgStride = pcPicOrg->getStride(compID);
	////
	Pel*  piResi = pcResiYuv->getAddr(compID, uiAbsPartIdx);
	Pel*  pResi = piResi;
	int dstindex = 0;
	int  indexlist[64 * 64][5];
	memset(indexlist, 0, 5 * 64 * 64 * sizeof(int));
	int contz = 0;
	int contnz = 0;
	int cs = 0;
	int bits = 0;
	//        int temp;
	int lx = 0; int ly = 0; int lv = 0;
	int hasleft = 1;
	Pel  pel;

	while (hasleft) {
		//found the least distance point
		int ndis = 1000;
		int nx = -1; int ny = -1;
		pResi = piResi;
		for (UInt y = 0; y < uiHeight; y++) {
			for (UInt x = 0; x < uiWidth; x++) {
				if (pResi[x] != 0) {
					int dis = 0;
					dis += getG0Bits((x - lx));
					dis += getG0Bits((y - ly));
					if (dis < ndis) {
						nx = x;
						ny = y;
						ndis = dis;
					}
				}
			}
			pResi += uiStride;
		}
		if (nx != -1 && ny != -1) {
			pel = *(piResi + ny*uiStride + nx);

			int srcdis = 1024 * 4;
			int srccur = -1;
			for (UInt s = 0; s < srcindex; s++) {
				int curdis = 0;

				curdis += getG0Bits((nx - srclist[0][s]));
				curdis += getG0Bits((ny - srclist[1][s]));
				//                          curdis += getG0Bits( (pel-srclist[2][s]));// getG0Bits can handle -512 && 512

				if (curdis < srcdis) {
					srccur = s;
					srcdis = curdis;
				}
			}
			if (srccur != -1) {
				indexlist[dstindex][0] = nx - srclist[0][srccur];
				indexlist[dstindex][1] = ny - srclist[1][srccur];
				assert(pel != 0);

				indexlist[dstindex][4] = srccur;

				dstindex++;
				cursrclistindex = srccur;
			}
			else {
				assert(true);
			}
			lx = nx; ly = ny; lv = pel;
			*(piResi + ny*uiStride + nx) = 0;
		}
		else {
			hasleft = 0;
		}
	}

	pcTemp->destroy();
	delete pcTemp;
	pcTemp = NULL;

	if (dstindex == 0) {
		assert(bits == 0);
		return bits;
	}
	qsort(indexlist, dstindex, sizeof(int)* 5, compare5);

	for (UInt x = dstindex - 1; x > 0; x--) {
		indexlist[x][4] -= indexlist[x - 1][4];
	}

	//bits += getG0Bits( (indexlist[0][0]));
	//bits += getG0Bits( (indexlist[0][1]));

	int maxlength = 0;
	UInt truebits = 0;
	bool  vlcf[3] = { false, false, false };// dx & dy   residual  srcindex
	int z01 = 0;
	for (UInt x = 1; x < dstindex; x++) {
		if (indexlist[x][0] == indexlist[x - 1][0] && indexlist[x][1] == indexlist[x - 1][1]) {
			maxlength++;
		}
		else {
			bits += getG0Bits((maxlength));
			bits += getG0Bits((indexlist[x - 1][0]));
			bits += getG0Bits((indexlist[x - 1][1]));
			maxlength = 0;
		}
		if (indexlist[x][0] == 0 && indexlist[x][1] == 0) z01++;
	}

	bits += getG0Bits((maxlength));
	bits += getG0Bits((indexlist[dstindex - 1][0]));
	bits += getG0Bits((indexlist[dstindex - 1][1]));
	UInt sbits = 0;
	for (UInt x = 0; x < dstindex; x++) {
		sbits += getG0Bits((indexlist[x][0]));
		sbits += getG0Bits((indexlist[x][1]));
	}
	//   printf("gain %6d position before %6d after %6d\n",sbits-bits,sbits,bits);

	if (sbits < bits)  {
		vlcf[0] = true;
		bits = sbits;
	}

	sbits = bits + 1;
	//bits += getG0Bits( PTable(compID,indexlist[0][2]));
	{
		maxlength = 0;
		for (UInt x = 1; x < dstindex; x++) {
			if (indexlist[x][2] == indexlist[x - 1][2] && indexlist[x][3] == indexlist[x - 1][3]) {
				maxlength++;
			}
			else {
				bits += getG0Bits((maxlength));
				bits += getG0Bits(indexlist[x - 1][3]);

				bits += getG0Bits(indexlist[x - 1][2]);
				maxlength = 0;
			}
		}
		bits += getG0Bits((maxlength));
		bits += getG0Bits(indexlist[dstindex - 1][3]);
		bits += getG0Bits((indexlist[dstindex - 1][2]));
	}
	UInt vbits = 0;
	sbits = bits - sbits;
	for (UInt x = 0; x < dstindex; x++) {
		{
			vbits += getG0Bits(indexlist[x][2]);
			vbits += getG0Bits(indexlist[x][3]);
		}
	}
	//        printf("gain %6d delta resi before %6d after %6d\n",vbits-sbits,vbits,sbits);
	if (vbits < sbits) {
		vlcf[1] = true;
		bits = bits - sbits + vbits;
	}
	sbits = bits + 1;
	//bits += getG0Bits( (indexlist[0][3]));
	Int srcPosIndex;

	srcPosIndex = 4;

	maxlength = 0;

	for (UInt x = 1; x < dstindex; x++) {
		if (indexlist[x][srcPosIndex] == indexlist[x - 1][srcPosIndex]) {
			maxlength++;
		}
		else {
			bits += getG0Bits((maxlength));
			bits += getG0Bits((indexlist[x - 1][srcPosIndex]));
			maxlength = 0;
		}
	}

	bits += getG0Bits((maxlength));
	bits += getG0Bits((indexlist[dstindex - 1][srcPosIndex]));

	sbits = bits - sbits;
	vbits = 0;
	for (UInt x = 0; x < dstindex; x++) {
		vbits += getG0Bits((indexlist[x][srcPosIndex]));
	}
	//        printf("gain %6d delta index before %6d after %6d\n",vbits-sbits,vbits,sbits);
	if (vbits < sbits) {
		vlcf[2] = true;
		bits = bits - sbits + vbits;
	}
	//#if INTRA_PR_DEBUG
	//        if( pcCU->getAddr()==INTRA_PR_CUNUM )
	//            printf("position distance zero  %6d of %6d  total bits %6d\n",z01,dstindex,bits+1);
	//#endif
	truebits = 0;
	ExpGolombEncode(uiAIndex, pcCoeff, truebits);
	//--------------encode  srcindex

	if (vlcf[2]) {
		ExpGolombEncode(0, pcCoeff, truebits);
		for (UInt x = 0; x < dstindex; x++) {
			//cout<<"  "<<indexlist[x][3]  ;
			ExpGolombEncode((indexlist[x][srcPosIndex]), pcCoeff, truebits);
		}
	}
	else{
		ExpGolombEncode(1, pcCoeff, truebits);
		maxlength = 0;
		for (UInt x = 1; x<dstindex; x++) {
			if (indexlist[x][srcPosIndex] == indexlist[x - 1][srcPosIndex]) {
				maxlength++;
			}
			else {
				ExpGolombEncode((maxlength), pcCoeff, truebits);
				ExpGolombEncode((indexlist[x - 1][srcPosIndex]), pcCoeff, truebits);
				maxlength = 0;
			}
		}
		assert(maxlength>-1);
		ExpGolombEncode((maxlength), pcCoeff, truebits);
		ExpGolombEncode((indexlist[dstindex - 1][srcPosIndex]), pcCoeff, truebits);
	}
	ExpGolombEncode(-1, pcCoeff, truebits);
	//---------------encode  residual

	if (vlcf[1]) {
		ExpGolombEncode(0, pcCoeff, truebits);
		for (UInt x = 0; x < dstindex; x++) {
			{
				ExpGolombEncode(indexlist[x][2], pcCoeff, truebits);
				//  if( !bNoResidual)
				//      ExpGolombEncode( indexlist[x][3],pcCoeff,truebits);
			}
		}
	}
	else{
		ExpGolombEncode(1, pcCoeff, truebits);
		//ExpGolombEncode( (indexlist[0][2]),pcCoeff,truebits);
		maxlength = 0;
		{
			for (UInt x = 1; x < dstindex; x++) {
				if (indexlist[x][2] == indexlist[x - 1][2] && indexlist[x][3] == indexlist[x - 1][3]) {
					maxlength++;
				}
				else {
					ExpGolombEncode((maxlength), pcCoeff, truebits);
					//assert( (maxlength)>=0);
					ExpGolombEncode(indexlist[x - 1][2], pcCoeff, truebits);
					//   if( !bNoResidual)
					//       ExpGolombEncode( indexlist[x-1][3],pcCoeff,truebits);

					maxlength = 0;
				}
			}
			ExpGolombEncode((maxlength), pcCoeff, truebits);
			ExpGolombEncode(indexlist[dstindex - 1][2], pcCoeff, truebits);
			//if( !bNoResidual)
			//    ExpGolombEncode( indexlist[dstindex-1][3] , pcCoeff, truebits);
		}
	}
	//--------------encode dx and dy -----------
	if (vlcf[0])  {
		ExpGolombEncode(0, pcCoeff, truebits);
		for (UInt x = 0; x < dstindex; x++) {
			ExpGolombEncode((indexlist[x][0]), pcCoeff, truebits);
			ExpGolombEncode((indexlist[x][1]), pcCoeff, truebits);
		}
	}
	else{
		ExpGolombEncode(1, pcCoeff, truebits);
		//ExpGolombEncode( (indexlist[0][0]),pcCoeff,truebits);
		//ExpGolombEncode( (indexlist[0][1]),pcCoeff,truebits);

		maxlength = 0;

		for (UInt x = 1; x < dstindex; x++) {
			if (indexlist[x][0] == indexlist[x - 1][0] && indexlist[x][1] == indexlist[x - 1][1]) {
				maxlength++;
			}
			else {
				ExpGolombEncode((maxlength), pcCoeff, truebits);
				ExpGolombEncode((indexlist[x - 1][0]), pcCoeff, truebits);
				ExpGolombEncode((indexlist[x - 1][1]), pcCoeff, truebits);
				maxlength = 0;
			}
		}
		ExpGolombEncode((maxlength), pcCoeff, truebits);
		ExpGolombEncode((indexlist[dstindex - 1][0]), pcCoeff, truebits);
		ExpGolombEncode((indexlist[dstindex - 1][1]), pcCoeff, truebits);
	}
	return   truebits;/* bits +1*/;
}

Int compareint(const void * a, const void * b)
{
	return (*(Int*)a - *(Int*)b);
}

UInt  PositionCode_Predict(TComYuv *pcResiYuv, TComTU&     rTu, const ComponentID compID, TCoeff* pcCoeff /*,Bool bMV,bool bNoResidual*/)
{
	if (!rTu.ProcessComponentSection(compID)) return -1;

	TComDataCU *pcCU = rTu.getCU();
	UInt  uiCUAddr = pcCU->getCtuRsAddr();
	UInt  uiFrameWidthInCU = pcCU->getPic()->getFrameWidthInCtus();
	UInt  uiCUAboveAddr = uiCUAddr - uiFrameWidthInCU;
	UInt  uiCULeftAddr = uiCUAddr - 1;
	UInt  uiCUAboveLeftAddr = uiCUAddr - 1 - uiFrameWidthInCU;
	UInt  uiCUAboveRightAddr = uiCUAddr + 1 - uiFrameWidthInCU;
	UInt  uiCUArray[4] = { uiCULeftAddr, uiCUAboveAddr, uiCUAboveLeftAddr, uiCUAboveRightAddr };
	UInt  uiCUHandleAddr;
	Int curindex, minibits, temp/*,truebits*/;
	TCoeff  pcCoeffTemp[MAX_BUFFER];
	TComYuv *pcComYuv = new TComYuv;
	const TComRectangle &rect = rTu.getRect(compID);
	UInt uiTempStride, uiSrcStride;

	uiSrcStride = pcResiYuv->getStride(compID);

	UInt uiHeight = pcResiYuv->getHeight(compID);
	UInt uiWidth = pcResiYuv->getWidth(compID);
	pcComYuv->create(uiWidth, uiHeight, pcCU->getPic()->getChromaFormat());
	uiTempStride = pcComYuv->getStride(compID);
	//memset(pcCoeffTemp,0,sizeof(TCoeff)*MAX_BUFFER);
	minibits = MAX_INT;
	for (UInt index = 0; index < 4; index++) {
		uiCUHandleAddr = uiCUArray[index];
		if ((int)uiCUHandleAddr < 0) continue;
		pcComYuv->clear();
		for (UInt y = 0; y < uiHeight; y++) {
			for (UInt x = 0; x<uiWidth; x++) {
				*(pcComYuv->getAddr(compID) + y*uiTempStride + x) = *(pcResiYuv->getAddr(compID) + y*uiSrcStride + x);
			}
		}
		//pcResiYuv->copyToPartComponent ( compID, pcComYuv, 0);

		memset(pcCoeffTemp, 0, sizeof(TCoeff)*MAX_BUFFER);
		//_CrtMemState s1,s2,s3;
		//_CrtMemCheckpoint( &s1 );
		temp = GolombCode_Predict_SingleNeighbor(pcComYuv, rTu, compID, uiCUHandleAddr, index, pcCoeffTemp);
		if (temp> -1 && temp < minibits) {
			curindex = index;//uiCUHandleAddr;

			minibits = temp;

			//truebits = minibits  / GOLOMB_EXP_BUFFER + ( (minibits % GOLOMB_EXP_BUFFER==0) ? 0:1  );
			memcpy(pcCoeff, pcCoeffTemp, sizeof(TCoeff)*rect.height*rect.width/* truebits*/);
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
	pcComYuv = NULL;

	if (minibits != MAX_INT) {
		assert(uiCUArray[curindex] >= 0);
		return minibits;
	}

	return MAX_UINT;
}

bool cmpPelCount(void* pc1, void* pc2)
{
	PelCount* a = (PelCount*)pc1;
	PelCount* b = (PelCount*)pc2;
	return a->m_uiCount > b->m_uiCount;
};

Void derivePGRGlobalPLT(TComPicYuv* pcOrgYuv)
{
	UInt uiNumValidComponents = pcOrgYuv->getNumberValidComponents();
	for (UInt ch = 0; ch < uiNumValidComponents; ch++)
	{
		ComponentID cId = ComponentID(ch);
		Pel* pOrg = pcOrgYuv->getAddr(cId);
		UInt uiWidth = pcOrgYuv->getWidth(cId);
		UInt uiHeight = pcOrgYuv->getHeight(cId);
		UInt uiStride = pcOrgYuv->getStride(cId);
		UInt uiNumPixels = uiWidth*uiHeight;

		PelCount* pPixelCount[256];
		for (int i = 0; i < 256; i++)
			pPixelCount[i] = new PelCount(i);
		// statistics
		for (UInt uiY = 0; uiY < uiHeight; uiY++)
		{
			for (UInt uiX = 0; uiX < uiWidth; uiX++)
			{
				pPixelCount[pOrg[uiX]]->m_uiCount++;
			}

			pOrg += uiStride;
		}

		g_ppPalette[cId].m_uiSize = 0;
		// sort
		sort(pPixelCount, pPixelCount + 256, cmpPelCount);

		// insert entry
		for (int i = 0; i < 4; i++)
		{
			g_ppPalette[cId].m_pEntry[i] = pPixelCount[i]->m_uiVal;
			g_ppPalette[cId].m_uiSize++;
		}

		for (int i = 0; i < 256; i++)
			delete pPixelCount[i];
	}
}

Void derivePGRPLT(TComDataCU* pcCtu)
{
	TComPic* pcPic = pcCtu->getPic();
	TComPicYuv* pcOrgYuv = pcPic->getPicYuvOrg();
	UInt uiNumValidComponents = pcOrgYuv->getNumberValidComponents();

	UInt uiMaxCUWidth = pcCtu->getSlice()->getSPS()->getMaxCUWidth();
	UInt uiMaxCUHeight = pcCtu->getSlice()->getSPS()->getMaxCUWidth();

	for (UInt ch = 0; ch < uiNumValidComponents; ch++)
	{
		ComponentID cId = ComponentID(ch);

		UInt uiPicWidth = pcOrgYuv->getWidth(cId);
		UInt uiPicHeight = pcOrgYuv->getHeight(cId);
		UInt uiStride = pcOrgYuv->getStride(cId);

		UInt uiCUPelX = pcCtu->getCUPelX() >> (pcOrgYuv->getComponentScaleX(cId));			// x of upper left corner of the cu
		UInt uiCUPelY = pcCtu->getCUPelY() >> (pcOrgYuv->getComponentScaleY(cId));;			// y of upper left corner of the

		UInt uiCBWidth = uiMaxCUWidth >> (pcOrgYuv->getComponentScaleX(cId));
		UInt uiCBHeight = uiMaxCUHeight >> (pcOrgYuv->getComponentScaleY(cId));

		uiCBWidth = Clip3((UInt)0, uiPicWidth - uiCUPelX, uiCBWidth);
		uiCBHeight = Clip3((UInt)0, uiPicHeight - uiCUPelY, uiCBHeight);

		// statistics

		PelCount* pPixelCount[256];
		for (int i = 0; i < 256; i++)
			pPixelCount[i] = new PelCount(i);

		Pel* pOrg = pcOrgYuv->getAddr(cId, pcCtu->getCtuRsAddr());
		for (UInt uiY = 0; uiY < uiCBHeight; uiY++)
		{
			for (UInt uiX = 0; uiX < uiCBWidth; uiX++)
			{
				pPixelCount[pOrg[uiX]]->m_uiCount++;
			}

			pOrg += uiStride;
		}

		// sort
		sort(pPixelCount, pPixelCount + 256, cmpPelCount);

		g_ppCTUPalette[cId].m_uiSize = 0;
		// insert entry
		for (int i = 0, k = 0; k < 4; i++)
		{
			bool bDuplicate = false;
			for (int j = 0; j < 4; j++)
			{
				// duplicate
				if (g_ppCTUPalette[cId].m_pEntry[i] == g_ppPalette[cId].m_pEntry[j])
				{
					bDuplicate = true;
					break;
				}
			}
			if (!bDuplicate)
			{
				g_ppCTUPalette[cId].m_pEntry[k++] = pPixelCount[i]->m_uiVal;
				g_ppCTUPalette[cId].m_uiSize++;
			}
		}
		for (int i = 0; i < 256; i++)
			delete pPixelCount[i];
	}
}