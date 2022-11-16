#include <cstdint>
#include <cstdio>
#include <fstream>
#include <getopt.h>
#include <map>
#include <string>

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
    enum class sampleformat_t { u8, s16, f32 } sampleformat;
    std::map<std::string, sampleformat_t> sampleformat_map{{"u8", sampleformat_t::u8}, {"s16", sampleformat_t::s16}, {"f32", sampleformat_t::f32}};

    uint32_t samplerate;
    uint16_t channelcount;

    std::string outputfile = "";
    std::string inputfile = "";

    int parsed_args = 0;

    int opt;
    while ((opt = getopt(argc, argv, "f:r:c:o:i:")) != -1) {
        switch (opt) {
        case 'f': { // sample format
            std::string format = optarg;
            if (sampleformat_map.count(format) == 0) {
                fprintf(stderr, "possible values for -f are: u8, s16, f32\n");
                return 1;
            }
            sampleformat = sampleformat_map[format];
            parsed_args++;
        } break;
        case 'r': { // sample rate
            std::string samplerate_str = optarg;
            try {
                long samplerate_i = std::stol(samplerate_str);
                if (samplerate_i > UINT32_MAX) {
                    fprintf(stderr, "samplerate too large\n");
                    return 1;
                }
                samplerate = samplerate_i;
                parsed_args++;
            } catch (...) {
                fprintf(stderr, "samplerate must be a number\n");
                return 1;
            }
        } break;
        case 'c': { // channel count
            std::string channelcount_str = optarg;
            try {
                long channelcount_i = std::stol(channelcount_str);
                if (channelcount_i > UINT16_MAX) {
                    fprintf(stderr, "channelcount too large\n");
                    return 1;
                }
                channelcount = channelcount_i;
                parsed_args++;
            } catch (...) {
                fprintf(stderr, "channelcount must be a number\n");
                return 1;
            }
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
        fprintf(stderr, "usage: %s -f {u8, s16, f32} -r [samplerate] -c [channelcount] -i [input file] -o [output file]\nnote: stdin/stdout is not supported\n", argv[0]);
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

    std::map<sampleformat_t, uint8_t> bitdepth_map{{sampleformat_t::u8, 8}, {sampleformat_t::s16, 16}, {sampleformat_t::f32, 32}};
    uint8_t bitdepth = bitdepth_map[sampleformat];

    struct waveheader header;
    header.filesize = input_filesize + sizeof(struct waveheader) - 8; // this will probably overflow in most usecases... in that case the software using it does not care either...
    header.formatheaderlength = 16;

    header.sampletype = 1; // PCM
    if (sampleformat == sampleformat_t::f32) {
        header.sampletype = 3; // float
    }

    header.channelcount = channelcount;
    header.samplerate = samplerate;

    header.bytespersec = samplerate * channelcount * (bitdepth / 8);
    header.bytespersample = (bitdepth / 8) * channelcount;
    header.bitdepth = bitdepth;

    header.datasize = input_filesize;

    output.write((char *)&header, sizeof(struct waveheader));

    output << input.rdbuf();

    output.close();
    input.close();
    return 0;
}
