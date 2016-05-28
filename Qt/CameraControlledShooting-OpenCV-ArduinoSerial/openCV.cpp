#include <opencv2/opencv.hpp>
#include "servoControl.cpp"

__attribute__((always_inline))
static inline int getByte(cv::Mat frame, int x, int y, int byte) {
    return *(frame.data + frame.step[0] * y + frame.step[1] * x + byte);
}

__attribute__((always_inline))
static inline void writeByte(cv::Mat frame, int x, int y, int byte, int value) {
    *(frame.data + frame.step[0] * y + frame.step[1] * x + byte) = value;
}

__attribute__((always_inline))
static inline float getRelation(cv::Mat frame, int x, int y, int byte) {
    float sum = (getByte(frame, x, y, 0) + getByte(frame, x, y, 1) + getByte(frame, x, y, 2));
    float single = getByte(frame, x, y, byte);
    if (sum == 0) {
        sum = 1;
    }
    return single/sum;
}

void markPosition(cv::Mat &frame, int posx, int posy) {
    int size = 5;
    int markColor[3] = {255, 0, 0};
    for (int y = posy - size; y < posy + size; y++) {
        for (int x = posx - size; x < posx + size; x++){
            for (int i = 0; i < 3; i++) {
                if (x > 0 && y > 0){
                    writeByte(frame, x, y, i, markColor[i]);
                }
            }
        }
    }
}

void detectBallByAverage(cv::Mat &frame) {
    int ctr = 0, ypos = 0, xpos = 0;
    for (int y = 0; y < frame.rows; y++) {
        for (int x = 0; x < frame.cols; x++) {
            if (getRelation(frame, x, y, 2) > 0.7) {
                ctr++;
                ypos += y;
                xpos += x;
            }
        }
    }
    if (ctr == 0) {
        ctr = 1;
    }
    ypos /= ctr;
    xpos /= ctr;
    if (xpos > 0 && ypos > 0) {
        std::cout << "Position x: " << xpos << " y: " << ypos << " ctr: " << ctr << std::endl;
        markPosition(frame, xpos, ypos);
        if (ctr > 100) {
            updateServosAccordingToCam(xpos, ypos);
        }
    }
}

int moveWhileSameColor(cv::Mat &frame, int starty, int startx, int directiony, int directionx) {
    int length = 0;
    int posy = starty, posx = startx;
    bool colorOK = true;
    while (colorOK) {
        posy += directiony;
        posx += directionx;
        length++; //will not work in 45° angles
        for (int i = 0; i < 3; i++) {
            if (getRelation(frame, posx, posy, 2) < 0.7) {
                colorOK = false;
            }
        }
    }
    return length; //minimum return value is 1
}

void detectBallWithLines(cv::Mat frame) {
    for (int y = 0; y < frame.rows; y++) {
        for (int x = 0; x < frame.cols; x++) {
            if (getRelation(frame, x, y, 2) < 0.7) {
                int initHeight = moveWhileSameColor(frame, y, x, 1, 0); //go from first point down
                int iniWidthRight = moveWhileSameColor(frame, y + initHeight * 0.5, x, 0, 1); //go at half right
                int iniWidthLeft = moveWhileSameColor(frame, y + initHeight * 0.5, x, 0, -1); //go at half left
                int iniWidth = iniWidthLeft + iniWidthRight;
                if (initHeight > 50 && iniWidth > 50) {
                    std::cout << "pos x: " << x << " y: " << y + initHeight * 0.5 << std::endl;
                    markPosition(frame, x, y + initHeight * 0.5);
                    return;
                }

            }
        }
    }
}

void showAvgBGR(cv::Mat frame) {
    long int avgRGB[3] = {0, 0, 0}; //2^32 is far greater than e.g. 640 * 480 * 255
    for (int y = 0; y < frame.rows; y += 1) {
        for (int x = 0; x < frame.cols; x += 1) {
            for (int color = 0; color < 3; color++){
                avgRGB[color] += *(frame.data + frame.step[0] * y + frame.step[1] * x + color); //http://docs.opencv.org/2.4/modules/core/doc/basic_structures.html#mat; the 4th byte ist the first byte of the next image
            }
        }
    }
    float divisor = frame.rows * frame.cols; //640 * 480;
    std::cout << "Average BGR-values: ";
    for (int ctr = 0; ctr < 3; ctr++) {
        avgRGB[ctr] /= divisor;
        std::cout << avgRGB[ctr] << "|";
    }
    std::cout << std::endl;
}

