#include "proclib.hpp"

/**
 * @brief Внутреннее (анонимное) пространство имен для вспомогательных функций.
 */
namespace
{
    /**
     * @brief Преобразует изображение в оттенки серого.
     * Если изображение уже в оттенках серого, возвращает его клон.
     */
    cv::Mat toGrayscale(const cv::Mat& src)
    {
        if (src.channels() == 1) {
            return src.clone();
        }
        cv::Mat gray;
        cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
        return gray;
    }

    /**
     * @brief Рассчитывает оптимальный порог по методу Оцу.
     */
    int calculateOtsuThresholdInternal(const cv::Mat& graySrc)
    {
        CV_Assert(graySrc.channels() == 1 && graySrc.type() == CV_8U);
        
        long totalPixels = graySrc.rows * graySrc.cols;
        if (totalPixels == 0) return -1;

        long histogram[256] = {0};
        for (int y = 0; y < graySrc.rows; y++) {
            const uchar* rowPtr = graySrc.ptr<uchar>(y);
            for (int x = 0; x < graySrc.cols; x++) {
                histogram[rowPtr[x]]++;
            }
        }

        long totalSum = 0;
        for (int i = 0; i < 256; i++) {
            totalSum += i * histogram[i];
        }

        long wB = 0;
        long sumB = 0;
        double maxVariance = 0;
        int bestThreshold = 0;

        for (int t = 0; t < 256; t++) {
            wB += histogram[t];
            if (wB == 0) continue;

            long wF = totalPixels - wB;
            if (wF == 0) break;

            sumB += (long)t * histogram[t];

            double muB = (double)sumB / wB;
            double muF = (double)(totalSum - sumB) / wF;

            double variance = (double)wB * (double)wF * (muB - muF) * (muB - muF);

            if (variance > maxVariance) {
                maxVariance = variance;
                bestThreshold = t;
            }
        }
        
        return bestThreshold;
    }
} // namespace


cv::Mat proc::sharpen(const cv::Mat& src)
{
    cv::Mat dest;
    
    cv::Mat kernel = (cv::Mat_<float>(3, 3) <<
         0, -1,  0,
        -1,  5, -1,
         0, -1,  0);

    cv::filter2D(src, dest, -1, kernel);
    
    return dest;
}

cv::Mat proc::manualThreshold(const cv::Mat& src, int threshold)
{
    cv::Mat gray = toGrayscale(src);
    cv::Mat dest;

    cv::threshold(gray, dest, threshold, 255, cv::THRESH_BINARY);
    
    return dest;
}

cv::Mat proc::otsuThreshold(const cv::Mat& src)
{
    cv::Mat gray = toGrayscale(src);
    cv::Mat dest;

    int otsuT = calculateOtsuThresholdInternal(gray);
    
    std::cout << "Otsu method calculated threshold: " << otsuT << std::endl;

    cv::threshold(gray, dest, otsuT, 255, cv::THRESH_BINARY);

    return dest;
}
