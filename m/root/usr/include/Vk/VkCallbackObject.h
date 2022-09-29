////////////////////////////////////////////////////////////////////////////////
///////   Copyright 1992, Silicon Graphics, Inc.  All Rights Reserved.   ///////
//                                                                            //
// This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;     //
// the contents of this file may not be disclosed to third parties, copied    //
// or duplicated in any form, in whole or in part, without the prior written  //
// permission of Silicon Graphics, Inc.                                       //
//                                                                            //
// RESTRICTED RIGHTS LEGEND:                                                  //
// Use,duplication or disclosure by the Government is subject to restrictions //
// as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data     //
// and Computer Software clause at DFARS 252.227-7013, and/or in similar or   //
// successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -    //
// rights reserved under the Copyright Laws of the United States.             //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////


#ifndef VKCALLBACKOBJECT_H
#define VKCALLBACKOBJECT_H

#include <Vk/VkBase.h>
#if _VK_MAJOR > 1
// IMPORTANT:  need this for the definition of XrmQuark...
//
#include <Xm/Xm.h>
#endif

class VkCallbackObject;
class VkCallbackObjectList;
class VkCallbackList;
class VkNameList;

#define VkRNoArg    "NoArg"
#define VkRFilename "Filename"

typedef void (*VkCallbackFunction) (VkCallbackObject *caller,
				    void *clientData,
				    void *callData);

typedef void (VkCallbackObject::*VkCallbackMethod) (VkCallbackObject *caller,
						    void *clientData,
						    void *callData);

#define VkAddCallbackFunction(name, you, func, clientData) \
                    ( (you)->addCallback(name, func, clientData))
#define VkAddCallbackMethod(name, you, me, func, clientData) \
                   ( (you)->addCallback(name, (me), (VkCallbackMethod) func, clientData))

#define VkRemoveCallbackFunction(name, you, func, clientData) \
                  ( (you)->removeCallback(name, func, clientData))
#define VkRemoveCallbackMethod(name, you, me, func, clientData) \
                  ( (you)->removeCallback(name, (me), (VkCallbackMethod) func, clientData))


/*******************************************************
CLASS
    VkCallbackObject

    Base class for all ViewKit objects

OVERVIEW


******************************************************/

#if _VK_MAJOR > 1
class VkCallbackObject : public VkBase {
#else	
class VkCallbackObject {
#endif

 public:


    ////////////
    //
    //
    virtual ~VkCallbackObject();

    ////////////
    //
    //
    virtual const char *className();

    ////////////
    //
    //
    void addCallback(const char *, 
		     VkCallbackFunction, 
		     void *clientData = 0);

#if _VK_MAJOR > 1
    void addCallback(XrmQuark, 
		     VkCallbackFunction, 
		     void *clientData = 0);

    void addCallback(long, 
		     VkCallbackFunction, 
		     void *clientData = 0);
#endif
    ////////////
    //
    //
    void addCallback(const char *, 
		     VkCallbackObject *, 
		     VkCallbackMethod, 
		     void *clientData = 0);

#if _VK_MAJOR > 1
    void addCallback(XrmQuark, 
		     VkCallbackObject *, 
		     VkCallbackMethod, 
		     void *clientData = 0);

    void addCallback(long, 
		     VkCallbackObject *, 
		     VkCallbackMethod, 
		     void *clientData = 0);
#endif
    ////////////
    //
    //
    void removeCallback(const char *, 
			VkCallbackFunction, 
			void *clientData = 0);

#if _VK_MAJOR > 1
    void removeCallback(XrmQuark, 
			VkCallbackFunction, 
			void *clientData = 0);

    void removeCallback(long, 
			VkCallbackFunction, 
			void *clientData = 0);
#endif
    ////////////
    //
    //
    void removeCallback(const char *, 
			VkCallbackObject *, 
			VkCallbackMethod,
			void *clientData = 0);

#if _VK_MAJOR > 1
    void removeCallback(XrmQuark, 
			VkCallbackObject *, 
			VkCallbackMethod, 
			void *clientData = 0);

