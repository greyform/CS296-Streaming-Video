/*#include <stdio.h>
#include <opencv2/opencv.hpp>

using namespace cv;

int main(int argc, char** argv )
{
    if ( argc != 2 )
    {
        printf("usage: DisplayImage.out <Image_Path>\n");
        return -1;
    }

    Mat image;
    image = imread( argv[1], 1 );

    if ( !image.data )
    {
        printf("No image data \n");
        return -1;
    }
    namedWindow("Display Image", WINDOW_AUTOSIZE );
    imshow("Display Image", image);

    waitKey(0);

    return 0;
}*/
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <cv.h>
#include <highgui.h>
#include <ml.h>
#include <cxcore.h>

/*int main(int argc, char *argv[])
{
    CvCapture* cam = cvCaptureFromCAM(0); // open the default camera

    cvNamedWindow("mainWin", CV_WINDOW_AUTOSIZE); 
    cvMoveWindow("mainWin", 100, 100);
    int i;
	 for(;;){
			IplImage* img = 0; 
			if(!cvGrabFrame(cam)){              // capture a frame 
				printf("Could not grab a frame\n\7");
				exit(0);
			}
			img=cvRetrieveFrame(cam, i);           // retrieve the captured frame
			cvShowImage("mainWin",img);
			cvReleaseImage(&img );
			if(cvWaitKey(10) == 27) break;
			i++;
		}
	cvDestroyWindow("mainWin");

    // the camera will be deinitialized automatically in VideoCapture destructor
    return 0;
}*/
int main(int argc, char** argv)
{
CvCapture* capture=0;
IplImage* img=0;
capture =cvCaptureFromAVI("drop.avi");// cvCaptureFromCAM(0); 
if( !capture )
    printf( "Error when reading steam_avi");

cvNamedWindow( "w", CV_WINDOW_AUTOSIZE);
int num = 0;
int x = 1;
char s[20];

while(1)
{
	num++;
	img = cvQueryFrame(capture);
	if(!img) {break;}
	cvShowImage("w",img);
	char c = cvWaitKey(10);
	if(c =='x') break;
	if(c =='f'){
		sprintf(s, "frame%d.jpg", num);
		cvSaveImage(s, img, &x);
	}
    /*frame = cvQueryFrame( capture );
    if(!frame)
        break;
    cvShowImage("w", frame);*/
}
//cvReleaseImage(&img);
cvDestroyWindow("w");
}
