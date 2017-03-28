
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <cv.h>
#include <highgui.h>
#include <ml.h>
#include <cxcore.h>

CvMat* encode(){
	CvCapture* capture=0;
	IplImage* img=0;

capture = cvCaptureFromAVI("drop.avi");//

if( !capture )
    printf( "Error when reading steam_avi");

const static int encodeParams[] = { CV_IMWRITE_JPEG_QUALITY, 95 };

img = cvQueryFrame( capture );

return cvEncodeImage(".jpg", img, encodeParams);
}


void decode(CvMat * frame){
cvShowImage( "w", frame );
}

int main(int argc, char** argv)
{ cvNamedWindow( "w", CV_WINDOW_AUTOSIZE);
	while(1){
		decode(encode());
	}
	cvDestroyWindow("w");
	return 0;
}
////////////////////////////////////////////////////////////////////////////////
/*int main(int argc, char** argv)
{
CvCapture* capture=0;
IplImage* img=0;

capture = cvCaptureFromCAM(0);//cvCaptureFromAVI("drop.avi");//

if( !capture )
    printf( "Error when reading steam_avi");

cvNamedWindow( "w", CV_WINDOW_AUTOSIZE);
int num = 0;
int x = 1;
char s[20];

int fps = 60;//(int)cvGetCaptureProperty(capture, CV_CAP_PROP_FPS);
int frameW= (int)cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH);
int frameH= (int)cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT);
  CvVideoWriter *writer=cvCreateVideoWriter("camera60.avi", CV_FOURCC('M','J','P','G'),
                       fps,cvSize(frameW, frameH), 1);

//int param = CV_IMWRITE_JPEG_QUALITY;
while(1)
{
	img = cvQueryFrame( capture );
	cvShowImage("w",img);

	char c = cvWaitKey(10);
	if(c =='x') break;
	if(c =='f'){
		sprintf(s, "frame%d.jpg", num);
		cvSaveImage(s, img, &x);
		
	}
	//img = cvLoadImage(s, 1);
	cvWriteFrame(writer,img);

}
cvReleaseImage(&img);
cvDestroyWindow("w");
cvReleaseVideoWriter(&writer);
}*/
