//
// VhelpApp.h
//
// class definition for VhelpApp, a sub-class of tkApp
//
#ifndef	_VhelpApp_
#define	_VhelpApp_

#include "tkApp.h"
#include "tkWindow.h"
#include "tkNotifier.h"
#include "textStyle.h"
#include "VhelpText.h"

#define TOPMARG		15.0	// Margin between top of window and top of text
#define SIDEMARG	15.0	// Margin between sides of window and text
#define BOTMARG		90.0	// Margin between bottom of window and text
#define OLW		3	// Outline width
#define BARWIDTH	21.0	// Scroll bar width: 1+21+1 (1 for margin)
// Indicies into tkFontMap
#define PLAIN_FONT	0
#define BOLD_FONT	1
#define ITALIC_FONT	2

class VhelpApp : public tkApp {
protected:
	tkWindow	*win;
	VhelpText	*helptext;
	tkButton	*quitb;
	tkButton	*nextb;
	tkButton	*prevb;
	tkNotifier	*notify;
	tkValueEvent	*buttInterest;
	tkValueEvent	*redrawInterest;
	tkValueEvent	*shutInterest;
	tkEvent		*delayEvent;
	textFont	*currfont;
	int		error;

	Bool		noquitmenu;
	float		def_xsiz;
	float		def_ysiz;
	float		def_xloc;
	float		def_yloc;
	float		default_fsize;
	float		twidth;
	char		*default_font;
	char		*section;
	char		*vhelptitle;
	char		*fileName;
	int		filenum;
	int		state;
	int		row;

	void setupbuttons();
	void readfile();
	void setuphelptext();
	void setupwindow();
	void setupfonts();
	void setupcolors();
	int  helptextformat(textFrame *, char *, int);
	char *StringCopy(char *);
	void postClose();
	void MakeQuitNotify(int, int);
public:
	VhelpApp(char *);
	VhelpApp(int );
	~VhelpApp();

	virtual void appInit( tkApp*, int, char*[], char*[] );
	virtual void rcvEvent(tkEvent*);
	void appClose();

	void SetQuitOption(Bool flag) { noquitmenu = flag; }
	void SetTitle(char *);
	void SetSection(char *);
	void SetPosition(float, float);
	void SetSize(float, float);
	void SetFontName(char *);
	void SetPointSize(float);
	void PopWindow();
};

/*
textStyle plain_style(PLAIN_FONT,  0, 1);
textStyle bold_style(BOLD_FONT,   0, 1);
textStyle italic_style(ITALIC_FONT, 0, 1);
textStyle select_style(PLAIN_FONT,  0, 2);
*/

#endif
