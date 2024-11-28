#include "reed_solomon.h"

#include <bitset>

ReedSolomon::ReedSolomon() : gf_exp_(512, 0), gf_log_(256, 0) { init_tables(); }

void ReedSolomon::init_tables() {
    int x = 1;
    for (int i = 0; i < 255; ++i) {
        gf_exp_[i] = x;
        gf_log_[x] = i;
        x = gf_mult_noLUT(x, 2);
    }

    for (int i = 255; i < 512; ++i) {
        gf_exp_[i] = gf_exp_[i - 255];
    }
}

int ReedSolomon::gf_mult_noLUT(int x, int y) {
    int r = 0;
    while (y > 0) {
        if (y & 1) r ^= x;
        y >>= 1;
        x <<= 1;
        if (x & 0x100) x ^= PRIMITIVE;
    }
    return r;
}

int ReedSolomon::gf_mul(int x, int y) {
    if (x == 0 || y == 0) return 0;
    return gf_exp_[(gf_log_[x] + gf_log_[y]) % 255];
}

int ReedSolomon::gf_div(int x, int y) {
    if (y == 0) throw std::runtime_error("Division by zero");
    if (x == 0) return 0;
    int result = gf_exp_[(gf_log_[x] + 255 - gf_log_[y]) % 255];
    return result;
}

int ReedSolomon::gf_pow(int x, int power) {
    if (power == 0) return 1;
    if (x == 0) return 0;
    int result = gf_exp_[(gf_log_[x] * power) % 255];
    return result;
}

int ReedSolomon::gf_inverse(int x) {
    if (x == 0) throw std::runtime_error("Cannot compute inverse of zero");
    return gf_exp_[255 - gf_log_[x]];
}

int ReedSolomon::gf_poly_eval(const std::vector<int>& poly, int x) {
    int y = poly[0];
    for (size_t i = 1; i < poly.size(); ++i) {
        y = gf_mul(y, x) ^ poly[i];
    }
    return y;
}

std::vector<int> ReedSolomon::gf_poly_add(const std::vector<int>& p,
                                          const std::vector<int>& q) {
    size_t max_len = std::max(p.size(), q.size());
    std::vector<int> r(max_len, 0);
    for (size_t i = 0; i < p.size(); ++i) {
        r[i + max_len - p.size()] = p[i];
    }
    for (size_t i = 0; i < q.size(); ++i) {
        r[i + max_len - q.size()] ^= q[i];
    }
    return r;
}

std::vector<int> ReedSolomon::gf_poly_scale(const std::vector<int>& p, int x) {
    std::vector<int> r(p.size());
    for (size_t i = 0; i < p.size(); ++i) {
        r[i] = gf_mul(p[i], x);
    }
    return r;
}

std::vector<int> ReedSolomon::gf_poly_mul(const std::vector<int>& p,
                                          const std::vector<int>& q) {
    std::vector<int> r(p.size() + q.size() - 1, 0);
    for (size_t j = 0; j < q.size(); ++j) {
        for (size_t i = 0; i < p.size(); ++i) {
            r[i + j] ^= gf_mul(p[i], q[j]);
        }
    }
    return r;
}

std::pair<std::vector<int>, std::vector<int>> ReedSolomon::gf_poly_div(
    const std::vector<int>& dividend, const std::vector<int>& divisor) {
    std::vector<int> msg_out = dividend;
    int len_diff = msg_out.size() - divisor.size();
    std::vector<int> coeffs(len_diff + 1, 0);
    for (int i = 0; i <= len_diff; ++i) {
        int coef = msg_out[i];
        if (coef != 0) {
            for (size_t j = 1; j < divisor.size(); ++j) {
                if (divisor[j] != 0) {
                    msg_out[i + j] ^= gf_mul(divisor[j], coef);
                }
            }
        }
        coeffs[i] = coef;
    }
    std::vector<int> remainder(msg_out.begin() + len_diff + 1, msg_out.end());
    return {coeffs, remainder};
}

ReedSolomon::Code ReedSolomon::encode(std::string message) {
    Code code = get_code(message);
    return encode_message(code);
}

ReedSolomon::Code ReedSolomon::get_code(std::string message) {
    std::vector<std::string> bin_code;
    for (char c : message) {
        std::bitset<8> b(c);
        bin_code.push_back(b.to_string());
    }
    std::string mode = "0100";
    std::bitset<8> data_len_bits(message.size());
    std::string data_len = data_len_bits.to_string();
    std::string terminator = "0000";
    std::string code = mode + data_len + "";
    for (auto& s : bin_code) {
        code += s;
    }
    code += terminator;
    while (code.size() < 152) {
        if (code.size() >= 16 &&
            code.substr(code.size() - 8, 8) == "00010001") {
            code += "11101100";
        } else {
            code += "00010001";
        }
    }
    std::vector<int> result;
    for (size_t i = 0; i < code.size(); i += 8) {
        std::bitset<8> byte(code.substr(i, 8));
        result.push_back((int)byte.to_ulong());
    }
    return result;
}

