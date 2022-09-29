#pragma once

// $Revision: 1.4 $
// $Date: 1990/08/09 14:33:13 $
#include "tkScrollBar.h"
#include "textFont.h"

// a scrollable list of names
class ScrollList : public tkView {
protected:
    tkScrollBar*	bar;
    int			eventName;
    int			numberOfNames;
    int			spaceForNames;
    int			firstVisibleName;
    int			viewingPixelOffset;
    int			beginSelectSlot;
    Bool		sorted;
    char**		names;
    char*		nameStatus;
    textFont*		font;

    // additions (john)
    int			currentSlot;
    Bool		postOnSelection;
    Bool		dragSelection;
    Bool		differentColor;

    // size of contents changed
    void sizeChanged();

    // pixels that the contents cover
    int contentHeight();

    // convert a view coordinate to a name slot
    int viewToName(int viewY);

    // find a name in the data structure
    int findName(char* name);

    // update the view based on a new scroll bar position
    void updateView();

public:
    ScrollList();
    ~ScrollList();

    //	where memory is actually freed. -- jice
    virtual void close();

    // set the scroll bar to use
    void setScrollBar(tkScrollBar* bar);

    // set the font to use
    void setFont(char *fontName);

    // add a name to the list
    void addName(char* name);

    // sort the names in the list
    void sort();

    // enable/disable a name
    void disableName(char* name);
    void enableName(char* name);

    // get name, given slot
    char* name(int slot);

    void select(int slot);
    void deselect(int slot);

    Bool isDisabled(int slot);

    // set the event to post when a name is picked
    void setPostEvent(int name);

// additional functions -- john ----------------------

    // return the last slot to be selected.
    int currentSelection() { return currentSlot; }
    char *currentName() { return name(currentSlot); }

    // if true, will not post selection events but instead will update self.
    void doPost(Bool postit) { postOnSelection = postit; }

    // continuous selection -- but only post on endSelect.
    void dragSelectOK(Bool dragit) { dragSelection = dragit; }

    void disabledColor(Bool different) { differentColor = different; }

    //
    virtual void continueSelect(Point&);

//----------------------------------------------------

    virtual void beginSelect(Point&);
    virtual void endSelect(Point&);
    virtual void setBounds(Box2 const&);
    virtual void paint();
    virtual void rcvEvent(tkEvent* e);
};
