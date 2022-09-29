/*
 * File generated by cset
 * (C) Abandoned 1998 klarz <klarz@svr-sun.ktg.gau.hu>
 * This file is based on ctype-latin2.c 
 *
 */

#include <global.h>
#include "m_string.h"

uchar NEAR ctype_hungarian[257] = {
0,
 32, 32, 32, 32, 32, 32, 32, 32, 32, 40, 40, 40, 40, 40, 32, 32,
 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
 72, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
132,132,132,132,132,132,132,132,132,132, 16, 16, 16, 16, 16, 16,
 16,129,129,129,129,129,129,  1,  1,  1,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, 16, 16, 16, 16, 16,
 16,130,130,130,130,130,130,  2,  2,  2,  2,  2,  2,  2,  2,  2,
  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 16, 16, 16, 16, 32,
 32, 32, 32, 32, 32, 32, 32, 32, 40, 40, 40, 40, 40, 32, 32, 32,
 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 72,
  1, 16,  1, 16,  1,  1, 16,  0,  0,  1,  1,  1,  1, 16,  1,  1,
 16,  2, 16,  2, 16,  2,  2, 16, 16,  2,  2,  2,  2, 16,  2,  2,
  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
 16,  1,  1,  1,  1,  1,  1, 16,  1,  1,  1,  1,  1,  1,  1, 16,
  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
  2,  2,  2,  2,  2,  2,  2, 16,  2,  2,  2,  2,  2,  2,  2, 16
};

uchar NEAR to_lower_hungarian[] = {
  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
 64, 97, 98, 99,100,101,102,103,104,105,106,107,108,109,110,111,
112,113,114,115,116,117,118,119,120,121,122, 91, 92, 93, 94, 95,
 96, 97, 98, 99,100,101,102,103,104,105,106,107,108,109,110,111,
112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
177,161,179,163,181,182,166,167,168,185,186,187,188,173,190,191,
176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
208,241,242,243,244,245,246,215,248,249,250,251,252,253,254,223,
224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255
};

uchar NEAR to_upper_hungarian[] = {
  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
 96, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90,123,124,125,126,127,
128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
176,160,178,162,180,164,165,183,184,169,170,171,172,189,174,175,
192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
240,209,210,211,212,213,214,247,216,217,218,219,220,221,222,255
};

uchar NEAR sort_order_hungarian[] = {
  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
 64, 65, 71, 72, 76, 78, 83, 84, 85, 86, 90, 91, 92, 96, 97,100,
105,106,107,110,114,117,122,123,124,125,127,131,132,133,134,135,
136, 65, 71, 72, 76, 78, 83, 84, 85, 86, 90, 91, 92, 96, 97,100,
105,106,107,110,114,117,122,123,124,125,127,137,138,139,140,  0,
  1,120, 79,  4,  5,  6,  7,  8,  9, 10,103,103, 86, 86, 15, 67,
 79, 18, 19,103,103,100,120,117,120,103,120, 28, 29, 30, 31,255,
 67, 86,100,117, 94,111,255,103,255,112,113,115,128,255,129,130,
255, 66,255, 93,255, 67,111,255,255,112,113,115,128,255,129,130,
108, 67, 68, 69, 70, 95, 73, 75, 74, 79, 81, 82, 80, 86, 87, 77,
255, 98, 99,100,102,103, 86,255,109,119,117,120,120,126,116,255,
108, 67, 68, 69, 70, 95, 73, 75, 74,117, 81,120, 80, 86, 88, 77,
255, 98, 99,100,102,103,103,255,109,119,117,120,120,126,116,255
};
