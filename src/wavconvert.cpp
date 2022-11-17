#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <getopt.h>
#include <map>
#include <string>

#define BUFFER_SIZE 1000

struct __attribute((packed)) waveheader {
    char signature[4] = {'R', 'I', 'F', 'F'};
    uint32_t filesize;
    char filetype[4] = {'W', 'A', 'V', 'E'};
    char formatmarker[4] = {'f', 'm', 't', ' '};
    uint32_t formatheaderlength;
    uint16_t sampletype; // 1 for PCM, 3 for floating point
    uint16_t channelcount;
    uint32_t samplerate;
    uint32_t bytespersec;
    uint16_t bytespersample;
    uint16_t bitdepth;
    char datamarker[4] = {'d', 'a', 't', 'a'};
    uint32_t datasize;
};

int main(int argc, char *argv[]) {
    enum class sampleformat_t { u8, s16, f32 } sampleformat_out;
    std::map<std::string, sampleformat_t> sampleformat_map{{"u8", sampleformat_t::u8}, {"s16", sampleformat_t::s16}, {"f32", sampleformat_t::f32}};

    std::string outputfile = "";
    std::string inputfile = "";

    int parsed_args = 0;

    int opt;
    while ((opt = getopt(argc, argv, "f:o:i:")) != -1) {
        switch (opt) {
        case 'f': { // sample format
            std::string format = optarg;
            if (sampleformat_map.count(format) == 0) {
                fprintf(stderr, "possible values for -f are: u8, s16, f32\n");
                return 1;
            }
            sampleformat_out = sampleformat_map[format];
            parsed_args++;
        } break;
        case 'o': { // output file
            outputfile = optarg;
            parsed_args++;
        } break;
        case 'i': { // input file
            inputfile = optarg;
            parsed_args++;
        } break;
        }
    }

    if (parsed_args != 5) {
        fprintf(stderr, "usage: %s -f {u8, s16, f32} -i [input file] -o [output file]\nnote: stdin/stdout is not supported\n", argv[0]);
        return 1;
    }

    if (inputfile == outputfile) {
        fprintf(stderr, "input file may not be the same as output file\n");
        return 1;
    }

    if ((inputfile == "-") || (outputfile == "-")) {
        fprintf(stderr, "stdin/stdout is not supported...\n");
        return 1;
    }

    std::ifstream input(inputfile, std::ios::binary);
    if (!input.is_open()) {
        fprintf(stderr, "could not open input file\n");
        return 1;
    }

    std::ofstream output(outputfile, std::ios::binary);
    if (!output.is_open()) {
        fprintf(stderr, "could not open output file\n");
        return 1;
    }

    input.seekg(0, input.end);
    size_t input_filesize = input.tellg();
    input.seekg(0, input.beg);

    struct waveheader header;
    input.read((char *)&header, sizeof(waveheader));

    if ((memcmp(header.signature, "RIFF", 4) != 0) || (memcmp(header.filetype, "WAVE", 4) != 0) || (memcmp(header.formatmarker, "fmt ", 4) != 0)) {
        fprintf(stderr, "invalid WAV header!\n");
        return 1;
    }

    std::map<size_t, sampleformat_t> sampleformat_map_bits{{8, sampleformat_t::u8}, {16, sampleformat_t::s16}};
    sampleformat_t sampleformat_in;

    if (header.sampletype == 3) {
        sampleformat_in = sampleformat_t::f32;
    } else {
        sampleformat_in = sampleformat_map_bits[header.bitdepth];
    }

    size_t size = 0;
    size_t start_offset = 0;
    char buf[4];
    do {
        input.read(buf, 4);
        if (memcmp(buf, "data", 4) == 0) {
            start_offset = (size_t)input.tellg() + 4;
            if (start_offset > (input_filesize - 1)) {
                start_offset = 0;
            }
            break;
        }
        size += 4;
        if (size > 1000000) {
            break;
        }
    } while (input.gcount() == 4);

    header.formatheaderlength = 16;

    memcpy(header.datamarker, "data", 4);

    switch (sampleformat_out) {
    case sampleformat_t::u8:
        header.sampletype = 1;
        header.bitdepth = 8;
        break;
    case sampleformat_t::s16:
        header.sampletype = 1;
        header.bitdepth = 16;
        break;
    case sampleformat_t::f32:
        header.sampletype = 3;
        header.bitdepth = 32;
        break;
    }

    header.bytespersample = header.bitdepth / 8;
    header.bytespersec = header.bytespersample * header.samplerate * header.channelcount;

    output.write((char *)&header, sizeof(struct waveheader));

    input.seekg(start_offset);

    std::map<sampleformat_t, uint8_t> typesize_map{{sampleformat_t::u8, sizeof(uint8_t)}, {sampleformat_t::s16, sizeof(int16_t)}, {sampleformat_t::f32, sizeof(float)}};
    size_t typesize_in = typesize_map[sampleformat_in];
    size_t typesize_out = typesize_map[sampleformat_out];

    void *buffer_in = malloc(BUFFER_SIZE * typesize_in);
    float *buffer_f = (float *)malloc(BUFFER_SIZE * sizeof(float));
    void *buffer_out = malloc(BUFFER_SIZE * typesize_out);

    while (true) {
        input.read((char *)buffer_in, BUFFER_SIZE * typesize_in);
        size_t read = input.gcount();
        read /= typesize_in;

        for (size_t i = 0; i < read; i++) {
            switch (sampleformat_in) {
            case sampleformat_t::u8:
                buffer_f[i] = (((float)((uint8_t *)buffer_in)[i] / 255) * 2) - 1;
                break;
            case sampleformat_t::s16:
                buffer_f[i] = (float)((int8_t *)buffer_in)[i] / 32768;
                break;
            case sampleformat_t::f32:
                buffer_f[i] = ((float *)buffer_in)[i];
                break;
            }
        }

        for (size_t i = 0; i < read; i++) {
            switch (sampleformat_out) {
            case sampleformat_t::u8:
                ((uint8_t *)buffer_out)[i] = ((buffer_f[i] + 1) * (255.0f / 2));
                break;
            case sampleformat_t::s16:
                ((int16_t *)buffer_out)[i] = buffer_f[i] * 32767;
                break;
            case sampleformat_t::f32:
                ((float *)buffer_out)[i] = buffer_f[i];
                break;
            }
        }

        output.write((char *)buffer_out, read * typesize_out);

        if (read != BUFFER_SIZE) {
            break;
        }
    }

    free(buffer_in);
    free(buffer_f);
    free(buffer_out);

    output.close();
    input.close();
    return 0;
}
