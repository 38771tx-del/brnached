#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>

class IconDetector {
private:
    int minArea = 100;
    int maxArea = 5000;
    double minAspectRatio = 0.7;
    double maxAspectRatio = 1.3;
    int cannyLow = 80;
    int cannyHigh = 150;
    int blurSize = 3;

public:
    IconDetector() = default;
    
    void setThresholds(int minArea, int maxArea, double minAspect, double maxAspect) {
        this->minArea = minArea;
        this->maxArea = maxArea;
        this->minAspectRatio = minAspect;
        this->maxAspectRatio = maxAspect;
    }
    
    void setCannyThresholds(int low, int high) {
        this->cannyLow = low;
        this->cannyHigh = high;
    }
    
    void setBlurSize(int size) {
        this->blurSize = size;
    }

    int detectIconCount(const cv::Mat& image, bool showDebug = false) {
        if (image.empty()) {
            std::cout << "Failed to load image!\n";
            return -1;
        }

        cv::Mat gray;
        if (image.channels() == 3) {
            cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
        } else {
            gray = image.clone();
        }

        cv::Mat blurred;
        cv::GaussianBlur(gray, blurred, cv::Size(blurSize, blurSize), 0);

        cv::Mat edges;
        cv::Canny(blurred, edges, cannyLow, cannyHigh);

        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(edges, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        int iconCount = 0;
        cv::Mat debugImg = image.clone();

        for (const auto& contour : contours) {
            cv::Rect box = cv::boundingRect(contour);
            double aspectRatio = (double)box.width / (double)box.height;
            double area = cv::contourArea(contour);

            if (area > minArea && area < maxArea && 
                aspectRatio > minAspectRatio && aspectRatio < maxAspectRatio) {
                iconCount++;
                
                if (showDebug) {
                    cv::rectangle(debugImg, box, cv::Scalar(0, 255, 0), 2);
                    cv::putText(debugImg, std::to_string(iconCount), 
                               cv::Point(box.x, box.y - 5), 
                               cv::FONT_HERSHEY_SIMPLEX, 0.6, 
                               cv::Scalar(0, 255, 0), 2);
                }
            }
        }

        if (showDebug) {
            cv::imshow("Original Image", image);
            cv::imshow("Edges", edges);
            cv::imshow("Detected Icons", debugImg);
            cv::waitKey(0);
        }

        return iconCount;
    }

    int detectIconCountInROI(const cv::Mat& image, cv::Rect roi, bool showDebug = false) {
        if (roi.x < 0 || roi.y < 0 || 
            roi.x + roi.width > image.cols || 
            roi.y + roi.height > image.rows) {
            std::cout << "Invalid ROI bounds!\n";
            return -1;
        }

        cv::Mat croppedImage = image(roi);
        return detectIconCount(croppedImage, showDebug);
    }

    void autoTuneThresholds(const cv::Mat& image, int expectedCount) {
        std::cout << "Auto-tuning thresholds for " << expectedCount << " expected icons...\n";
        
        int bestCount = 0;
        int bestMinArea = minArea;
        int bestCannyLow = cannyLow;
        int bestCannyHigh = cannyHigh;

        for (int area = 50; area <= 200; area += 25) {
            for (int low = 50; low <= 120; low += 20) {
                for (int high = 120; high <= 200; high += 20) {
                    int oldMinArea = this->minArea;
                    int oldLow = this->cannyLow;
                    int oldHigh = this->cannyHigh;
                    
                    this->minArea = area;
                    this->cannyLow = low;
                    this->cannyHigh = high;
                    
                    int count = detectIconCount(image, false);
                    
                    if (abs(count - expectedCount) < abs(bestCount - expectedCount)) {
                        bestCount = count;
                        bestMinArea = area;
                        bestCannyLow = low;
                        bestCannyHigh = high;
                    }
                    
                    this->minArea = oldMinArea;
                    this->cannyLow = oldLow;
                    this->cannyHigh = oldHigh;
                }
            }
        }

        this->minArea = bestMinArea;
        this->cannyLow = bestCannyLow;
        this->cannyHigh = bestCannyHigh;
        
        std::cout << "Best parameters found: minArea=" << bestMinArea 
                  << ", cannyLow=" << bestCannyLow 
                  << ", cannyHigh=" << bestCannyHigh 
                  << ", detected=" << bestCount << " icons\n";
    }
};

int main() {
    IconDetector detector;

    cv::Mat image = cv::imread("screenshot.png");
    if (image.empty()) {
        std::cout << "Please place your screenshot as 'screenshot.png' in the same directory.\n";
        std::cout << "Press Enter to exit...";
        std::cin.get();
        return -1;
    }

    std::cout << "=== Icon Detection Test ===\n";
    std::cout << "Image loaded: " << image.cols << "x" << image.rows << " pixels\n\n";

    std::cout << "Testing with default parameters...\n";
    int count = detector.detectIconCount(image, true);
    std::cout << "Detected icons: " << count << "\n\n";

    std::cout << "Would you like to auto-tune parameters? (y/n): ";
    char choice;
    std::cin >> choice;
    
    if (choice == 'y' || choice == 'Y') {
        std::cout << "How many icons do you expect to see? ";
        int expected;
        std::cin >> expected;
        
        detector.autoTuneThresholds(image, expected);
        
        std::cout << "\nTesting with tuned parameters...\n";
        count = detector.detectIconCount(image, true);
        std::cout << "Final detected icons: " << count << "\n";
    }

    std::cout << "\n=== ROI Detection Example ===\n";
    std::cout << "You can also detect icons in a specific region:\n";
    std::cout << "detector.detectIconCountInROI(image, cv::Rect(x, y, width, height), true);\n";

    return 0;
}
