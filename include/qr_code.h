#pragma once

#include <fstream>
#include <vector>

inline void save_qr_to_ppm(const std::vector<std::vector<int>> &matrix,
                    const std::string &filename) {
    int size = matrix.size();
    int scale = 10;  // Масштабирование для лучшей видимости
    std::ofstream ofs(filename, std::ios::binary);
    ofs << "P6\n" << size * scale << " " << size * scale << "\n255\n";
    for (int i = 0; i < size; i++) {
        for (int si = 0; si < scale; si++) {
            for (int j = 0; j < size; j++) {
                for (int sj = 0; sj < scale; sj++) {
                    if (matrix[i][j] == 1) {
                        // Черный пиксель
                        unsigned char color[3] = {0, 0, 0};
                        ofs.write(reinterpret_cast<char *>(color), 3);
                    } else {
                        // Белый пиксель
                        unsigned char color[3] = {255, 255, 255};
                        ofs.write(reinterpret_cast<char *>(color), 3);
                    }
                }
            }
        }
    }
    ofs.close();
}

class QRCode {
   public:
    const static int SIZE = 21;
    const static int MASK_INDEX = 3;

    QRCode() = default;

    std::vector<std::vector<int>> generate(std::vector<int> message);

   private:
    std::vector<std::vector<int>> qr_code_ =
        std::vector<std::vector<int>>(SIZE, std::vector<int>(SIZE, 0));

    std::vector<std::vector<std::vector<int>>> mask_lines = {
        {{1, 1, 1, 0, 1, 1, 1, 1}, {0, 0, 1, 0, 0, 0, 1, 1}},
        {{1, 1, 1, 0, 0, 1, 0, 1}, {1, 1, 0, 0, 1, 1, 1, 1}},
        {{1, 1, 1, 1, 1, 0, 1, 1}, {0, 1, 0, 1, 0, 1, 0, 1}},
        {{1, 1, 1, 1, 0, 0, 0, 1}, {1, 0, 1, 1, 1, 0, 0, 1}},
        {{1, 1, 0, 0, 1, 1, 0, 0}, {1, 1, 1, 1, 0, 1, 0, 0}},
        {{1, 1, 0, 0, 0, 1, 1, 0}, {0, 0, 0, 1, 1, 0, 0, 0}},
        {{1, 1, 0, 1, 1, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 1, 0}},
        {{1, 1, 0, 1, 0, 0, 1, 0}, {0, 1, 1, 0, 1, 1, 1, 0}}};

    bool mask_fn(int row, int column);
    std::vector<std::pair<int, int>> generate_module_sequence();
    void fill_service_info(std::vector<std::vector<int>> &matrix);
    void fill_matrix_by_message(const std::vector<int> &message,
                                std::vector<std::pair<int, int>> &sequence);
    void apply_mask();
    void generate_spec_lines();
};
