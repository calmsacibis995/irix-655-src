
//////////////////////////////////////////////////////////////
//
// Header file for BulletinBoard
//
//    This file is generated by RapidApp 1.2
//
//    This class is derived from BulletinBoardUI which 
//    implements the user interface created in 
//    RapidApp. This class contains virtual
//    functions that are called from the user interface.
//
//    When you modify this header file, limit your changes to those
//    areas between the "//---- Start/End editable code block" markers
//
//    This will allow RapidApp to integrate changes more easily
//
//    This class is a ViewKit user interface "component".
//    For more information on how components are used, see the
//    "ViewKit Programmers' Manual", and the RapidApp
//    User's Guide.
//////////////////////////////////////////////////////////////
#ifndef BULLETINBOARD_H
#define BULLETINBOARD_H
#include "BulletinBoardUI.h"
//---- Start editable code block: headers and declarations
#include <sys/numa_stats.h>

#define HIST_LEN 20    // Amount of history used to calculate moving average
//---- End editable code block: headers and declarations


//---- BulletinBoard class declaration

class BulletinBoard : public BulletinBoardUI
{

  public:

    BulletinBoard ( const char *, Widget );
    BulletinBoard ( const char * );
    ~BulletinBoard();
    const char *  className();
    virtual void setDiff10(Widget, XtPointer);
    virtual void setDiff100(Widget, XtPointer);
    virtual void setDiff1000(Widget, XtPointer);
    virtual void setInterval10(Widget, XtPointer);
    virtual void setInterval100(Widget, XtPointer);
    virtual void setInterval1000(Widget, XtPointer);
    virtual void setTotal(Widget, XtPointer);

    static VkComponent *CreateBulletinBoard( const char *name, Widget parent ); 

    //---- Start editable code block: BulletinBoard public
    virtual void Update(int force = 0);
    virtual void SumHistory(int node, numa_stats_t *numa_stats);
    virtual void CalcAvg(numa_stats_t *numa_stats);

    //---- End editable code block: BulletinBoard public



  protected:


    // These functions will be called as a result of callbacks
    // registered in BulletinBoardUI

    virtual void nodeSelect ( Widget, XtPointer );

    //---- Start editable code block: BulletinBoard protected
    virtual int getNodeNames();

    //---- End editable code block: BulletinBoard protected



  private:

    static void* RegisterBulletinBoardInterface();

    //---- Start editable code block: BulletinBoard private
    char *currentNode;
    int currentNodeNum;
    XmString *nodeNames;
    numa_stats_t *numa_stats_old;
    int numNodes;
    int scaleVal;  // factor used to determine amount of change on meters
    numa_stats_t *numa_stats_hist[HIST_LEN];  // keep track of history for moving average
    int currentEntry;  // current entry in numa_stats_hist
    int totalFlag;  // 0 if stats for one node are to be shown, 1 if total stats

    //---- End editable code block: BulletinBoard private


};
//---- Start editable code block: End of generated code


//---- End editable code block: End of generated code

#endif
