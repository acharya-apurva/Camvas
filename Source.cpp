#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>

using namespace cv;
using namespace std;

vector<Scalar> drawingColors = {
    Scalar(0,0,255),     // Red
    Scalar(0,255,0),     // Green
    Scalar(255,0,0),     // Blue
};

vector<Scalar> lowerBounds = { Scalar(0, 50, 50),Scalar(35, 50, 50), Scalar(100, 50, 50) }; //Upper bound for red green and blue
vector<Scalar> upperBounds = { Scalar(10, 255, 255),Scalar(85, 255, 255), Scalar(140, 255, 255) }; //Lower bound for red green and blue

int selectedColor = 0;  // Global variable used by trackbar callback
int redValue = 0;
int greenValue = 0;
int blueValue = 0;

class Pen {
private:
    Scalar currentColor;

    Point getContours(const Mat& image, const Mat& frame) {
        // Implementation of getContours function
        // ...
        vector<vector<Point>> contours;
        vector<Vec4i> hierarchy;

        findContours(image, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
        //drawcontours(img, contours, -1, Scalar(255, 0, 255), 2);
        vector<vector<Point>> conPoly(contours.size());
        vector<Rect> boundRect(contours.size());

        Point myPoint(0, 0);

        for (int i = 0; i < contours.size(); i++)
        {
            int area = contourArea(contours[i]);
            cout << area << endl;

            if (area > 1000)
            {
                float peri = arcLength(contours[i], true);
                approxPolyDP(contours[i], conPoly[i], 0.02 * peri, true);

                cout << conPoly[i].size() << endl;
                boundRect[i] = boundingRect(conPoly[i]);

                //getting the top centre of the object in the webcam
                myPoint.x = boundRect[i].x + boundRect[i].width / 2; //getting the centre x value of the top side on the bounding rect
                myPoint.y = boundRect[i].y; //getting the highest y

                drawContours(frame, conPoly, i, Scalar(255, 0, 255), 2);
                rectangle(frame, boundRect[i].tl(), boundRect[i].br(), Scalar(0, 255, 0), 5);
            }
        }
        return myPoint;
    }

public:
    Pen() : currentColor(drawingColors[0]) {}  // Default constructor uses the first color as default

    void setColor(const Scalar& color) {
        currentColor = color;
    }

    Scalar getColor() const {
        return currentColor;
    }

    Point detect(const Mat& mask, Mat& frame) {
        return getContours(mask, frame);
    }

    void draw(Mat& canvas, const Point& position) {
        if (position.x != 0 && position.y != 0) {
            circle(canvas, position, 10, currentColor, FILLED);
        }
    }

    void erase(Mat& canvas, const Point& position) {
        int eraserSize = 30;
        circle(canvas, position, eraserSize, Scalar(0, 0, 0), FILLED);
    }
};

class ColorPalette {
private:
    vector<Scalar> colors;
    Scalar eraserIconColor;
    bool eraserMode;
    int iconRadius = 20;
    int iconSpace = 10;

public:
    ColorPalette()
        : colors(drawingColors), eraserIconColor(Scalar(128, 128, 128)), eraserMode(false) {}

    bool isInEraserMode() const {
        return eraserMode;
    }

    void drawPalette(Mat& img) {
        // Logic from drawColorPalette()
        // ...
        int totalLength = (drawingColors.size() + 1) * iconRadius * 2 + (drawingColors.size()) * iconSpace;  // +1 for eraser
        Point startingPoint((img.cols - totalLength) / 2, img.rows - 2 * iconRadius - iconSpace);
        for (size_t i = 0; i < drawingColors.size(); ++i) {
            circle(img, Point(startingPoint.x + i * (2 * iconRadius + iconSpace), startingPoint.y), iconRadius, drawingColors[i], FILLED);
        }
        // Draw eraser icon
        Point eraserIconCenter = Point(startingPoint.x + drawingColors.size() * (2 * iconRadius + iconSpace), startingPoint.y);
        circle(img, eraserIconCenter, iconRadius, eraserIconColor, FILLED);
        if (isInEraserMode()) {
            // Make a red border to indicate eraser mode is active
            circle(img, eraserIconCenter, iconRadius, Scalar(0, 0, 255), 2);
        }
    }

    bool isInsideColorIcon(Point pt, Point iconCenter, int radius) {
        int dx = pt.x - iconCenter.x;
        int dy = pt.y - iconCenter.y;
        return (dx * dx + dy * dy <= radius * radius);
    }

