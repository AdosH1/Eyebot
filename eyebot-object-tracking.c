#include "eyebot.h"
#include <stdio.h>
#include "math.h"

/* QQVGA is 160x120 resolution*/
const int width = 160 * 3;
const int height = 120;
/* Turns out QQVGA and QQVGA_SIZE are constants implicitly */
double avg = 0;
double hues[9];
BYTE bitmap[QQVGA_SIZE];
int histX[160];
int histY[120];

double hueStart = 0;
double hueEnd = 0;
double threshold = 0.1;
double max_hue = 1;

BYTE img[QQVGA_SIZE];
double hsi[QQVGA_SIZE];

// Returns max of RGB
BYTE Max(BYTE R, BYTE G, BYTE B)
{
    if (R >= G && R >= B) return R;
    if (G >= R && G >= B) return G;
    return B;
}

// Returns min of RGB
BYTE Min(BYTE R, BYTE G, BYTE B)
{
    if (R <= G && R <= B) return R;
    if (G <= R && G <= B) return G;
    return B;
}

// Returns hue value between 0 - 1
double Hue(BYTE R, BYTE G, BYTE B)
{
    double r = R / 255.0;
    double g = G / 255.0;
    double b = B / 255.0;
    double M = fmax(r, fmax(g, b));
    double m = fmin(r, fmin(g, b));
    double C = M - m;
    
    double h = 0;
    if(C == 0) h = 0;
    else if(M == r) h = fmod((g-b)/C, 6) / 6;
    else if(M == g) h = ((b-r)/C + 2) / 6;
    else if(M == b) h = ((r-g)/C + 4) / 6;
    return h;
}

// Returns saturation value between 0 - 1
double Saturation(BYTE R, BYTE G, BYTE B)
{
    double min = Min(R,G,B);
    double sum = R + G + B;
    double div = 3 / sum;
    double result = (1 - ( div * min));
    
    return result;
}

// Returns intensity value between 0 - 1
double Intensity(BYTE R, BYTE G, BYTE B)
{
    double r = R / 255.0;
    double g = G / 255.0;
    double b = B / 255.0;
    
    double sum = r;
    sum += g;
    sum += b;
    return sum / 3;
}

// Returns average of the center 3x3 hues
double Average()
{
    double sum = 0;
    int num = 0;
    for(int i = 0; i < 9; i++)
    {
        sum += hues[i];
        num++;
    }
    double avg = (sum / num);
    
    hueStart = fmod(avg - threshold + max_hue, max_hue);
    hueEnd = fmod(avg + threshold +  max_hue, max_hue);
    return avg;
}

// Populates the hues array with the center 3x3 colours
void GetCenterHues()
{
    int hueIndex = 0;
    for(int i = 79; i <= 81; i++) 
    {
        for(int j = 59; j <= 61; j++) 
        {
            int index = i*3 + (j * width);
            hues[hueIndex] = Hue(img[index], img[index+1], img[index+2]);
            hueIndex++;
        }
    }
    return;
}

// Draws a green crosshair where the program sees has the highest density of colour matches in the x and y axis
void DrawHistogramFocus(int xIndex, int yIndex)
{
    for (int i = 0; i < width; i+=3) {
        for (int j = 0; j < height; j++) {
            if (i == xIndex * 3)
            {
                /* Vertical line */
                int index = i + (j * width);
                bitmap[index] = 0; 
                bitmap[index + 1] = 255; 
                bitmap[index + 2] = 0; 
            }            
            if (j == yIndex)
            {
                /* Horizontal line */
                int index = i + (j * width);
                bitmap[index] = 0; 
                bitmap[index + 1] = 255; 
                bitmap[index + 2] = 0; 
            }         
        }
    }
}

// Draws a small green crosshair so we can see where the center of the camera is pointing at
void DrawCrosshair()
{
    for (int i = 0; i < width; i+=3) 
    {
        for (int j = 0; j < height; j++) 
        {
            if ((i == width / 2) && 
                (j > (height / 2) - 10) && 
                (j < (height / 2) + 10))
            {
                /* Vertical line */
                int index = i + (j * width);
                img[index] = 0; 
                img[index + 1] = 255; 
                img[index + 2] = 0; 
            }            
            if ((j == height / 2) && 
                (i > (width / 2) - 30) && 
                (i < (width / 2) + 30))
            {
                /* Horizontal line */
                int index = i + (j * width);
                img[index] = 0; 
                img[index + 1] = 255; 
                img[index + 2] = 0; 
            }         
        }
    }
}

