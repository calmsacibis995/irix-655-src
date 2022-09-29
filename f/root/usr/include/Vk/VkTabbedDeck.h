
//////////////////////////////////////////////////////////////
//
// Header file for VkTabbedDeck
//
//    This class is a ViewKit component, which should eventually
// be added to the ViewKit library.
// Normally, nothing in this file should need to be changed.
//
//////////////////////////////////////////////////////////////
#ifndef VKTABBEDDECK_H
#define VKTABBEDDECK_H
#include <Vk/VkComponent.h>

#define VkTABS_ON_BOTTOM 0
#define VkTABS_ON_LEFT   1
#define VkTABS_ON_TOP    2
#define VkTABS_ON_RIGHT  3

class VkTabbedDeck: public VkComponent {

  public:

    VkTabbedDeck(const char *, Widget);
    ~VkTabbedDeck();
    const char *className();

    Widget topWidget(); 
    VkComponent *topComponent();

    void setTabLocation(int location);

    void registerChild ( VkComponent*, const char *name);
    void registerChild ( Widget, const char *name);

    Widget deckParent();

    static const char *const tabPopupCallback;    

  protected:

    void pop ( VkCallbackObject *, void *, void *);
    
    class VkDeck *_vkdeck;
    class VkTabPanel *_tabs;

  private:

    // This class is not intended to be copied.  Strangers will get a
    // compile-time error.  Because there is no implementing code, friends
    // and inadvertant local use will get an ld-time error.  If a copy
    // constructor or operator= is ever needed, simply write them and
    // make these entries public.
    VkTabbedDeck(const VkTabbedDeck&);
    VkTabbedDeck &operator= (const VkTabbedDeck&);
};
#endif
