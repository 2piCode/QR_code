#include "qr_code.h"

std::vector<std::vector<int>> QRCode::generate(std::vector<int> message) {
    std::vector<std::pair<int, int>> sequence = generate_module_sequence();
    fill_matrix_by_message(message, sequence);
    apply_mask();
    generate_spec_lines();
    return qr_code_;
}

bool QRCode::mask_fn(int row, int column) {
    switch (MASK_INDEX) {
        case 0:
            return (row + column) % 2 == 0;
        case 1:
            return row % 2 == 0;
        case 2:
            return column % 3 == 0;
        case 3:
            return (row + column) % 3 == 0;
        case 4:
            return ((row / 2) + (column / 3)) % 2 == 0;
        case 5:
            return ((row * column) % 2 + (row * column) % 3) == 0;
        case 6:
            return (((row * column) % 2) + ((row * column) % 3)) % 2 == 0;
        case 7:
            return (((row + column) % 2) + ((row * column) % 3)) % 2 == 0;
        default:
            return false;
    }
}

std::vector<std::pair<int, int>> QRCode::generate_module_sequence() {
    std::vector<std::vector<int>> matrix(SIZE, std::vector<int>(SIZE, 0));
    fill_service_info(matrix);

    std::vector<std::pair<int, int>> sequence;
    int row_step = -1;
    int row = SIZE - 1;
    int column = SIZE - 1;
    int index = 0;
    while (column >= 0) {
        if (matrix[row][column] == 0) {
            sequence.push_back({row, column});
        }
        if (index % 2 == 1) {
            row += row_step;
            if (row == -1 || row == SIZE) {
                row_step = -row_step;
                row += row_step;
                column -= column == 7 ? 2 : 1;
            } else {
                column += 1;
            }
        } else {
            column -= 1;
        }
        index++;
    }
    return sequence;
}

void fill_area(std::vector<std::vector<int>>& matrix, int row, int column,
               int width, int height, int fill = 1) {
    std::vector<int> fill_row(width, fill);
    for (int i = row; i < row + height; ++i) {
        std::memcpy(&matrix[i][column], fill_row.data(), width * sizeof(int));
    }
}

void QRCode::fill_service_info(std::vector<std::vector<int>>& matrix) {
    fill_area(matrix, 0, 0, 9, 9);
    fill_area(matrix, 0, SIZE - 8, 8, 9);
    fill_area(matrix, SIZE - 8, 0, 9, 8);
    fill_area(matrix, 6, 9, 4, 1);
    fill_area(matrix, 9, 6, 1, 4);
    matrix[SIZE - 8][8] = 1;
}

void QRCode::fill_matrix_by_message(
    const std::vector<int>& message,
    std::vector<std::pair<int, int>>& sequence) {
    for (size_t index = 0; index < sequence.size(); ++index) {
        int row = sequence[index].first;
        int col = sequence[index].second;

        int codeword = message[index / 8];
        int bit_index = index % 8;
        std::bitset<8> bit(codeword);
        int bit_int = bit[7 - bit_index];
        qr_code_[row][col] = bit[7 - bit_index] ^ mask_fn(row, col);
    }
}

// Маскирование
void QRCode::apply_mask() {
    for (size_t i = 0; i < 8; i++) {
        if (i >= 6) {
            qr_code_[8][i + 1] = mask_lines[MASK_INDEX][0][i];
        } else {
            qr_code_[8][i] = mask_lines[MASK_INDEX][0][i];
        }

        qr_code_[20 - i][8] = mask_lines[MASK_INDEX][0][i];
        if (i >= 6) {
            qr_code_[i + 1][8] = mask_lines[MASK_INDEX][1][i];
        } else {
            qr_code_[i][8] = mask_lines[MASK_INDEX][1][i];
        }
        qr_code_[8][20 - i] = mask_lines[MASK_INDEX][1][i];
    }
}

void QRCode::generate_spec_lines() {
    int l = qr_code_.size();

    // Добавление линий синхронизации
    for (int i = 8; i <= 12; i += 2) {
        qr_code_[6][i] = 1;
        qr_code_[i][6] = 1;
    }

    // Создание маркеров позиционирования в углах
    for (int i = 0; i < 7; i++) {
        qr_code_[0][i] = 1;
        qr_code_[6][i] = 1;
        qr_code_[i][0] = 1;
        qr_code_[i][6] = 1;
        qr_code_[l - 1][i] = 1;
        qr_code_[l - 7][i] = 1;
        qr_code_[i][l - 1] = 1;
        qr_code_[i][l - 7] = 1;
        qr_code_[l - i - 1][0] = 1;
        qr_code_[l - i - 1][6] = 1;
        qr_code_[0][l - i - 1] = 1;
        qr_code_[6][l - i - 1] = 1;
    }

    // Заполнение внутренних квадратов маркеров
    for (int i = 2; i < 5; i++) {
        for (int j = 2; j < 5; j++) {
            qr_code_[i][j] = 1;
            qr_code_[l - i - 1][j] = 1;
            qr_code_[i][l - j - 1] = 1;
        }
    }

    // Дополнительный черный модуль
    qr_code_[13][8] = 1;
}
