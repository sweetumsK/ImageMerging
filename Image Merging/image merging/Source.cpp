#include <cstdio>
#include <algorithm>
#include <string>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <json/json.h>

extern "C" {
#include <qrencode.h>
}

#include"types.hpp"

using namespace std;
using namespace cv;
using namespace Json;

class Item{
private:
	Mat img, grayImg, binaryImg, cutImg, transformImg;
	myPoint leftupside, center, leftcorner, rightcorner;
	mySize imgsize;
	int rows, cols;
	double rateLimit;

public:

	void setImg(string Path) {
		Mat temp;
		temp = imread(Path);
		img.create(temp.size(), CV_8UC4);
		cvtColor(temp, img, CV_BGR2BGRA);
		rows = img.rows;
		cols = img.cols;
	}

	void setLeftUpSide(int y_, int x_) {
		leftupside.x = x_;
		leftupside.y = y_;
	}

	void setSize(int y_, int x_) {
		imgsize.height = x_;
		imgsize.width = y_;
	}

	//void setIndex(int index_) {	index = index_;	}
	void setRate(double rate_) { rateLimit = rate_;	}
	void setCenter() {
		center.x = leftupside.x + imgsize.height / 2;
		center.y = leftupside.y + imgsize.width / 2;
	}

/*
	void cutOut() {
		vector<vector<Point>> contours;
		vector<Vec4i> hierarchy;
		cvtColor(img, grayImg, CV_RGB2GRAY);
		threshold(grayImg, binaryImg, 245, 255, CV_THRESH_BINARY_INV);
		//threshold(grayImg, binaryImg, 230, 255, CV_THRESH_OTSU);
		//bitwise_not(binaryImg, binaryImg);
		findContours(binaryImg, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
		int size = 0 , j = 0;
		for (int i = 0; i < contours.size(); i++)
			if (contours[i].size() > size) size = contours[i].size(), j = i;
		drawContours(binaryImg, contours, j, Scalar(255, 255, 255), -1);
		Rect rect = boundingRect(contours[j]);
		binaryImg = binaryImg(rect);
		cutImg = img(rect);
		rows = cutImg.rows;
		cols = cutImg.cols;
		for (int i = 0 ; i < rows; i++)
			for (int j = 0; j < cols; j++)
				if (binaryImg.at<uchar>(i, j) == 0) cutImg.at<Vec4b>(i, j)[3] = 0;
	}
*/
	void cutOut(){
		int i, j;
		leftcorner.init(rows, cols);
		rightcorner.init(0,0);
		for (i = 0; i < rows; i++) {
			j = 0;
			while (img.at<Vec4b>(i, j)[0] > 240 && img.at<Vec4b>(i, j)[1] > 240 && img.at<Vec4b>(i, j)[2] > 240 && j < cols - 1) {
				img.at<Vec4b>(i, j)[3] = 0; 
				++j; 
			}
			if (j < cols - 1) {
				leftcorner.y = min(leftcorner.y, j);
				j = cols - 1;
				while (img.at<Vec4b>(i, j)[0] > 240 && img.at<Vec4b>(i, j)[1] > 240 && img.at<Vec4b>(i, j)[2] > 240 && j > 0) { 
					img.at<Vec4b>(i, j)[3] = 0; 
					--j; 
				}
				rightcorner.y = max(rightcorner.y, j);
				leftcorner.x = min(leftcorner.x, i);
				rightcorner.x = max(rightcorner.x, i);
			}
		}
		Rect rect(leftcorner.y, leftcorner.x, rightcorner.y - leftcorner.y + 1, rightcorner.x - leftcorner.x + 1);
		cutImg = img(rect);
		if (cutImg.rows / (double)cutImg.cols > rateLimit && rateLimit > 0) {
			cout << "2Long" << endl;
			exit(0);
		}
	}

	void transform() {
		double heightRate = cutImg.rows / (double)imgsize.height, widthRate = cutImg.cols / (double)imgsize.width;
		double rate = max(heightRate, widthRate);
		int height = cutImg.rows / rate, width = cutImg.cols / rate;
		Size dsize = Size(width,height);
		transformImg = Mat(dsize, CV_8UC4);
		resize(cutImg,transformImg,dsize);
	}

	myPoint getCenter() { return center;	}

	Mat getImg() { return transformImg;	}

	mySize getSize() { return imgsize; }

	//int getIndex() { return index; }

	void operate() {
		setCenter();
		cutOut();
		transform();
	}
	
	void enCode(const char* url, int y, int x, int width, int height) {
		setLeftUpSide(y, x);
		setSize(width, height);
		setCenter();

		QRcode *qrCode = QRcode_encodeString(url, 7, QR_ECLEVEL_L, QR_MODE_8, 1);
		int LEN = qrCode->width;
		cutImg.create(LEN, LEN, CV_8UC4);
		for (int i = 0; i < LEN; ++i)
			for (int j = 0; j < LEN; ++j) {
				if (qrCode->data[i*LEN + j] & 0x01)
					cutImg.at<Vec4b>(i, j)[0] = cutImg.at<Vec4b>(i, j)[1] = cutImg.at<Vec4b>(i, j)[2] = 0;
				else
					cutImg.at<Vec4b>(i, j)[0] = cutImg.at<Vec4b>(i, j)[1] = cutImg.at<Vec4b>(i, j)[2] = 255;
				cutImg.at<Vec4b>(i, j)[3] = 255;
			}
		imwrite("/Users/Ttdnts/Downloads/code.jpg", cutImg);
		transform();
	}
};

