#include <array>
#include <cstdint>
#include <vector>

__inline__ static uint32_t bitMask(int32_t n) {
  return n == 32 ? 0xffffffffU : ((1U << n) - 1);
}

struct BitWriter {
  std::vector<uint8_t> &output;

  uint32_t buffer = 0;
  int32_t bits = 0;

  BitWriter(std::vector<uint8_t> &o) : output(o) {}

  void Write(uint32_t v, int32_t n) {
    buffer |= (v & bitMask(n)) << bits;
    bits += n;

    while (8 <= bits) {
      output.push_back(buffer & 0xff);
      buffer >>= 8;
      bits -= 8;
    }
  }

  void Flush() {
    if (bits != 0) {
      output.push_back(buffer & 0xff);
      buffer = 0;
      bits = 0;
    }
  }
};

struct BitReader {
  size_t pos = 0;
  const std::vector<uint8_t> &data;

  uint32_t buffer = 0;
  int32_t bits = 0;

  BitReader(const std::vector<uint8_t> &d, size_t offset = 0)
      : pos(offset), data(d) {}

  uint32_t Read(int32_t n) {
    while (bits < n) {
      if (pos >= data.size()) {
        return 0;
      }

      buffer |= uint32_t(data[pos++]) << bits;
      bits += 8;
    }

    uint32_t value = buffer & bitMask(n);

    buffer >>= n;
    bits -= n;

    return value;
  }

  uint32_t Peek(int32_t n) {
    while (bits < n) {
      if (pos >= data.size()) {
        return 0;
      }

      buffer |= uint32_t(data[pos++]) << bits;
      bits += 8;
    }

    return buffer & bitMask(n);
  }

  void Drop(int32_t n) {
    buffer >>= n;
    bits -= n;
  }
};

struct HuffCode {
  uint16_t code;
  uint8_t bits;
};

static uint16_t ReverseBits(uint16_t x, int32_t n) {
  uint16_t reversed = 0;
  while ((n--) != 0) {
    reversed = (reversed << 1) | (x & 1);
    x >>= 1;
  }

  return reversed;
}

static void BuildHuffman(const std::vector<uint8_t> &lengths,
                         std::vector<HuffCode> &table) {
  int32_t code = 0;
  std::array<int32_t, 16> next = {};
  std::array<int32_t, 16> count = {};
  for (uint8_t len : lengths) {
    if (len != 0u) {
      count[len]++;
    }
  }

  table.resize(lengths.size());

  for (int32_t bits = 1; bits <= 15; bits++) {
    code = (code + count[bits - 1]) << 1;
    next[bits] = code;
  }

  for (size_t i = 0; i < lengths.size(); i++) {
    uint8_t len = lengths[i];

    if (len != 0u) {
      table[i].bits = len;
      table[i].code = ReverseBits(next[len]++, len);
    } else {
      table[i] = {0, 0};
    }
  }
}

struct HuffmanDecoder {
  // 1 << 9 should cover all the huffman codes
  std::array<HuffCode, 512> table;
};

static void BuildDecoder(const std::vector<HuffCode> &codes,
                         HuffmanDecoder &dec) {
  // Mark entries as invalid
  for (auto &entry : dec.table) {
    entry = {0xffff, 0};
  }

  for (size_t sym = 0; sym < codes.size(); sym++) {
    auto code = codes[sym];
    if (code.bits == 0u) {
      continue;
    }

    int32_t fill = 1 << (9 - code.bits);

    for (int32_t i = 0; i < fill; i++) {
      int32_t idx = code.code | (i << code.bits);

      dec.table[idx].code = (uint16_t)sym;
      dec.table[idx].bits = code.bits;
    }
  }
}

struct FixedTables {
  std::vector<HuffCode> literal;
  std::vector<HuffCode> distance;
};

static FixedTables MakeFixedTables() {
  FixedTables tables;
  std::vector<uint8_t> literalLenghts(288);

  for (int32_t i = 0; i <= 143; i++) {
    literalLenghts[i] = 8;
  }
  for (int32_t i = 144; i <= 255; i++) {
    literalLenghts[i] = 9;
  }
  for (int32_t i = 256; i <= 279; i++) {
    literalLenghts[i] = 7;
  }
  for (int32_t i = 280; i <= 287; i++) {
    literalLenghts[i] = 8;
  }

  std::vector<uint8_t> distanceLengths(32, 5);

  BuildHuffman(literalLenghts, tables.literal);
  BuildHuffman(distanceLengths, tables.distance);

  return tables;
}

static void Emit(BitWriter &bw, const HuffCode &c) { bw.Write(c.code, c.bits); }

struct Match {
  int32_t length;
  int32_t distance;
};

// PNG spec defines that compression method 0 (used in this project),
//   should use LZ77 window size of not more than 32K.
struct LZ77 {
  static constexpr int32_t WINDOW = 32768;
  static constexpr int32_t HASH = 1 << 15;