    void removeCallback(long, 
			VkCallbackObject *, 
			VkCallbackMethod, 
			void *clientData = 0);
#endif
    ////////////
    //
    //
    void removeAllCallbacks();

    ////////////
    //
    //
    void removeAllCallbacks(VkCallbackObject *);

    ////////////
    //
    //
    int hasCallbacks(const char *) const;

#if _VK_MAJOR > 1
    int hasCallbacks(XrmQuark) const;
    int hasCallbacks(long) const;
#endif

    ////////////
    //
    //
    void cloneCallbacks(VkCallbackObject *);


    ////////////
    //
    //
    void cloneCallback(const char * const, VkCallbackObject *);


    ////////////
    //
    //
    void cloneCallback(const char * const, const char * const,
		       VkCallbackObject *);

#if _VK_MAJOR > 1
    void cloneCallback(XrmQuark, XrmQuark,
		       VkCallbackObject *);
    void cloneCallback(long, long,
		       VkCallbackObject *);
#endif

    ////////////
    // loadObject supports dynamic loading
    // of objects, as supported by rapidapp. Objects must be
    // set up properly for this to work. See man VkComponent
    // and man VkCallbackObject or the rapidapp documentation for details.
    static VkCallbackObject *loadObject(const char *name,
					const char *className,
					const char *filename);


    ////////////
    //
    //
    const VkNameList* getMethods();

    ////////////
    //
    //
    const char *getMethodArgType(const char *methodName);

    ////////////
    //
    //
    void invokeMethod(const char *method, void *);

    ////////////
    //
    //
    void invokeMethod(const char *method, int);

    ////////////
    //
    //
    void invokeMethod(const char *method, float);    

    ////////////
    //
    //
    void invokeMethod(const char *method, unsigned char);

    ////////////
    //
    //
    void invokeMethod(const char *method, const char *);            

    ////////////
    //
    //
    void invokeMethod(const char *method);


    ////////////
    //
    //
    static const char *const updateCallback;
    
  protected:


    ////////////
    //
    //
    void callCallbacks(const char *const, void *);

#if _VK_MAJOR > 1
    void callCallbacks(XrmQuark, void *);
    void callCallbacks(long, void *);
#endif
    ////////////
    // Protected constructor
    VkCallbackObject( ); 

    ////////////
    // Protected copy constructor and operator=()
    // 
    // The only reason that these are provided is so that subclasses can
    // have their own versions if they choose to do so.  The default copy
    // constructor and operator=() clearly do not work.
    //
    // Using either this copy constructor or operator=() is not recommended.
    // If you do,  you are blazing a new trail.  It is debatable whether
    // anyone can tell me in plain English what (in general) copying or
    // operator=() of a VkCallbackObject even means.  If our wisdom in
    // this area expands, these calls may in time become better -- not
    // necessarily compatibly.
    //
    // VkCallbackObjectOperatorEqual does the real operator=() work.  Without
    // this, subclasses could not have operator=(), because they'd have no
    // way to get the VkCallbackObject opeator=() work done.
    //
    VkCallbackObject( const VkCallbackObject &); 
    VkCallbackObject &operator =(const VkCallbackObject&);
    VkCallbackObject &VkCallbackObjectOperatorEqual(const VkCallbackObject&);

    ////////////
    // Only to be called from VkComponent
    // Used by derived classes to describe methods for use in dynamic loading
    // Largely used by RapidApp, but could be useful in programs
    static void registerLoadedObject(VkCallbackObject*, void *, void *); 
    
    struct  InterfaceMap {
       char *resourceName;
       char *methodName;
       char *argType;
       char *definingClass;	
       void (VkCallbackObject::*method)(...);	
    };

  private:

    VkCallbackList        *_callbacks;
    VkCallbackObjectList  *_senders;

    void destroyVkCallbackObject(); 	// Used by destructor, and buy operator=
    void registerSender(VkCallbackObject *); 
    void unregisterSender(VkCallbackObject *); 
    void unregisterSenders(VkCallbackObject *);

    void findMethod(const char *);            
};

#endif