class MergeImg {
private:
	Mat img, itemimg;
	myPoint center, leftupside;
	int rows, cols;
public:
	void setImg(int cols_, int rows_) {
		img.create(Size(cols_, rows_), CV_8UC4);
		cols = cols_;
		rows = rows_;
		for( int i = 0; i < rows; i++)
			for (int j = 0; j < cols; j++) {
				img.at<Vec4b>(i, j)[0] = img.at<Vec4b>(i, j)[1] = img.at<Vec4b>(i, j)[2] = 255;
				img.at<Vec4b>(i, j)[3] = 255;
			}
	}
	/*
	void mergeByPoints(Item item) {

		int index = item.getIndex();
		mySize size = item.getSize();
		if (index == 1) leftupside.x = center.x + size.height / 2 - itemimg.rows;
		else leftupside.x = center.x - size.height / 2;
		leftupside.y = center.y - itemimg.cols / 2;
	}
	*/
	void mergeByCenter(Item item) {
		leftupside.x = center.x - itemimg.rows / 2;
		leftupside.y = center.y - itemimg.cols / 2;
	}

	void mergeItem(Item item) {
		itemimg = item.getImg();
		center = item.getCenter();
		//if (item.getIndex() > 0) mergeByPoints(item);
		//else 
		mergeByCenter(item);
		for (int i = 0; i < itemimg.rows; ++i)
			for (int j = 0; j < itemimg.cols; ++j)
				if (itemimg.data[i*itemimg.cols * 4 + j * 4 + 3] == 0) continue;
				else for (int k = 0; k < 4; k++)
					img.data[(i + leftupside.x)*cols * 4 + (j + leftupside.y) * 4 + k] = itemimg.data[i*itemimg.cols * 4 + j * 4 + k];
	}

	Mat getImg() {
		return img;
	}
};

int main(int argc, char* argv[]){
	
	MergeImg mergeImg;
	Item item;
	int T, index;
	string s, itemPath, finalPath, url;
	myPoint location;
	mySize size;
	Reader reader;
	Value root, output, products, qrcode;
	
	s = argv[argc - 1];
	//s = "{\"output\":[{\"width\":1000,\"height\":1600,\"file\":\"D:\\final.jpg\"}],\"products\":[{\"y\":25,\"x\":25,\"width\":524, \"height\":727, \"file\":\"D:\\suits.jpg\"},{\"y\":52,\"x\":750,\"width\":447, \"height\":785, \"file\":\"D:\\trousers.jpg\"}]}";
	if (reader.parse(s, root)) {
		output = root["output"];
		size.width = output["width"].asInt();
		size.height = output["height"].asInt();
		finalPath = output["file"].asString();
		mergeImg.setImg(size.width, size.height);

		products = root["products"];
		for (int i = 0; i < products.size(); ++i) {
			location.y = products[i]["y"].asInt();
			location.x = products[i]["x"].asInt();
			size.width = products[i]["width"].asInt();
			size.height = products[i]["height"].asInt();
			itemPath = products[i]["file"].asString();
			if (products[i]["rate"].isNull()) item.setRate(-1); else item.setRate(products[i]["rate"].asDouble());
			item.setImg(itemPath);
			item.setLeftUpSide(location.y, location.x);
			item.setSize(size.width, size.height);
			item.operate();
			mergeImg.mergeItem(item);

		}
		if (!output["qrcode"].isNull()) {
			qrcode = output["qrcode"];
			url = qrcode["url"].asString();
			location.x = qrcode["x"].asInt();
			location.y = qrcode["y"].asInt();
			size.width = qrcode["width"].asInt();
			size.height = qrcode["height"].asInt();
			item.enCode(url.c_str(), location.y, location.x, size.width, size.height);
			mergeImg.mergeItem(item);
		}
		imwrite(finalPath, mergeImg.getImg());
	}
	
	/*
	mySize size;
	MergeImg mergeImg;
	Item item;
	myPoint location;
	int T;
	string name;
	scanf("%d %d", &size.width, &size.height);
	mergeImg.setImg(size.width, size.height);

	scanf("%d", &T);
	getchar();
	for (int i = 0; i < T;++i) {
		getline(cin,name);
		item.setImg(name);
		scanf("%d %d", &location.y, &location.x);
		item.setLeftUpSide(location.y, location.x);
		scanf("%d %d", &size.width, &size.height);
		item.setSize(size.width,size.height);
		//scanf("%d", &index);
		//item.setIndex(index);
		item.operate();
		mergeImg.mergeItem(item);
		getchar();
	}
	imwrite("finalpic.png", mergeImg.getImg());
	*/
	return 0;
}