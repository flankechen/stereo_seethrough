// stereo_seethrough.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <opencv2\opencv.hpp>
#include <opencv2\core\core.hpp>
#include <opencv2\highgui\highgui.hpp>

#define FRAME_WIDTH 960
#define FRAME_HEIGHT 1080
#define RENDER_WIDTH 1920
#define RENDER_HEIGHT 1080

//#define CAMERA_FLIP

using namespace std;
using namespace cv;


int main(int argc, char* argv[])
{
	int index_1;
	int index_2;

	if ( argc != 3 ) /* argc should be 2 for correct execution */
	{
		/* We print argv[0] assuming it is the program name */
		printf( "usage: stereo_seethrough.exe [camera index 0] [camera index 1], \n example: PTAM.exe 0 1\n please, do not use nagative index");
		system("pause");
		return 0;

		//index_1 = 0;
		//index_2 = 1;
	}
	else
	{
		index_1 = atoi(argv[1]);
		index_2 = atoi(argv[2]);
		cout<<"index :"<<index_1<<"  "<<index_2<<endl;
	}

	//read calibration parameters
	Mat mx1,my1,mx2,my2;
	mx1.create( FRAME_HEIGHT, FRAME_WIDTH, CV_16S);
	my1.create( FRAME_HEIGHT, FRAME_WIDTH, CV_16S);
	mx2.create( FRAME_HEIGHT, FRAME_WIDTH, CV_16S);
	my2.create( FRAME_HEIGHT, FRAME_WIDTH, CV_16S);
	Rect validRoi[2];

	FileStorage fs("./save_param/calib_para.yml", CV_STORAGE_READ);
	fs["MX1"] >> mx1;
	fs["MX2"] >> mx2;
	fs["MY1"] >> my1;
	fs["MY2"] >> my2;
	fs.release();

	Mat validroi1,validroi2;
	cv::FileStorage fs_roi("./save_param/validRoi.yml" ,CV_STORAGE_READ);
	if (fs_roi.isOpened())
	{
		fs_roi["validRoi1"] >> validroi1;
		fs_roi["validRoi2"] >> validroi2;
	}
	fs_roi.release();

	/*image valid boundary after image rectification*/
	int image_chop_left,image_chop_right,image_chop_up,image_chop_down; 

	image_chop_up = MAX(validroi1.at<int>(0,1),validroi2.at<int>(0,1));
	image_chop_down = MAX(validroi1.at<int>(0,3),validroi2.at<int>(0,3));
	image_chop_left = MAX(validroi1.at<int>(0,0),validroi2.at<int>(0,0));
	image_chop_right = MAX(validroi1.at<int>(0,2),validroi2.at<int>(0,2));

	//video capture setup
	Mat frame,frame_right;
	Mat frame_crop(RENDER_HEIGHT,RENDER_WIDTH/2,CV_8UC3);
	Mat frame_crop_right(RENDER_HEIGHT,RENDER_WIDTH/2,CV_8UC3);
	Mat stitch_left_right(RENDER_HEIGHT, RENDER_WIDTH, CV_8UC3);
	Mat frame_rectify(frame.size(), CV_8UC3);
	Mat frame_rectify_right(frame_right.size(), CV_8UC3);

	VideoCapture cap(index_1);
	VideoCapture cap_right(index_2);
	if(!cap.isOpened() || !cap_right.isOpened()) return -1;
	cap.set(CV_CAP_PROP_FOURCC ,CV_FOURCC('M', 'J', 'P', 'G') );
	cap.set(CV_CAP_PROP_FRAME_WIDTH,FRAME_WIDTH);
	cap.set(CV_CAP_PROP_FRAME_HEIGHT,FRAME_HEIGHT);
	cap_right.set(CV_CAP_PROP_FOURCC ,CV_FOURCC('M', 'J', 'P', 'G') );
	cap_right.set(CV_CAP_PROP_FRAME_WIDTH,FRAME_WIDTH);
	cap_right.set(CV_CAP_PROP_FRAME_HEIGHT,FRAME_HEIGHT);

	//namedWindow("camera_crop",1);
	//namedWindow("camera2_crop",1);
	namedWindow("stitch",0);


	for(;;)
	{
		double tic=(double)cvGetTickCount();
		cap>>frame;
		cap_right>>frame_right;

		if(frame.empty() || frame_right.empty())
			continue;

#ifdef CAMERA_FLIP
		flip(frame.t(), frame, 0);
		flip(frame_right.t(), frame_right, 0);
#endif
		remap(frame, frame_rectify, mx1, my1, CV_INTER_LINEAR);
		remap(frame_right, frame_rectify_right, mx2, my2, CV_INTER_LINEAR);

		//frame_rectify = frame_rectify.adjustROI(-image_chop_up, -image_chop_down, -image_chop_left, -image_chop_right).clone();
		//resize(frame_rectify,frame_rectify,frame.size(),INTER_LINEAR );
		//frame_rectify_right = frame_rectify_right.adjustROI(-image_chop_up, -image_chop_down, -image_chop_left, -image_chop_right).clone();
		//resize(frame_rectify_right,frame_rectify_right,frame_right.size(),INTER_LINEAR );

		resize(frame_rectify,frame_crop,frame_crop.size());
		resize(frame_rectify_right,frame_crop_right,frame_crop_right.size());

		//480*2 == 1920/2


		/*resize(frame_rectify, frame_rectify, frame_rectify.size()*2);
		resize(frame_rectify_right, frame_rectify_right, frame_rectify_right.size()*2);

		frame_rectify(Rect(0, (frame_rectify.rows - RENDER_HEIGHT)/2, RENDER_WIDTH/2, RENDER_HEIGHT)).copyTo(frame_crop);
		frame_rectify_right(Rect(0, (frame_rectify_right.rows - RENDER_HEIGHT)/2, RENDER_WIDTH/2, RENDER_HEIGHT)).copyTo(frame_crop_right);*/


		frame_crop.copyTo(stitch_left_right(Rect(0,0,RENDER_WIDTH/2,RENDER_HEIGHT)));
		frame_crop_right.copyTo(stitch_left_right(Rect(RENDER_WIDTH/2,0,RENDER_WIDTH/2,RENDER_HEIGHT)));

		//Size new_size(frame_rectify.size().width*1.4487, frame_rectify.size().height*1.4486);
		//resize(frame_rectify,frame_rectify,new_size );
		//resize(frame_rectify_right,frame_rectify_right,new_size);

		//frame_rectify.copyTo(stitch_left_right(Rect(132,76,new_size.width,new_size.height)));
		//frame_rectify_right.copyTo(stitch_left_right(Rect(1092,76,new_size.width,new_size.height)));



		double toc=(double)cvGetTickCount();
		double detectionTime = (toc-tic)/((double) cvGetTickFrequency()*1000);
		cout << " frame spend: " << detectionTime << endl;

		//imshow("camera", frame_right);
		//imshow("camera2", frame);
		//imshow("camera_crop",frame_crop);
		//imshow("camera2_crop",frame_crop_right);
		imshow("stitch", stitch_left_right);
		if(waitKey(3.3) >= 0) break;

	}

	return 0;
}

