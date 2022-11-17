#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <getopt.h>
#include <map>
#include <string>
#include <volk/volk.h>

#define BUFFER_SIZE 10000000

static void u8_convert_32f(float *output, const uint8_t *input, unsigned int points) {
    for (unsigned int i = 0; i < points; i++) {
        output[i] = ((float)input[i] / 127.5) - 1;
    }
}

static void f32_convert_u8(uint8_t *output, const float *input, unsigned int points) {
    for (unsigned int i = 0; i < points; i++) {
        output[i] = (input[i] + 1) * (255.0f / 2);
    }
}

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
    enum class sampleformat_t { none, u8, s8, s16, f32 };
    sampleformat_t sampleformat_in = sampleformat_t::none;
    sampleformat_t sampleformat_out = sampleformat_t::none;
    std::map<std::string, sampleformat_t> sampleformat_map{{"u8", sampleformat_t::u8}, {"s8", sampleformat_t::s8}, {"s16", sampleformat_t::s16}, {"f32", sampleformat_t::f32}};

    std::string outputfile = "";
    std::string inputfile = "";

    bool output_wav = false;

    int parsed_args = 0;

    int lastarg = 0;
    int opt;
    while ((opt = getopt(argc, argv, "f:c:o:i:w")) != -1) {
        switch (opt) {
        case 'f': { // sample format
            std::string format = optarg;
            if (sampleformat_map.count(format) == 0) {
                fprintf(stderr, "possible values for -f are: u8, s8, s16, f32\n");
                return 1;
            }
            if (lastarg == 'i') {
                sampleformat_in = sampleformat_map[format];
            } else if (lastarg == 'o') {
                sampleformat_out = sampleformat_map[format];
            } else {
                fprintf(stderr, "-f was not after -i or -o\n");
                return 1;
            }
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
        case 'w': { // output_wav
            output_wav = true;
            parsed_args++;
        } break;
        }
        lastarg = opt;
    }

    if ((parsed_args < 3) || (parsed_args > 4)) {
    usage:
        fprintf(stderr,
                "usage: %s -i [input file] (-f {u8, s8, s16, f32}) -o [output file] -f {u8, s8, s16, f32} (-w to output WAV - only works with WAV input)\nnote: stdin/stdout is not supported\n",
                argv[0]);
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

    struct waveheader header;
    input.read((char *)&header, sizeof(waveheader));
    input.seekg(0);

    if ((memcmp(header.signature, "RIFF", 4) == 0) || (memcmp(header.filetype, "WAVE", 4) == 0) || (memcmp(header.formatmarker, "fmt ", 4) == 0)) {
        if (sampleformat_in != sampleformat_t::none) {
            fprintf(stderr, "you may not specify a input sample format for WAV files\n");
            return 1;
        }
        if (sampleformat_out == sampleformat_t::s8 && output_wav) {
            fprintf(stderr, "WAV does not support s8\n");
            return 1;
        }

        input.seekg(0, input.end);
        size_t input_filesize = input.tellg();
        input.seekg(0, input.beg);

        std::map<size_t, sampleformat_t> sampleformat_map_bits{{8, sampleformat_t::u8}, {16, sampleformat_t::s16}};

        if (header.sampletype == 3) {
            if (header.bitdepth != 32) {
                fprintf(stderr, "unsupported WAV format float, %d bits\n", (int)header.bitdepth);
                return 1;
            }
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

        if (output_wav) {
            input.seekg(start_offset);
        } else {
            input.seekg(0);
        }
    } else if (sampleformat_in == sampleformat_t::none) {
        goto usage;
    } else {
        if (output_wav) {
            fprintf(stderr, "WAV output unsupported without WAV input, use iqtools_wavheader instead\n", (int)header.bitdepth);
            return 1;
        }
    }

    std::ofstream output(outputfile, std::ios::binary);
    if (!output.is_open()) {
        fprintf(stderr, "could not open output file\n");
        return 1;
    }

    if (output_wav) {
        output.write((char *)&header, sizeof(struct waveheader));
    }

    std::map<sampleformat_t, uint8_t> typesize_map{
        {sampleformat_t::u8, sizeof(uint8_t)}, {sampleformat_t::s8, sizeof(char)}, {sampleformat_t::s16, sizeof(int16_t)}, {sampleformat_t::f32, sizeof(float)}};
    size_t typesize_in = typesize_map[sampleformat_in];
    size_t typesize_out = typesize_map[sampleformat_out];

    size_t alignment = volk_get_alignment();
    void *buffer_in = volk_malloc(BUFFER_SIZE * typesize_in, alignment);
    float *buffer_f = (float *)volk_malloc(BUFFER_SIZE * sizeof(float), alignment);
    void *buffer_out = volk_malloc(BUFFER_SIZE * typesize_out, alignment);

    size_t written_out_samples = 0;

    while (true) {
        input.read((char *)buffer_in, BUFFER_SIZE * typesize_in);
        size_t read = input.gcount();
        read /= typesize_in;

        switch (sampleformat_in) {
        case sampleformat_t::u8:
            u8_convert_32f(buffer_f, (uint8_t *)buffer_in, read);
            break;
        case sampleformat_t::s8:
            volk_8i_s32f_convert_32f(buffer_f, (int8_t *)buffer_in, 128, read);
            break;
        case sampleformat_t::s16:
            volk_16i_s32f_convert_32f(buffer_f, (int16_t *)buffer_in, 32768, read);
            break;
        case sampleformat_t::f32:
            memcpy(buffer_f, buffer_in, read * sizeof(float));
            break;
        }

        switch (sampleformat_out) {
        case sampleformat_t::u8:
            f32_convert_u8((uint8_t *)buffer_out, buffer_f, read);
            break;
        case sampleformat_t::s8:
            volk_32f_s32f_convert_8i((int8_t *)buffer_out, buffer_f, 127, read);
            break;
        case sampleformat_t::s16:
            volk_32f_s32f_convert_16i((int16_t *)buffer_out, buffer_f, 32767, read);
            break;
        case sampleformat_t::f32:
            memcpy(buffer_out, buffer_f, read * sizeof(float));
            break;
        }

        output.write((char *)buffer_out, read * typesize_out);

        written_out_samples += read;

        if (read != BUFFER_SIZE) {
            break;
        }
    }

    if (output_wav) {
        header.filesize = (written_out_samples * typesize_out) + sizeof(struct waveheader) - 8;
        header.datasize = (written_out_samples * typesize_out);
        output.seekp(0);
        output.write((char *)&header, sizeof(struct waveheader));
    }

    volk_free(buffer_in);
    volk_free(buffer_f);
    volk_free(buffer_out);
    output.close();
    input.close();
    return 0;
}
