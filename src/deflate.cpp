#include <cstddef>
#include <cstdint>
#include <vector>

#include "deflate/primitives.cpp"

__inline__ static uint32_t ReadBE32(const uint8_t* p) {
  return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8)  | uint32_t(p[3]);
}

__inline__ static void WriteBE32(std::vector<uint8_t>& out, uint32_t x) {
  out.push_back((x >> 24) & 0xFF);
  out.push_back((x >> 16) & 0xFF);
  out.push_back((x >>  8) & 0xFF);
  out.push_back((x >>  0) & 0xFF);
}

static uint32_t Adler32(const uint8_t* data, size_t len) {
    uint32_t a = 1;
    uint32_t b = 0;

    for (size_t i = 0; i < len; ++i) {
        a += data[i];
        if (a >= 65521) {
            a -= 65521;
        }

        b += a;
        b %= 65521;
    }

    return (b << 16) | a;
}

void DeflateStored(const uint8_t* data, size_t size, std::vector<uint8_t>& out) {
    out.push_back(0x78); // zlib headers
    out.push_back(0x01); // Huffman fixed codes

    BitWriter bw(out);

    // final fixed block
    bw.Write(1, 1); // BFINAL
    bw.Write(1, 2); // BTYPE=01

    size_t pos = 0;
    LZ77 lz(size);
    FixedTables tables = MakeFixedTables();

    while(pos < size) {
        Match m = lz.Find(data, size, pos);

        if (m.length >= 3) {
            WriteLength(bw, tables, m.length);
            WriteDistance(bw, tables, m.distance);

            for(int32_t i = 0; i < m.length; i++) {
                lz.Insert(data, pos + i, size);
            }

            pos += m.length;
        } else {
            Emit(bw, tables.literal[data[pos]]);
            lz.Insert(data, pos, size);

            pos++;
        }
    }

    // end block
    Emit(bw, tables.literal[256]);

    bw.Flush();

    WriteBE32(out, Adler32(data,size));
}

int InflateStored(const std::vector<uint8_t>& data, std::vector<uint8_t>& out) {
    if (data.size() < 6) {
        return -1;
    }

    if (data[0] != 0x78) {
        return -1;
    }

    BitReader br(data, 2); // start after zlib header

    FixedTables tables = MakeFixedTables();

    HuffmanDecoder litDec {};
    HuffmanDecoder distDec {};

    BuildDecoder(tables.literal, litDec);
    BuildDecoder(tables.distance, distDec);

    int32_t final = br.Read(1);
    int32_t type = br.Read(2);

    if ((final == 0) || type != 1) {
        return -1;
    }

    while (true) {
        int32_t sym = Decode(br, litDec);

        if (sym < 256) {
            out.push_back((uint8_t)sym);
            continue;
        }

        if (sym == 256) {
            break;
        }

        int32_t length = ReadLength(br, sym);
        int32_t distSym = Decode(br, distDec);
        int32_t distance = ReadDistance(br, distSym);

        if (distance <= 0 || distance > static_cast<int>(out.size())) {
            return -1;
        }

        size_t start = out.size() - distance;

        for (int32_t i = 0; i < length; ++i) {
            out.push_back(out[start + i]);
        }
    }

    uint32_t stored = ReadBE32(&data[data.size() - 4]);
    uint32_t actual = Adler32(out.data(), out.size());

    return stored == actual ? 0 : -1;
}
