#ifndef _QUADTREE_
#define _QUADTREE_

#include <map>
#include <list>
#include <unordered_set>
#include <utility>
using namespace std;

#define BACK_LEVEL 3


typedef struct tagTemplate
{
	int x;
	int y;
	int t;
	tagTemplate* next;
	tagTemplate(int i, int j) :x(i), y(j),t(0),next(NULL) {};
}Template;


class CNode
{
public:
	map<unsigned int, Template*> m_data;
	unordered_set<unsigned int> m_hashSet;
	CNode* m_children[4];
	int m_leftUpperX;
	int m_leftUpperY;
	int m_rangeX;
	int m_rangeY;
	int m_count;
	int m_depth;

public:
	CNode();
	CNode(int leftUpperX, int leftUpperY, int rangeX, int rangeY, int depth);
	void split();
	int getIndex(int x, int y);
};

class CQuadTree
{
public:
	int m_maxDepth;
	int m_nodeCapacity;
	CNode* m_root;
	int m_rangeX;
	int m_rangeY;
	int m_leafDepth;

public:
	CQuadTree();
	CQuadTree(int uiMaxDepth, int uiNodeCapacity, int uiRangeX, int uiRangeY);
	~CQuadTree();
	void insertPoint(int x, int y, int hashvalue);
	void deletePoint();
	void dfsFreeNode(CNode* root);
	//Template*& find(int hashvalue, int orgX, int orgY, bool &flag);
	void find(int hashvalue, int orgX, int orgY, vector<Template**>& can_list, bool& flag);


protected:
	void insertPoint(int x, int y, int hashvalue, CNode* root);
	//Template*& find(int hashvalue, int x, int y, CNode* root, bool &flag);
	void find(int hashvalue, int x, int y, CNode * root, vector<Template**>& can_list, bool& flag);



};

#endif // !_QUADTREE_