    void checkColorChangeOrEraser(Pen& pen, const Point& point, const Mat& img) {

        int totalLength = (drawingColors.size() + 1) * iconRadius * 2 + (drawingColors.size()) * iconSpace; // +1 for eraser
        //Point startingPoint((img.cols - totalLength) / 2, img.rows - 2 * iconRadius - iconSpace);
        Point startingPoint((img.cols - totalLength) / 2, img.rows - 2 * iconRadius - iconSpace);
        for (size_t i = 0; i < drawingColors.size(); ++i) {
            Point iconCenter = Point(startingPoint.x + i * (2 * iconRadius + iconSpace), startingPoint.y);
            if (isInsideColorIcon(point, iconCenter, iconRadius)) {
                pen.setColor(drawingColors[i]);
                //currentDrawingColor = drawingColors[i];
                eraserMode = false;  // Deactivate eraser mode when color is picked
                break;
            }
        }
        Point eraserIconCenter = Point(startingPoint.x + drawingColors.size() * (2 * iconRadius + iconSpace), startingPoint.y);
        if (isInsideColorIcon(point, eraserIconCenter, iconRadius)) {
            eraserMode = true; // Activate eraser mode
        }
    }


};

class Webcam {
private:
    VideoCapture cap;

public:
    Webcam(int device = 0) : cap(device) {}

    Mat captureFrame() {
        Mat frame;
        cap.read(frame);
        return frame;
    }
};

class Canvas {
private:
    Mat content;

public:
    Canvas(const Size& size) : content(Mat::zeros(size, CV_8UC3)) {}

    void combineWith(const Mat& otherContent, Mat& destination) {
        addWeighted(otherContent, 1, content, 0.5, 0, destination);
    }

    Mat& getContent() {
        return content;
    }
};

void on_trackbar(int, void*) {
    int total = redValue + greenValue + blueValue;

    if (total > 1)  // If more than one trackbar is moved off 0, reset all to 0.
    {
        redValue = 0;
        greenValue = 0;
        blueValue = 0;

        setTrackbarPos("Red", "Color Selector", redValue);
        setTrackbarPos("Green", "Color Selector", greenValue);
        setTrackbarPos("Blue", "Color Selector", blueValue);
    }
}

Mat locatePen(const Mat& img) {
    Mat imgHSV;
    Mat mask;
    //HSV color space has different colors represented by their hue values which makes it easier to detect a range of shades for a 
    //specific color. That is why  we aqre converting the image.
    cvtColor(img, imgHSV, COLOR_BGR2HSV); //converting image from bgr to hsv

    if (redValue == 1) {
        inRange(imgHSV, lowerBounds[0], upperBounds[0], mask);
        Mat maskUpper;
        Scalar upperRedLowerBound(170, 50, 50);
        Scalar upperRedUpperBound(180, 255, 255);
        inRange(imgHSV, upperRedLowerBound, upperRedUpperBound, maskUpper);
        bitwise_or(mask, maskUpper, mask);
    }
    else if (greenValue == 1) {
        inRange(imgHSV, lowerBounds[1], upperBounds[1], mask);
    }
    else if (blueValue == 1) {
        inRange(imgHSV, lowerBounds[2], upperBounds[2], mask);
    }
    else {
        cerr << "No color selected." << endl;
        mask = Mat::zeros(imgHSV.size(), CV_8UC1);
    }

    imshow("Detected Color", mask);  // Changed the name of the window
    return mask;

    //Scalar lower(hmin, smin, vmin);
    //Scalar upper(hmax, smax, vmax);
}

int main() {
    Webcam webcam;
    Canvas canvas(webcam.captureFrame().size());
    Pen pen;
    ColorPalette palette;


    namedWindow("Color Selector", 1);
    //createTrackbar("Color", "Color Selector", &selectedColor, lowerBounds.size() - 1, on_trackbar);
    //namedWindow("Color Selector", 1);

// Creating trackbars for Red, Green, and Blue
createTrackbar("Red", "Color Selector", &redValue, 1, on_trackbar);
createTrackbar("Green", "Color Selector", &greenValue, 1, on_trackbar);
createTrackbar("Blue", "Color Selector", &blueValue, 1, on_trackbar);

    while (true) {
        Mat frame = webcam.captureFrame();
        Mat mask = locatePen(frame);
        Point point = pen.detect(mask,frame);
        palette.checkColorChangeOrEraser(pen, point, frame);
        palette.drawPalette(frame);

        if (palette.isInEraserMode()) {
            pen.erase(canvas.getContent(), point);
        }
        else {
            pen.draw(canvas.getContent(), point);
        }

        Mat combinedImg;
        canvas.combineWith(frame, combinedImg);

        imshow("Webcam", combinedImg);
        waitKey(1);
    }

    return 0;
}
