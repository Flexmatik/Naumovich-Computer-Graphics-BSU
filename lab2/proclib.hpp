#ifndef PROCESSING_HPP
#define PROCESSING_HPP

#include <opencv2/opencv.hpp>

/**
 * @brief Пространство имен для функций обработки изображений.
 */
namespace proc
{
    /**
     * @brief Увеличивает резкость изображения с помощью ядра Лапласа.
     * @param src Исходное изображение.
     * @return Обработанное изображение с повышенной резкостью.
     */
    cv::Mat sharpen(const cv::Mat& src);

    /**
     * @brief Применяет ручную глобальную пороговую обработку.
     * @param src Исходное изображение (будет преобразовано в оттенки серого).
     * @param threshold Значение порога (0-255).
     * @return Черно-белое бинаризованное изображение.
     */
    cv::Mat manualThreshold(const cv::Mat& src, int threshold);

    /**
     * @brief Применяет глобальную пороговую обработку методом Оцу.
     * @param src Исходное изображение (будет преобразовано в оттенки серого).
     * @return Черно-белое бинаризованное изображение.
     */
    cv::Mat otsuThreshold(const cv::Mat& src);

} // namespace proc

#endif // PROCESSING_HPP