std::vector<int> ReedSolomon::encode_message(const std::vector<int>& msg_in) {
    std::vector<int> gen = generator_poly();
    std::vector<int> padded_msg = msg_in;
    padded_msg.resize(msg_in.size() + gen.size() - 1, 0);

    auto div_result = gf_poly_div(padded_msg, gen);
    std::vector<int> msg_out = msg_in;
    msg_out.insert(msg_out.end(), div_result.second.begin(),
                   div_result.second.end());
    return msg_out;
}

std::vector<int> ReedSolomon::generator_poly() {
    std::vector<int> g = {1};
    for (int i = 0; i < CORRECTION_SYMBOL_COUNT; ++i) {
        std::vector<int> term = {1, gf_exp_[i]};
        g = gf_poly_mul(g, term);
    }
    return g;
}

std::vector<int> ReedSolomon::calc_syndromes(const std::vector<int>& msg) {
    std::vector<int> synd(CORRECTION_SYMBOL_COUNT);
    for (int i = 0; i < CORRECTION_SYMBOL_COUNT; ++i) {
        synd[i] = gf_poly_eval(msg, gf_pow(2, i));
    }
    synd.insert(synd.begin(), 0);
    return synd;
}

std::vector<int> ReedSolomon::find_errors_locator(
    const std::vector<int>& e_pos) {
    std::vector<int> e_loc = {1};
    for (int i : e_pos) {
        std::vector<int> term = {gf_pow(2, i), 0};
        std::vector<int> sum = gf_poly_add({1}, term);
        e_loc = gf_poly_mul(e_loc, sum);
    }
    return e_loc;
}

std::vector<int> ReedSolomon::find_error_evaluator(
    const std::vector<int>& synd, const std::vector<int>& err_loc, int nsym) {
    auto product = gf_poly_mul(synd, err_loc);
    std::vector<int> divisor(nsym + 2, 0);
    divisor[0] = 1;
    auto div_result = gf_poly_div(product, divisor);
    return div_result.second;
}

std::vector<int> ReedSolomon::find_error_locator(
    const std::vector<int>& synd, int erase_count) {
    std::vector<int> err_loc = {1};
    std::vector<int> old_loc = {1};

    int synd_shift = synd.size() - CORRECTION_SYMBOL_COUNT;

    for (int i = 0; i < CORRECTION_SYMBOL_COUNT - erase_count; ++i) {
        int K = i + synd_shift;

        int delta = synd[K];
        for (size_t j = 1; j < err_loc.size(); ++j) {
            delta ^= gf_mul(err_loc[err_loc.size() - 1 - j], synd[K - j]);
        }

        old_loc.push_back(0);

        if (delta != 0) {
            if (old_loc.size() > err_loc.size()) {
                auto new_loc = gf_poly_scale(old_loc, delta);
                old_loc = gf_poly_scale(err_loc, gf_inverse(delta));
                err_loc = new_loc;
            }
            err_loc = gf_poly_add(err_loc, gf_poly_scale(old_loc, delta));
        }
    }

    while (!err_loc.empty() && err_loc[0] == 0) {
        err_loc.erase(err_loc.begin());
    }
    int errs = err_loc.size() - 1;
    if ((errs - erase_count) * 2 + erase_count > CORRECTION_SYMBOL_COUNT) {
        throw std::runtime_error("Too many errors to correct");
    }
    return err_loc;
}

std::vector<int> ReedSolomon::find_errors(const std::vector<int>& err_loc,
                                             int nmess) {
    int errs = err_loc.size() - 1;
    std::vector<int> err_pos;
    for (int i = 0; i < nmess; ++i) {
        if (gf_poly_eval(err_loc, gf_pow(2, i)) == 0) {
            err_pos.push_back(nmess - 1 - i);
        }
    }
    if (err_pos.size() != errs) {
        throw std::runtime_error("Could not find error locations");
    }
    return err_pos;
}

