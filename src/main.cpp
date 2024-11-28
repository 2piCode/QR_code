#include <bitset>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "qr_code.h"
#include "reed_solomon.h"

int main() {
    std::string input_data = "spotify.com";

    ReedSolomon solomon;
    ReedSolomon::Code msg = solomon.encode(input_data);

    QRCode qr;
    std::vector<std::vector<int>> qr_code = qr.generate(msg);

    save_qr_to_ppm(qr_code, "qr_code.ppm");

    std::cout << "QR-код сохранён в файл qr_code.ppm" << std::endl;

    return 0;
}