  std::vector<int> head;
  std::vector<int> prevMatch;

  LZ77(size_t size) : head(HASH, -1), prevMatch(size, -1) {}

  static uint32_t Hash(const uint8_t *p) {
    return ((p[0] * 251U) ^ (p[1] * 911U) ^ (p[2] * 3571U)) & (HASH - 1);
  }

  void Insert(const uint8_t *data, size_t pos, size_t size) {
    if (size <= pos + 2) {
      return;
    }

    uint32_t h = Hash(data + pos);

    prevMatch[pos] = head[h];
    head[h] = (int)pos;
  }

  Match Find(const uint8_t *data, size_t size, size_t pos) {
    Match best{0, 0};

    if (size <= pos + 2) {
      return best;
    }

    uint32_t h = Hash(data + pos);

    int32_t matchPos = head[h];
    int32_t depth = 0;

    while (0 <= matchPos && depth++ < 128) {
      size_t distance = pos - matchPos;

      if (distance > WINDOW) {
        break;
      }

      int32_t len = 0;
      while (len < 258 && pos + len < size &&
             data[matchPos + len] == data[pos + len]) {
        len++;
      }

      if (best.length < len && 3 <= len) {
        best.length = len;
        best.distance = distance;

        if (len == 258) {
          break;
        }
      }

      matchPos = prevMatch[matchPos];
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
    {257, 3, 0},   {258, 4, 0},   {259, 5, 0},   {260, 6, 0},   {261, 7, 0},
    {262, 8, 0},   {263, 9, 0},   {264, 10, 0},  {265, 11, 1},  {266, 13, 1},
    {267, 15, 1},  {268, 17, 1},  {269, 19, 2},  {270, 23, 2},  {271, 27, 2},
    {272, 31, 2},  {273, 35, 3},  {274, 43, 3},  {275, 51, 3},  {276, 59, 3},
    {277, 67, 4},  {278, 83, 4},  {279, 99, 4},  {280, 115, 4}, {281, 131, 5},
    {282, 163, 5}, {283, 195, 5}, {284, 227, 5}, {285, 258, 0}};

void WriteLength(BitWriter &bw, const FixedTables &tables, int32_t length) {
  for (const auto &entry : LengthTable) {
    int32_t max = entry.base + ((1 << entry.extraBits) - 1);
    if (length >= entry.base && length <= max) {
      // Huffman symbol
      Emit(bw, tables.literal[entry.symbol]);

      // extra length bits
      if (entry.extraBits != 0) {
        bw.Write(length - entry.base, entry.extraBits);
      }

      return;
    }
  }
}

static int32_t ReadLength(BitReader &br, int32_t symbol) {
  const auto &entry = LengthTable[symbol - 257];
  return entry.base + ((entry.extraBits != 0) ? br.Read(entry.extraBits) : 0);
}

struct DistanceCode {
  int32_t symbol;
  int32_t base;
  int32_t extraBits;
};

static const std::vector<DistanceCode> DistanceTable = {
    {0, 1, 0},       {1, 2, 0},       {2, 3, 0}, {3, 4, 0},

    {4, 5, 1},       {5, 7, 1},

    {6, 9, 2},       {7, 13, 2},

    {8, 17, 3},      {9, 25, 3},

    {10, 33, 4},     {11, 49, 4},

    {12, 65, 5},     {13, 97, 5},

    {14, 129, 6},    {15, 193, 6},

    {16, 257, 7},    {17, 385, 7},

    {18, 513, 8},    {19, 769, 8},

    {20, 1025, 9},   {21, 1537, 9},

    {22, 2049, 10},  {23, 3073, 10},

    {24, 4097, 11},  {25, 6145, 11},

    {26, 8193, 12},  {27, 12289, 12},

    {28, 16385, 13}, {29, 24577, 13}};

void WriteDistance(BitWriter &bw, const FixedTables &tables, int32_t distance) {
  for (const auto &entry : DistanceTable) {
    int32_t max = entry.base + ((1 << entry.extraBits) - 1);

    if (entry.base <= distance && distance <= max) {
      // distance Huffman code
      Emit(bw, tables.distance[entry.symbol]);

      // extra distance bits
      if (entry.extraBits != 0) {
        bw.Write(distance - entry.base, entry.extraBits);
      }

      return;
    }
  }
}

static int32_t ReadDistance(BitReader &br, int32_t symbol) {
  const auto &entry = DistanceTable[symbol];
  return entry.base + ((entry.extraBits != 0) ? br.Read(entry.extraBits) : 0);
}

static uint16_t Decode(BitReader &br, const HuffmanDecoder &dec) {
  auto entry = dec.table[br.Peek(9)];
  br.Drop(entry.bits);
  return entry.code;
}
