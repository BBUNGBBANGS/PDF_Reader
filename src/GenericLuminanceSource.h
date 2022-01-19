typedef struct 
{
    unsigned char * pixels;
    int left;
    int top;
    int width;
    int height;
    int rowBytes;
}GenericLuminanceSource_t;

unsigned char * GenericLuminanceSource(int left, int top, int width, int height, unsigned char * bytes, int rowBytes, int pixelBytes, int redIndex, int greenIndex, int blueIndex);