void printApropriateSign(int value = 0) {
    char sign = 'Y';
    if (value > 255) {
        sign = '#';
    } else if (value > 200) {
        sign = 'X';
    } else if (value > 150) {
        sign = 'x';
    } else if (value > 100) {
        sign = 'O';
    } else if (value > 50) {
        sign = 'o';
    } else if (value > 5) {
        sign = '-';
    } else {
        sign = '.';
    }
    std::cout << sign << "|";
}

void showOnCMD(cv::Mat frame, int cols = 13, int rows = 30) {
    int ysteps = frame.rows/rows;
    int xsteps = frame.cols/cols;
    std::cout << "Frame: " << cols << " x " << rows << " pixel" << std::endl;
    for (int y = 0; y < frame.rows; y += ysteps) {
        for (int x = 0; x < frame.cols; x += xsteps) {
            std::cout << "|";
            for (int color = 0; color < 3; color++){
                printApropriateSign(*(frame.data + frame.step[0] * y + frame.step[1] * x + color)); //http://docs.opencv.org/2.4/modules/core/doc/basic_structures.html#mat; the 4th byte ist the first byte of the next image
            }
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

void printParams(cv::VideoCapture cap) {
    std::vector<double> propId =
    {
        CV_CAP_PROP_POS_MSEC,
        CV_CAP_PROP_POS_FRAMES,
        CV_CAP_PROP_POS_AVI_RATIO,
        CV_CAP_PROP_FRAME_WIDTH,
        CV_CAP_PROP_FRAME_HEIGHT,
        CV_CAP_PROP_FPS,
        CV_CAP_PROP_FOURCC,
        CV_CAP_PROP_FRAME_COUNT,
        CV_CAP_PROP_FORMAT,
        CV_CAP_PROP_MODE,
        CV_CAP_PROP_BRIGHTNESS,
        CV_CAP_PROP_CONTRAST,
        CV_CAP_PROP_SATURATION,
        CV_CAP_PROP_HUE,
        CV_CAP_PROP_GAIN,
        CV_CAP_PROP_EXPOSURE,
        CV_CAP_PROP_CONVERT_RGB,
        CV_CAP_PROP_WHITE_BALANCE_BLUE_U,
        CV_CAP_PROP_WHITE_BALANCE_RED_V,
        CV_CAP_PROP_RECTIFICATION,
        CV_CAP_PROP_ISO_SPEED,
        CV_CAP_PROP_BUFFERSIZE
    };
    for (unsigned int i = 0; i < propId.size(); i++) {
        std::cout << i << "(" << propId.at(i) << "): " << std::to_string(cap.get(propId.at(i))) << std::endl;
    }
}
/*
    CV_CAP_PROP_POS_MSEC Current position of the video file in milliseconds or video capture timestamp.
    CV_CAP_PROP_POS_FRAMES 0-based index of the frame to be decoded/captured next.
    CV_CAP_PROP_POS_AVI_RATIO Relative position of the video file: 0 - start of the film, 1 - end of the film.
    CV_CAP_PROP_FRAME_WIDTH Width of the frames in the video stream.
    CV_CAP_PROP_FRAME_HEIGHT Height of the frames in the video stream.
    CV_CAP_PROP_FPS Frame rate.
    CV_CAP_PROP_FOURCC 4-character code of codec.
    CV_CAP_PROP_FRAME_COUNT Number of frames in the video file.
    CV_CAP_PROP_FORMAT Format of the Mat objects returned by retrieve() .
    CV_CAP_PROP_MODE Backend-specific value indicating the current capture mode.
    CV_CAP_PROP_BRIGHTNESS Brightness of the image (only for cameras).
    CV_CAP_PROP_CONTRAST Contrast of the image (only for cameras).
    CV_CAP_PROP_SATURATION Saturation of the image (only for cameras).
    CV_CAP_PROP_HUE Hue of the image (only for cameras).
    CV_CAP_PROP_GAIN Gain of the image (only for cameras).
    CV_CAP_PROP_EXPOSURE Exposure (only for cameras).
    CV_CAP_PROP_CONVERT_RGB Boolean flags indicating whether images should be converted to RGB.
    CV_CAP_PROP_WHITE_BALANCE_U The U value of the whitebalance setting (note: only supported by DC1394 v 2.x backend currently)
    CV_CAP_PROP_WHITE_BALANCE_V The V value of the whitebalance setting (note: only supported by DC1394 v 2.x backend currently)
    CV_CAP_PROP_RECTIFICATION Rectification flag for stereo cameras (note: only supported by DC1394 v 2.x backend currently)
    CV_CAP_PROP_ISO_SPEED The ISO speed of the camera (note: only supported by DC1394 v 2.x backend currently)
    CV_CAP_PROP_BUFFERSIZE Amount of frames stored in internal buffer memory (note: only supported by DC1394 v 2.x backend currently)
*/