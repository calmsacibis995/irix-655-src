#ifndef	_linkList_
#define	_linkList_
// $Revision: 1.3 $
// $Date: 1990/07/18 14:33:44 $

struct linkItem {
	linkItem* next;
	linkItem* prev;

	// place other after this
	void appendItem(linkItem* other);

	// place other before this
	void insertItem(linkItem* other);

	// remove other from its list; doesn't update list head/tail
	void unlinkItem();
};

struct linkList {
	linkItem* head;
	linkItem* tail;

	// unlink the given item from its list
	void unlink(linkItem* item);

	// append the given item to the tail of the list
	void append(linkItem* item);

	// insert the given item at the head of the list
	void insert(linkItem* item);

	// append itemB after itemA
	void appendAfter(linkItem* itemA, linkItem* itemB);

	// insert itemA before itemB
	void insertBefore(linkItem* itemA, linkItem* itemB);

	// fix the tail pointer
	void fixTail();
};
#endif
