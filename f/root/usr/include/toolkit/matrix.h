//typedef float Matrix[4][4];
extern void dumpMatrix( char*, Matrix );
extern void copyMatrix( Matrix, Matrix );
extern void translateMatrix( Matrix, float, float, float );
extern void rotateMatrix( Matrix, float, char );
extern void scaleMatrix( Matrix, float, float, float );
