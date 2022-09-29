//////////////////////////////////////////////////////////////////////////////
// VkCommonDefs.h

#ifndef VKCOMMONDEFS_H
#define VKCOMMONDEFS_H

// Version history:
//	1.1		IRIX 5.3
//	1.3		IRIX 6.2 VkApp and release notes claimed this
//	1.4		IRIX 6.2 spec file erroneously claimed this.  We are
//				skipping it so everything can get back in sync.
//	1.5		Post IRIX 6.2 -- get back in step with the spec file
//	1.5.2		IRIX 6.5 Kudzu release
//	1.5.3		IRIX 6.5.2 release... [ Jasmine - Motif 1.2 ]
//	2.1.0		IRIX 6.5.2 release... [ Jasmine - Motif 2.1 ]
//
//////////////////////////////////////////////////////////////////////////////
#define _VK_MAJOR 1
#define _VK_MINOR 5
#define _VK_PATCH 3
#define _VK_STRING	"ViewKit Release: 1.5.3"

#define _VkVersion	((_Vk_MAJOR * 10000) + (_VK_MINOR * 100) + _VK_PATCH)
//////////////////////////////////////////////////////////////////////////////

#endif /* VKCOMMONDEFS_H */
