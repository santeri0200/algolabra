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
    // zlib headers
    out.push_back(0x78);
    out.push_back(0x01); // fastest compression

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

            for(int i = 0; i < m.length; i++)
                lz.Insert(data, pos + i, size);

            pos += m.length;
        } else {
            Emit(bw, tables.lit[data[pos]]);
            lz.Insert(data, pos, size);

            pos++;
        }
    }

    // end block
    Emit(bw, tables.lit[256]);

    bw.Flush();

    WriteBE32(out, Adler32(data,size));
}
