#pragma once

// $Revision: 1.11 $
// $Date: 1990/08/09 14:36:37 $
#include "tkRenderItem.h"
#include "textFont.h"

#define	TOPALIGNED	1
#define BOTALIGNED	2
#define	LEFTALIGNED	3
#define	RIGHTALIGNED	4
#define	CENTERED	5
#define CAP_CENTERED	6

#define	DEFAULTFONT		"Iris14"

class textFont;
class CharString;

class tkLabel : public tkRenderItem {
protected:
	char*		text;
	Box2 		bgArea;
	textFont*	font;
	unsigned char	vjust;
	unsigned char	hjust;
	Bool		haveBBOX;

	void	computeBoundingBox();

public:
	tkLabel() { /* make junk */ }
	tkLabel(Box2 const& a, tkPen const& p, CharString& str);
	tkLabel(Box2 const& a, tkPen const& p, char* str);
	tkLabel(FILE* strm);
	~tkLabel();

	void setstring( char* s );
	void setfont( char* str, float size );
	void setfont( textFont* nf ) { delete font; font = nf; haveBBOX=FALSE;}
	textFont* getfont() { return font; }
	void setjust ( short hj = LEFTALIGNED, short vj = BOTALIGNED )
		{ hjust = hj, vjust = vj, haveBBOX = FALSE; }
	
	virtual void changeArea( Box2 const& );
	virtual void draw();
	virtual void getBoundingBox(Box2& result);

	virtual const char* className();
};
