#include <cstddef>
#include <cstdint>
#include <vector>

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

    // stored blocks
    size_t pos = 0;
    while (pos < size) {
        uint16_t len = (uint16_t)std::min<size_t>(65535, size - pos);
        bool final = (pos + len == size);

        out.push_back(final ? 0x01 : 0x00); // BFINAL | BTYPE=00

        uint16_t nlen = ~len;
        out.push_back(len & 0xff);
        out.push_back(len >> 8);

        out.push_back(nlen & 0xff);
        out.push_back(nlen >> 8);

        out.insert(out.end(), data + pos, data + pos + len);
        pos += len;
    }

    // Checksum
    WriteBE32(out, Adler32(data, size));
}

int InflateStored(
    const uint8_t* input,
    size_t inputSize,
    std::vector<uint8_t>& output)
{
    if (inputSize < 6) {
        return -1;
    }

    // zlib header
    uint8_t cmf = input[0];
    uint8_t flg = input[1];

    // DEFLATE
    if ((cmf & 0x0F) != 8) {
        return -1;
    }      

    if (((cmf << 8) | flg) % 31 != 0) {
        return -1;
    }

    // preset dictionary unsupported
    if (flg & 0x20) {
        return -1;
    }

    size_t pos = 2;

    // DEFLATE blocks
    while (true) {
        if (pos >= inputSize - 4) {
            return -1;
        }

        uint8_t header = input[pos++];

        bool final = header & 1;
        uint8_t type = (header >> 1) & 3;

        if (type != 0) {
            return -2;      
        }

        if (pos + 4 > inputSize - 4) {
            return -1;
        }

        uint16_t len = input[pos] | (input[pos + 1] << 8);
        pos += 2;

        uint16_t nlen = input[pos] | (input[pos + 1] << 8);
        pos += 2;

        if ((uint16_t)~len != nlen) {
            return -1;
        }

        if (pos + len > inputSize - 4) {
            return -1;
        }

        output.insert(output.end(), input + pos, input + pos + len);

        pos += len;

        if (final) {
            break;
        }
    }

    // Checksum
    uint32_t expected = ReadBE32(input + inputSize - 4);
    uint32_t actual = Adler32(output.data(), output.size());

    if (actual != expected) {
        return -3;
    }

    return 0;
}
