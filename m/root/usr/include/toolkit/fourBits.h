#ifndef __FOURBIT__
#define __FOURBIT__

extern int try16colors;
extern int numStdColors;
#ifdef __cplusplus
extern "C" {
extern int dithcount( int );
extern void nextdither( );
extern int chartocolorindex(int c,
			    int iconcolor, int shadowcolor, int outlinecolor,
			    int ditherindex);

}
#endif
extern int _di_;

/*
 * withSdtColor(arg)
 *  This macro assumes that pattern 0 is set when
 * entered and leaves things that way.
 *
 *  If called with a positive argument it will behave
 * the same as a color(arg) command.
 *
 *  If called with a negative argument it will try
 * and use a possibly dithered color to get as close
 * possible to the abs(arg)'th color in the StdColors
 * pallette.
 */
#define withStdColor(i)							\
	for(_di_ = dithcount(i); _di_ >= 0; nextdither(), _di_--)

#define ditherCI0(I) ((-(I)) & 0xF)
#define ditherCI1(I) (((-(I)) & 0xF0) >> 4)

#endif
