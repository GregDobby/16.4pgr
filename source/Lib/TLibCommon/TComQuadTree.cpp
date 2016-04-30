#include "TComQuadTree.h"
#include <algorithm>
#include <assert.h>
CQuadTree::CQuadTree()
{
}

CQuadTree::CQuadTree(int uiMaxDepth, int uiNodeCapacity, int uiRangeX, int uiRangeY)
{
	m_root = NULL;
	m_maxDepth = uiMaxDepth;
	m_nodeCapacity = uiNodeCapacity;
	m_rangeX = uiRangeX;
	m_rangeY = uiRangeY;
}

CQuadTree::~CQuadTree()
{
	dfsFreeNode(m_root);
}

int CNode::getIndex(int x, int y)
{
	int idx = -1;
	int leftUpperX = m_leftUpperX;
	int leftUpperY = m_leftUpperY;
	int rangeX = m_rangeX;
	int rangeY = m_rangeY;
	int midX = leftUpperX + rangeX / 2;
	int midY = leftUpperY + rangeY / 2;
	//if (x >= leftUpperX && x < midX)
	//{
	//	if (y >= leftUpperY && y < midY)
	//	{
	//		idx = 0;
	//	}
	//	else if (y >= midY && y<leftUpperY + rangeY)
	//	{
	//		idx = 2;
	//	}
	//}
	//else if (x >= midX && x < leftUpperX + rangeX)
	//{
	//	if (y >= leftUpperY && y < midY)
	//	{
	//		idx = 1;
	//	}
	//	else if (y >= midY && y < leftUpperY + rangeY)
	//	{
	//		idx = 3;
	//	}
	//}

	if (x < midX)
	{
		if (y < midY)
		{
			idx = 0;
		}
		else if (y >= midY)
		{
			idx = 2;
		}
	}
	else if (x >= midX)
	{
		if (y < midY)
		{
			idx = 1;
		}
		else if (y >= midY)
		{
			idx = 3;
		}
	}
	
	return idx;
}

void CQuadTree::insertPoint(int x, int y, int hashvalue)
{
	if (m_root == NULL)
		m_root = new CNode(0, 0, m_rangeX, m_rangeY, 0);

	insertPoint(x, y, hashvalue, m_root);

}
void CQuadTree::insertPoint(int x, int y, int hashvalue, CNode * root)
{
	// hash set
	//if (root->m_hashSet.find(hashvalue) == root->m_hashSet.end())
	//	root->m_hashSet.insert(hashvalue);

	if (root->m_children[0] != NULL)
	{
		int idx = root->getIndex(x, y);
		insertPoint(x, y, hashvalue, root->m_children[idx]);
	}
	else
	{
		map<unsigned int, Template*> &data = root->m_data;
		if (data.find(hashvalue) == data.end())
		{
			data[hashvalue] = new Template(x,y);
		}
		else
		{
			Template* tmp = new Template(x, y);
			tmp->next = data[hashvalue];
			data[hashvalue] = tmp;
		}


		root->m_count++;

		// split
		if (root->m_count > m_nodeCapacity && root->m_depth < m_maxDepth)
		{
			root->split();
		}
	}
}



void CQuadTree::deletePoint()
{
}

void CQuadTree::dfsFreeNode(CNode * root)
{
	if (root != NULL)
	{
		if (root->m_children[0] != NULL)
		{
			dfsFreeNode(root->m_children[0]);
		}

		if (root->m_children[1] != NULL)
		{
			dfsFreeNode(root->m_children[1]);
		}

		if (root->m_children[2] != NULL)
		{
			dfsFreeNode(root->m_children[0]);
		}

		if (root->m_children[3] != NULL)
		{
			dfsFreeNode(root->m_children[0]);
		}

		for (map<unsigned int, Template*>::iterator it = root->m_data.begin(); it != root->m_data.end(); it++)
		{
			Template* p = it->second;
			while (p != NULL)
			{
				Template* tmp = p;
				p = p->next;
				delete p;
			}
		}

		delete root;
	}
}

void CQuadTree::find(int hashvalue, int orgX, int orgY, vector<Template**>& can_list,bool& flag)
{
	m_leafDepth = -1;
	if (m_root == NULL)
	{
		flag = false;
	}
	else
		find(hashvalue, orgX, orgY, m_root, can_list, flag);
}

void CQuadTree::find(int hashvalue, int x, int y, CNode * root, vector<Template**>& can_list,bool& flag)
{
	//m_leafDepth = max(m_leafDepth,root->m_depth);
	//if (root != NULL && root->m_hashSet.find(hashvalue) != root->m_hashSet.end())
	//{
		int idx = root->getIndex(x, y);
		// leaf
		if (root->m_children[0] == NULL)
		{
			if (root->m_data[hashvalue] != NULL)
				can_list.push_back(&root->m_data[hashvalue]);
			else
				root->m_data.erase(hashvalue);
		}
		else
		{
			find(hashvalue, x, y, root->m_children[idx], can_list, flag);
			if (m_leafDepth == -1)
				m_leafDepth = root->m_depth;
			// failed
			
			if (can_list.empty() && m_leafDepth - root->m_depth < BACK_LEVEL)
			{
				flag = false;
				for (int i = 1; i < 4; i++)
				{
					find(hashvalue, x, y, root->m_children[idx^i], can_list, flag);
				}
			}
		}
	//}
}


CNode::CNode()
{
}

CNode::CNode(int leftUpperX, int leftUpperY, int rangeX, int rangeY, int depth)
{
	this->m_leftUpperX = leftUpperX;
	this->m_leftUpperY = leftUpperY;
	this->m_rangeX = rangeX;
	this->m_rangeY = rangeY;
	this->m_count = 0;
	this->m_depth = depth;
	for (int i = 0; i < 4; i++)
		m_children[i] = NULL;
}

void CNode::split()
{
	int leftX = m_leftUpperX;
	int midX = m_leftUpperX + m_rangeX / 2;
	int rightX = leftX + m_rangeX;

	int leftY = m_leftUpperY;
	int midY = m_leftUpperY + m_rangeY / 2;
	int rightY = leftY + m_rangeY;

	m_count = 0;

	m_children[0] = new CNode(leftX, leftY, m_rangeX / 2, m_rangeY / 2, m_depth + 1);
	m_children[1] = new CNode(midX, leftY, m_rangeX - m_rangeX / 2, m_rangeY / 2, m_depth + 1);
	m_children[2] = new CNode(leftX, midY, m_rangeX / 2, m_rangeY - m_rangeY / 2, m_depth + 1);
	m_children[3] = new CNode(midX, midY, m_rangeX - m_rangeX / 2, m_rangeY - m_rangeY / 2, m_depth + 1);

	for (map<unsigned int, Template*>::iterator it = this->m_data.begin(); it != this->m_data.end(); it++)
	{
		unsigned int uiHashvalue = it->first;
		Template* p = it->second;

		while (p != NULL)
		{
			Template* next = p->next;
			int x = p->x;
			int y = p->y;
			int idx = this->getIndex(x, y);
			map<unsigned int, Template*>& data = m_children[idx]->m_data;

			if (data.find(uiHashvalue) == data.end())
			{
				data[uiHashvalue] = p;
				p->next = NULL;
			}
			else
			{
				p->next = data[uiHashvalue];
				data[uiHashvalue] = p;
			}
			p = next;
		}
		it->second = NULL;
	}
	this->m_data.clear();
}
