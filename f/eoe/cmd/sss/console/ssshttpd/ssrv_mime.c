/* --------------------------------------------------------------------------- */
/* -                             SSRV_MIME.C                                 - */
/* --------------------------------------------------------------------------- */
/*                                                                             */
/* Copyright 1992-1998 Silicon Graphics, Inc.                                  */
/* All Rights Reserved.                                                        */
/*                                                                             */
/* This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;      */
/* the contents of this file may not be disclosed to third parties, copied or  */
/* duplicated in any form, in whole or in part, without the prior written      */
/* permission of Silicon Graphics, Inc.                                        */
/*                                                                             */
/* RESTRICTED RIGHTS LEGEND:                                                   */
/* Use, duplication or disclosure by the Government is subject to restrictions */
/* as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data      */
/* and Computer Software clause at DFARS 252.227-7013, and/or in similar or    */
/* successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -     */
/* rights reserved under the Copyright Laws of the United States.              */
/*                                                                             */
/* --------------------------------------------------------------------------- */
#include <string.h>

/* --------------------------------------------------------------------------- */
/* String pair structure                                                       */
typedef struct s_sss_pair_string {
 const char *name;
 const char *val;
} PSTRING;
/* --------------------------------------------------------------------------- */
static const char szMimeText_Plain[] = "text/plain";
static const char szMimeText_Html[]  = "text/html";
static const char szMimeImage_Gif[]  = "image/gif";
static const char szMimeImage_Jpeg[] = "image/jpeg";
/* --------------------------------------------------------------------------- */
static const char szExtHtml[]        = "html";
static const char szExtHtm[]         = "htm";
/* --------------------------------------------------------------------------- */
/* Mime table                                                                  */
static PSTRING mimeTable[] = {
 { szMimeText_Plain,            "txt" },
 { szMimeText_Html,             szExtHtml },
 { szMimeText_Html,             szExtHtm },
 { szMimeImage_Gif,             "gif" },
 { szMimeImage_Jpeg,            "jpg" },
 { szMimeImage_Jpeg,            "jpeg" },
 { "image/x-bitmap",            "xbm" },
 { "image/ief",                 "ief" },
 { "image/x-png",               "png" },
 { "image/x-portable-pixmap",   "ppm" },
 { "image/tiff",                "tiff" },
 { "image/x-xpixmap",           "xpm" },
 { "image/x-portable-anymap",   "pnm" },
 { "image/x-portable-bitmap",   "pbm" },
 { "image/x-portable-graymap",  "pgm" },
 { "image/x-rgb",               "rgb" },
 { "image/x-MS-bmp",            "bmp" },
 { "image/x-photo-cd",          "pcd" },
 { "application/fractals",      "fif" },
 { "video/x-msvideo",           "avi" },
 { "video/quicktime",           "mov" },
 { "video/mpeg",                "mpeg" },
 { "video/x-mpeg2",             "mpv2" },
 { "audio/x-wav",               "wav" },
 { "audio/x-aiff",              "aif" },
 { "audio/basic",               "au" },
 { "audio/x-pn-realaudio",      "ra" },
 { "audio/x-mpeg",              "mpa" },
 { "x-world/x-vrml",            "wrl" },
 { "model/vrml",                "vrml" },
 { "application/pdf",           "pdf" },
 { "application/rtf",           "rtf" },
 { "application/x-tex",         "tex" },
 { "application/x-latex",       "latex" },
 { "application/dvi",           "dvi" },
 { "application/x-textinfo",    "texi" },
 { "application/msword",        "doc" },
 { "image/x-cmu-raster",        "ras" },
 { 0,                           0 } };

/* --------------------- ssrvFindContentTypeByExt ---------------------------- */
const char *ssrvFindContentTypeByExt(const char *ext)
{ int i;
  if(ext)
  { while(*ext == '.') ext++;
    if(*ext)
    { for(i = 0;mimeTable[i].name;i++)
       if(!strcasecmp(mimeTable[i].val,ext)) return mimeTable[i].name;
    }
  }
  return "application/octet-stream";
}
/* ------------------- ssrvFindContentTypeByFname ---------------------------- */
const char *ssrvFindContentTypeByFname(const char *fname)
{ char *c;
  if(fname && fname[0])
  { if((c = strrchr(fname,(int)'.')) != 0) return ssrvFindContentTypeByExt((const char*)c);
  }
  return 0;
}
/* --------------------- ssrvIsHtmlResource ---------------------------------- */
int ssrvIsHtmlResource(const char *fname)
{ char *c,*q,tmp[16];
  int i,retcode = 0;
  if(fname && fname[0])
  { if((q = strchr(fname,(int)'?')) != 0) *q = 0;
    if((c = strrchr(fname,(int)'.')) != 0)
    { if(*(++c))
      { for(i = 0;c[i] && c[i] != ' ' && c[i] != '\t' && i < sizeof(tmp)-1;i++) tmp[i] = c[i];
	tmp[i] = 0;
        if(!strcasecmp(tmp,szExtHtml) || !strcasecmp(tmp,szExtHtm)) retcode++;
      }
    }
    if(q) *q = '?';
  }
  return retcode;
}
