#ifndef VKFORMAT_H
#define VKFORMAT_H

// VkFormat is reserved for application use.
// Note that each time it is called it overwrites the same private buffer.
// See the man page for more information.

const char * VkFormat(const char* fmt, ...);


// _VkFormat is for internal library use,
// so the library does not collide with aplication use of VkFormat.

const char * _VkFormat(const char* fmt, ...);


#endif

