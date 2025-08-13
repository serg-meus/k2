#include <string>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <cctype>
#include <cmath>


typedef uint8_t u8;
typedef int8_t i8;
typedef uint16_t u16;
typedef int16_t i16;
typedef uint32_t u32;
typedef int32_t i32;
typedef unsigned long long u64;

std::vector<std::string> split(const std::string &str, const char sep,
                                      const size_t max_split);

std::string coord_to_str(const u8 coord);


inline std::string to_lower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(),
                   [](unsigned char c){return std::tolower(c);});
    return str;
}


inline std::vector<std::string> split(const std::string &str,
                               const char sep) {
    return split(str, sep, 0);
}


inline std::vector<std::string> split(const std::string &str) {
    return split(str, ' ', 0);
}


inline bool startswith(const std::string &str,
                       const std::string &substring) {
    return std::equal(substring.begin(), substring.end(), str.begin());
}


inline constexpr u8 get_col(const u8 coord) {
    return coord & 7;
}


inline constexpr u8 get_row(const u8 coord) {
    return u8(coord >> 3);
}


inline constexpr u8 get_coord(const u8 col, const u8 row) {
    return u8(8*row + col);
}


inline constexpr u64 file_mask(const u8 coord) {
    return u64(0x0101010101010101) << get_col(coord);
}


inline constexpr u64 file_mask(const char file) {
    return u64(0x0101010101010101) << u64(file - 'a');
}


inline constexpr u64 rank_mask(const u8 coord) {
    return u64(0xff) << u64(8*get_row(coord));
}


inline constexpr u64 rank_mask(const char rank) {
    return u64(0xff) << u64(8*(rank - '1'));
}


inline constexpr u8 str_to_coord(const char *str_c) {
    return get_coord(u8(str_c[0] - 'a'), u8(str_c[1] - '1'));
}


inline constexpr u64 one_nth_bit(const u8 bit_num) {
    return u64(1) << bit_num;
}


inline constexpr u64 lower_bit(const u64 x) {
    return x & -x;
}


inline constexpr u8 trail_zeros(const u64 mask) {
    return mask == 0 ? u8(-1) : u8(__builtin_ctzll(mask));
}


inline constexpr u64 signed_shift(const u64 x, const int shift) {
    return shift > 0 ? x >> u64(shift) : x << u64(-shift);
}


inline constexpr u64 get_nth_bit(const u64 inp, const u8 bit_num) {
    return inp >> u64(bit_num) & u64(1);
}


inline constexpr bool is_close(const double x, const double y) {
    return std::abs(x - y) <= 1e-8;
}

inline constexpr u64 roll_left(u64 bitboard) {
    return (bitboard & ~file_mask('a')) >> 1;
}

inline constexpr u64 roll_right(u64 bitboard) {
    return (bitboard & ~file_mask('h')) << 1;
}

inline constexpr u64 roll_up(u64 bitboard, bool color) {
    return signed_shift(bitboard, color ? -8 : 8);
}

inline constexpr u64 roll_down(u64 bitboard, bool color) {
    return signed_shift(bitboard, color ? 8 : -8);
}

inline constexpr int popcount(u64 x) {
    return __builtin_popcountll(x);
}