// Updates the histograms that measure highest density of matching hues on the x and y axis.
void UpdateHistograms()
{
    int Index = 0;
    int Count = 0;
    
    for (int j = 0; j < height; j++) 
    {
        Count = 0;
        for (int i = 0; i < width; i+=3) 
        {
            if (bitmap[i + (j * width)] == 255) Count++;
        }
        histY[Index] = Count;
        Index++;
    }
    
    Index = 0;
    Count = 0;
    
    for (int i = 0; i < width; i+=3) 
    {
        Count = 0;
        for (int j = 0; j < height; j++) 
        {
            if (bitmap[i + (j * width)] == 255) Count++;
        }
        histX[Index] = Count;
        Index++;
    }
}

// Returns the max number in an array
int MaxArray(int arr[], int size)
{
    int max = -9999;
    
    for (int i = 0; i < size; i++)
    {
        if (arr[i] > max) max = arr[i];
    }
    return max;
}

// Returns the index of the max number in an array
int MaxIndex(int arr[], int max, int size)
{
    int index = -1;
    
    for (int i = 0; i < size; i++)
    {
        if (arr[i] == max) 
        {
            index = i;
            break;
        }
    }
    return index;
}

// Rotates the car to keep our focused object in the center
void RotateCar(int index)
{
    if (index < 32) VWSetSpeed(0,30);
    else if (index >= 32 && index < 64) VWSetSpeed(0,30);
    else if (index >= 64 && index < 75) VWSetSpeed(0,30);
    else if (index >= 85 && index < 96) VWSetSpeed(0,-30);
    else if (index >= 96 && index < 138) VWSetSpeed(0,-30);
    else if (index >= 138) VWSetSpeed(0,-30);
    else VWSetSpeed(0,0);
}

// Checks if hue value requires wrapping eg. red is between ~330 - 30 degrees
bool checkHues(int start, int end, int hue)
{
    if (start > end) return (hue >= start) || (hue <= end);
    else return (hue >= start) && (hue <= end);
}

int main()
{
    /* Initialise */
    LCDMenu("Set Hue", "", "", "");
    CAMInit(QQVGA);
    
    /* Camera will try track the colour of the first object in-front on startup */
    GetCenterHues();
    avg = Average();
    
    while(1)
    {
        /* Returns rgb array into img[] */
        CAMGet(img); 
        
        /* RGB -> HSI conversion */
        for (int i = 0; i < width; i+=3) {
            for (int j = 0; j < height; j++) {
                
                int index = i + (j * width);
                hsi[index] = Hue(img[index], img[index+1], img[index+2]);
                hsi[index+1] = Saturation(img[index], img[index+1], img[index+2]);
                hsi[index+2] = Intensity(img[index], img[index+1], img[index+2]);
            }
        }
        
        /* HSI -> Bitmap conversion */
        for (int i = 0; i < width; i+=3) {
            for (int j = 0; j < height; j++) {
                int index = i + (j * width);
                if (checkHues(hueStart, hueEnd, hsi[index]) && hsi[index+1] > 0.4 && hsi[index+2] > 30) 
                {
                    bitmap[index] = 255;
                    bitmap[index + 1] = 255;
                    bitmap[index + 2] = 255;
                }
                else 
                {
                    bitmap[index] = 0;
                    bitmap[index+1] = 0;
                    bitmap[index+2] = 0;
                }
            }
        }
        
        /* Determine the location of the greatest density of matching hues */
        UpdateHistograms();
        int maxX = MaxArray(histX, 160);
        int maxY = MaxArray(histY, 120);
        int indexX = MaxIndex(histX, maxX, 160);
        int indexY = MaxIndex(histY, maxY, 120);
        DrawHistogramFocus(indexX, indexY);
        
        /* Rotate car to keep object in the center */
        RotateCar(indexX);

        /* Draw camera feed, bitmap and crosshairs */
        DrawCrosshair();
        LCDImageStart(0, 0, width / 3, height);
        LCDImage(img);
        LCDImageStart(width / 3, 0, width / 3, height);
        LCDImage(bitmap);
        
        
        usleep(33333);
    }
    
    CAMRelease();
    return 0;
}