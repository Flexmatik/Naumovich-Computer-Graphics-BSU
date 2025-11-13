#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>
#include "proclib.hpp"

cv::Mat g_srcImage, g_destImage;
int g_manualThreshold = 128;
const int g_thresholdMax = 255;

const char* g_windowSrc = "Original";
const char* g_windowDest = "Result";

/**
 * @brief Пустой коллбэк для трекбара.
 * Мы используем трекбар только для хранения значения g_manualThreshold.
 * Само применение происходит по нажатию клавиши 't'.
 */
void onTrackbar(int, void*) {
}

/**
 * @brief Выводит инструкции в консоль.
 */
void printInstructions() {
    std::cout << "\n--- Управление ---" << std::endl;
    std::cout << "Нажмите клавишу в окне:" << std::endl;
    std::cout << "  's' - Увеличить резкость (Sharpen)" << std::endl;
    std::cout << "  'o' - Порог Оцу (Otsu)" << std::endl;
    std::cout << "  't' - Ручной порог (Threshold)" << std::endl;
    std::cout << "  'r' - Сброс (Reset)" << std::endl;
    std::cout << "  'q' - Выход (Quit)" << std::endl;
    std::cout << "------------------" << std::endl;
}

int main(int argc, char** argv) {
    
    std::string imagePath = "../test1.jpg"; // Изображение по умолчанию
    if (argc > 1) {
        imagePath = "../" + std::string(argv[1]);
    }

    // Загружаем исходное изображение
    g_srcImage = cv::imread(imagePath);
    if (g_srcImage.empty()) {
        std::cerr << "Ошибка: Не удалось загрузить изображение: " << imagePath << std::endl;
        std::cerr << "Убедитесь, что файл существует и находится в правильной директории." << std::endl;
        std::cerr << "Прочтите README.md для инструкций." << std::endl;
        return -1;
    }

    g_destImage = g_srcImage.clone();

    cv::namedWindow(g_windowSrc, cv::WINDOW_AUTOSIZE);
    cv::namedWindow(g_windowDest, cv::WINDOW_AUTOSIZE);

    cv::createTrackbar("Threshold", g_windowDest, &g_manualThreshold, g_thresholdMax, onTrackbar);

    cv::imshow(g_windowSrc, g_srcImage);
    cv::imshow(g_windowDest, g_destImage);

    printInstructions();

    while (true) {
        int key = cv::waitKey(0);

        switch (key) {
            case 'q':
            case 27:  // ESC
                return 0;

            case 'r':
                std::cout << "Applying: Reset" << std::endl;
                g_destImage = g_srcImage.clone();
                break;

            case 's':
                std::cout << "Applying: Sharpen" << std::endl;
                g_destImage = proc::sharpen(g_srcImage);
                break;

            case 'o':
                std::cout << "Applying: Otsu Threshold" << std::endl;
                g_destImage = proc::otsuThreshold(g_srcImage);
                break;

            case 't':
                std::cout << "Applying: Manual Threshold (Value: " << g_manualThreshold << ")" << std::endl;
                g_destImage = proc::manualThreshold(g_srcImage, g_manualThreshold);
                break;
        }
        cv::imshow(g_windowDest, g_destImage);
    }

    return 0;
}
