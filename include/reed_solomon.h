#pragma once

#include <vector>

class ReedSolomon {
   public:
    using Code = std::vector<int>;
    static const int CORRECTION_SYMBOL_COUNT = 7;
    static const int PRIMITIVE = 0x11d;

    ReedSolomon();

    Code encode(std::string message);
    std::string decode(Code code, std::vector<int> erase_pos = {});

   private:
    std::vector<int> gf_log_;  // Логарифмическая таблица
    std::vector<int> gf_exp_;  // Экспоненциальная таблица

    void init_tables();
    int gf_mult_noLUT(int x, int y);

    std::vector<int> encode_message(const std::vector<int>& msg_in);
    std::pair<std::vector<int>, std::vector<int>> decode_message(
        std::vector<int> msg_in, const std::vector<int>& erase_pos);
    Code get_code(std::string message);
    std::vector<int> generator_poly();

    int gf_mul(int x, int y);
    int gf_div(int x, int y);
    int gf_pow(int x, int power);
    int gf_inverse(int x);
    int gf_poly_eval(const std::vector<int>& poly, int x);
    std::vector<int> gf_poly_add(const std::vector<int>& p,
                                 const std::vector<int>& q);
    std::vector<int> gf_poly_scale(const std::vector<int>& p, int x);
    std::vector<int> gf_poly_mul(const std::vector<int>& p,
                                 const std::vector<int>& q);
    std::pair<std::vector<int>, std::vector<int>> gf_poly_div(
        const std::vector<int>& dividend, const std::vector<int>& divisor);

    std::vector<int> calc_syndromes(const std::vector<int>& msg);
    std::vector<int> find_errors_locator(const std::vector<int>& e_pos);
    std::vector<int> find_error_evaluator(const std::vector<int>& synd,
                                             const std::vector<int>& err_loc,
                                             int nsym);
    std::vector<int> find_error_locator(const std::vector<int>& synd,
                                           int erase_count = 0);
    std::vector<int> find_errors(const std::vector<int>& err_loc, int nmess);
    std::vector<int> forney_syndromes(const std::vector<int>& synd,
                                         const std::vector<int>& pos,
                                         int nmess);
    std::vector<int> correct_errata(std::vector<int> msg_in,
                                       const std::vector<int>& synd,
                                       const std::vector<int>& err_pos);
};