std::vector<int> ReedSolomon::forney_syndromes(const std::vector<int>& synd,
                                                  const std::vector<int>& pos,
                                                  int nmess) {
    std::vector<int> erase_pos_reversed(pos.size());
    for (size_t i = 0; i < pos.size(); ++i) {
        erase_pos_reversed[i] = nmess - 1 - pos[i];
    }
    std::vector<int> fsynd(synd.begin() + 1, synd.end());
    for (size_t i = 0; i < pos.size(); ++i) {
        int x = gf_pow(2, erase_pos_reversed[i]);
        for (size_t j = 0; j < fsynd.size(); ++j) {
            int next = (j + 1 < fsynd.size()) ? fsynd[j + 1] : 0;
            fsynd[j] = gf_mul(fsynd[j], x) ^ next;
        }
    }
    return fsynd;
}

std::vector<int> ReedSolomon::correct_errata(
    std::vector<int> msg_in, const std::vector<int>& synd,
    const std::vector<int>& err_pos) {
    std::vector<int> coef_pos(err_pos.size());
    int len_msg = msg_in.size();
    for (size_t i = 0; i < err_pos.size(); ++i) {
        coef_pos[i] = len_msg - 1 - err_pos[i];
    }

    std::vector<int> err_loc = find_errors_locator(coef_pos);
    std::vector<int> synd_rev(synd.rbegin(), synd.rend());
    std::vector<int> err_eval =
        find_error_evaluator(synd_rev, err_loc, err_loc.size() - 1);
    std::reverse(err_eval.begin(), err_eval.end());

    std::vector<int> X(coef_pos.size());
    for (size_t i = 0; i < coef_pos.size(); ++i) {
        int l = 255 - coef_pos[i];
        X[i] = gf_pow(2, -l);
    }

    std::vector<int> E(len_msg, 0);
    for (size_t i = 0; i < X.size(); ++i) {
        int Xi = X[i];
        int Xi_inv = gf_inverse(Xi);

        std::vector<int> err_loc_prime_tmp;
        for (size_t j = 0; j < X.size(); ++j) {
            if (j != i) {
                err_loc_prime_tmp.push_back(1 ^ gf_mul(Xi_inv, X[j]));
            }
        }
        int err_loc_prime = 1;
        for (int coef : err_loc_prime_tmp) {
            err_loc_prime = gf_mul(err_loc_prime, coef);
        }

        int y = gf_poly_eval(err_eval, Xi_inv);
        y = gf_mul(gf_pow(Xi, 1), y);

        if (err_loc_prime == 0) {
            throw std::runtime_error("Cannot find error magnitude");
        }

        int magnitude = gf_div(y, err_loc_prime);
        E[err_pos[i]] = magnitude;
    }

    msg_in = gf_poly_add(msg_in, E);
    return msg_in;
}

std::pair<std::vector<int>, std::vector<int>> ReedSolomon::decode_message(
    std::vector<int> msg_in, const std::vector<int>& erase_pos) {
    if (msg_in.size() > 255) {
        throw std::runtime_error("Message too long");
    }

    std::vector<int> msg_out = msg_in;
    if (!erase_pos.empty()) {
        for (int e_pos : erase_pos) {
            msg_out[e_pos] = 0;
        }
    }

    if (erase_pos.size() > static_cast<size_t>(CORRECTION_SYMBOL_COUNT)) {
        throw std::runtime_error("Too many erasures to correct");
    }

    std::vector<int> synd = calc_syndromes(msg_out);
    if (*std::max_element(synd.begin(), synd.end()) == 0) {
        std::vector<int> data(msg_out.begin(),
                              msg_out.end() - CORRECTION_SYMBOL_COUNT);
        std::vector<int> ecc(msg_out.end() - CORRECTION_SYMBOL_COUNT,
                             msg_out.end());
        return {data, ecc};
    }

    std::vector<int> fsynd =
        forney_syndromes(synd, erase_pos, msg_out.size());
    std::vector<int> err_loc = find_error_locator(fsynd, erase_pos.size());
    std::vector<int> err_loc_rev(err_loc.rbegin(), err_loc.rend());
    std::vector<int> err_pos = find_errors(err_loc_rev, msg_out.size());
    if (err_pos.empty()) {
        throw std::runtime_error("Could not locate errors");
    }

    msg_out = correct_errata(msg_out, synd, erase_pos);

    synd = calc_syndromes(msg_out);
    if (*std::max_element(synd.begin(), synd.end()) > 0) {
        throw std::runtime_error("Could not correct message");
    }

    std::vector<int> data(msg_out.begin(),
                          msg_out.end() - CORRECTION_SYMBOL_COUNT);
    std::vector<int> ecc(msg_out.end() - CORRECTION_SYMBOL_COUNT,
                         msg_out.end());
    return {data, ecc};
}

std::string ReedSolomon::decode(Code code, std::vector<int> erase_pos) {

};
