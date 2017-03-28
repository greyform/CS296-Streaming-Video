
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

capture = cvCaptureFromCAM(0);//cvCaptureFromAVI("drop.avi");//

if( !capture )
    printf( "Error when reading steam_avi");

int param = CV_IMWRITE_JPEG_QUALITY;

	img = cvQueryFrame( capture );
	
	CvMat* encoded = cvEncodeImage(".jpg", img, &param );
	//cvShowImage("w",img);

	char c = cvWaitKey(10);
	if(c =='x') return NULL;
	
cvReleaseImage(&img);

return encoded;
}


void decode(CvMat * frame){
IplImage* img = cvDecodeImage(frame, CV_LOAD_IMAGE_COLOR);

cvNamedWindow( "w", CV_WINDOW_AUTOSIZE);

int fps = 60;//(int)cvGetCaptureProperty(capture, CV_CAP_PROP_FPS);
int frameW= (int)cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH);
int frameH= (int)cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT);
  CvVideoWriter *writer=cvCreateVideoWriter("camera60.avi", CV_FOURCC('M','J','P','G'),
                       fps,cvSize(frameW, frameH), 1);
cvShowImage("w",img);
	char c = cvWaitKey(10);
	if(c =='f'){
		cvWriteFrame(writer,img);
	}
	
	cvReleaseImage(&img);
	cvDestroyWindow("w");
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
