#include <cstdint>
#include <cstdio>
#include <fstream>
#include <getopt.h>
#include <map>
#include <string>

#define BUFFER_SIZE 1000

int main(int argc, char *argv[]) {
    enum class sampleformat_t { u8, s8, s16, f32 };
    sampleformat_t sampleformat_in;
    sampleformat_t sampleformat_out;
    std::map<std::string, sampleformat_t> sampleformat_map{{"u8", sampleformat_t::u8}, {"s8", sampleformat_t::s8}, {"s16", sampleformat_t::s16}, {"f32", sampleformat_t::f32}};

    std::string outputfile = "";
    std::string inputfile = "";

    int parsed_args = 0;

    int lastarg = 0;
    int opt;
    while ((opt = getopt(argc, argv, "f:c:o:i:")) != -1) {
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
        }
        lastarg = opt;
    }

    if (parsed_args != 4) {
        fprintf(stderr, "usage: %s -i [input file] -f {u8, s8, s16, f32} -o [output file] -f {u8, s8, s16, f32}\nnote: stdin/stdout is not supported\n", argv[0]);
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

    std::map<sampleformat_t, uint8_t> typesize_map{
        {sampleformat_t::u8, sizeof(uint8_t)}, {sampleformat_t::s8, sizeof(char)}, {sampleformat_t::s16, sizeof(int16_t)}, {sampleformat_t::f32, sizeof(float)}};
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
            case sampleformat_t::s8:
                buffer_f[i] = (float)((int8_t *)buffer_in)[i] / 128;
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
                ((uint8_t *)buffer_out)[i] = ((buffer_f[i] + 1) * (255 / 2));
                break;
            case sampleformat_t::s8:
                ((int8_t *)buffer_out)[i] = buffer_f[i] * 127;
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
