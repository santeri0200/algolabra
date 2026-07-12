#include <cstdint>
#include <vector>
#include <array>
#include <algorithm>

struct BitWriter {
    std::vector<uint8_t>& out;

    uint32_t buffer = 0;
    int32_t bits = 0;

    BitWriter(std::vector<uint8_t>& o) : out(o) {}

    void Write(uint32_t v, int32_t n) {
        buffer |= (v & ((1u << n) - 1)) << bits;
        bits += n;

        while (8 <= bits) {
            out.push_back(buffer & 0xff);
            buffer >>= 8;
            bits -= 8;
        }
    }

    void Flush() {
        if (bits) {
            out.push_back(buffer & 0xff);
            buffer = 0;
            bits = 0;
        }
    }
};

struct HuffCode {
    uint16_t code;
    uint8_t bits;
};


static uint16_t ReverseBits(uint16_t x, int32_t n) {
    uint16_t r = 0;
    while (n--) {
        r = (r << 1) | (x & 1);
        x >>= 1;
    }

    return r;
}

static void BuildHuffman(const std::vector<uint8_t>& lengths, std::vector<HuffCode>& table) {
    int32_t code = 0;
    int32_t next[16] = {};
    int32_t count[16] = {};
    for(uint8_t l: lengths) if (l) count[l]++;

    table.resize(lengths.size());

    for(int32_t bits = 1; bits <= 15; bits++) {
        code = (code + count[bits - 1]) << 1;
        next[bits] = code;
    }

    for (size_t i = 0; i < lengths.size(); i++) {
        uint8_t len=lengths[i];

        if (len) {
            table[i].bits = len;
            table[i].code = ReverseBits(next[len]++, len);
        } else {
            table[i] = {0, 0};
        }
    }
}

struct FixedTables {
    std::vector<HuffCode> lit;
    std::vector<HuffCode> dist;
};

static FixedTables MakeFixedTables() {
    FixedTables t;
    std::vector<uint8_t> ll(288);

    for(int32_t i = 0; i <= 143; i++) ll[i] = 8;
    for(int32_t i = 144; i <= 255; i++) ll[i] = 9;
    for(int32_t i = 256; i <= 279; i++) ll[i] = 7;
    for(int32_t i = 280; i <= 287; i++) ll[i] = 8;

    std::vector<uint8_t> dd(32,5);

    BuildHuffman(ll, t.lit);
    BuildHuffman(dd, t.dist);

    return t;
}

static void Emit(BitWriter& bw, const HuffCode& c) {
    bw.Write(c.code,c.bits);
}

struct Match {
    int32_t length;
    int32_t distance;
};

struct LZ77 {
    static constexpr int32_t WINDOW = 32768;
    static constexpr int32_t HASH = 1 << 15;

    std::vector<int> head;
    std::vector<int> prev;

    LZ77(size_t size) : head(HASH, -1), prev(size, -1) {}

    uint32_t Hash(const uint8_t* p) {
        return ((p[0] * 251u) ^ (p[1] * 911u) ^ (p[2] * 3571u)) & (HASH - 1);
    }

    void Insert(const uint8_t* data, size_t pos, size_t size) {
        if (size <= pos + 2) {
            return;
        }

        uint32_t h = Hash(data + pos);

        prev[pos] = head[h];
        head[h] = (int)pos;
    }

    Match Find(const uint8_t* data, size_t size, size_t pos) {
        Match best {0, 0};

        if (size <= pos + 2) {
            return best;
        }

        uint32_t h = Hash(data + pos);

        int32_t candidate = head[h];
        int32_t depth = 0;

        while (0 <= candidate && depth++ < 128) {
            size_t distance = pos - candidate;

            if (distance > WINDOW) {
                break;
            }

            int32_t len = 0;
            while (len < 258 && pos + len < size && data[candidate+len] == data[pos+len]) {
                len++;
            }

            if (best.length < len && 3 <= len) {
                best.length = len;
                best.distance = distance;

                if (len == 258) {
                    break;
                }
            }

            candidate = prev[candidate];
        }

        return best;
    }
};

struct LengthCode {
    int32_t symbol;
    int32_t base;
    int32_t extraBits;
};

static const std::vector<LengthCode> LengthTable = {
    {257, 3,   0},
    {258, 4,   0},
    {259, 5,   0},
    {260, 6,   0},
    {261, 7,   0},
    {262, 8,   0},
    {263, 9,   0},
    {264, 10,  0},
    {265, 11,  1},
    {266, 13,  1},
    {267, 15,  1},
    {268, 17,  1},
    {269, 19,  2},
    {270, 23,  2},
    {271, 27,  2},
    {272, 31,  2},
    {273, 35,  3},
    {274, 43,  3},
    {275, 51,  3},
    {276, 59,  3},
    {277, 67,  4},
    {278, 83,  4},
    {279, 99,  4},
    {280,115,  4},
    {281,131,  5},
    {282,163,  5},
    {283,195,  5},
    {284,227,  5},
    {285,258,  0}
};

void WriteLength(BitWriter& bw, const FixedTables& tables, int32_t length) {
    for(const auto& e : LengthTable) {
        int32_t max = e.base + ((1 << e.extraBits) - 1);
        if (length >= e.base && length <= max) {
            // Huffman symbol
            Emit(bw, tables.lit[e.symbol]);

            // extra length bits
            if (e.extraBits) {
                bw.Write(length - e.base, e.extraBits);
            }

            return;
        }
    }
}

struct DistanceCode {
    int32_t symbol;
    int32_t base;
    int32_t extraBits;
};


static const std::vector<DistanceCode> DistanceTable = {
    {0,     1,     0},
    {1,     2,     0},
    {2,     3,     0},
    {3,     4,     0},

    {4,     5,     1},
    {5,     7,     1},

    {6,     9,     2},
    {7,    13,     2},

    {8,    17,     3},
    {9,    25,     3},

    {10,   33,     4},
    {11,   49,     4},

    {12,   65,     5},
    {13,   97,     5},

    {14,  129,     6},
    {15,  193,     6},

    {16,  257,     7},
    {17,  385,     7},

    {18,  513,     8},
    {19,  769,     8},

    {20, 1025,     9},
    {21, 1537,     9},

    {22, 2049,    10},
    {23, 3073,    10},

    {24, 4097,    11},
    {25, 6145,    11},

    {26, 8193,    12},
    {27,12289,    12},

    {28,16385,    13},
    {29,24577,    13}
};

void WriteDistance(BitWriter& bw, const FixedTables& tables, int32_t distance) {
    for(const auto& e : DistanceTable) {
        int32_t max = e.base + ((1 << e.extraBits) - 1);

        if (e.base <= distance && distance <= max) {
            // distance Huffman code
            Emit(bw, tables.dist[e.symbol]);

            // extra distance bits
            if (e.extraBits) {
                bw.Write(distance - e.base, e.extraBits);
            }

            return;
        }
    }
}
