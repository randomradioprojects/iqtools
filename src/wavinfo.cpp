#include <cstdint>
#include <cstdio>
#include <cstring>
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
    std::string inputfile = "";

    int parsed_args = 0;

    int opt;
    while ((opt = getopt(argc, argv, "i:")) != -1) {
        switch (opt) {
        case 'i': { // input file
            inputfile = optarg;
            parsed_args++;
        } break;
        }
    }

    if (parsed_args != 1) {
        fprintf(stderr, "usage: %s -i [input file]\nnote: stdin/stdout is not supported\n", argv[0]);
        return 1;
    }

    if (inputfile == "-") {
        fprintf(stderr, "stdin/stdout is not supported...\n");
        return 1;
    }

    std::ifstream input(inputfile, std::ios::binary);
    if (!input.is_open()) {
        fprintf(stderr, "could not open input file\n");
        return 1;
    }

    input.seekg(0, input.end);
    size_t input_filesize = input.tellg();
    input.seekg(0, input.beg);

    struct waveheader header;
    input.read((char *)&header, sizeof(struct waveheader));

    if (memcmp(header.signature, "RIFF", 4)) {
        printf("RIFF signature not found\n");
        return 1;
    }

    if (memcmp(header.filetype, "WAVE", 4)) {
        printf("WAVE signature not found\n");
        return 1;
    }

    if (memcmp(header.formatmarker, "fmt ", 4)) {
        printf("format marker not found\n");
        return 1;
    }

    // https://www.mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/Docs/rfc2361.txt
    printf("sampletype: ");
    switch (header.sampletype) {
    case 1:
        printf("PCM\n");
        break;
    case 3:
        printf("IEEE Float\n");
        break;
    default:
        printf("unknown");
        break;
    }

    printf("channel count: %u\n", (unsigned int)header.channelcount);
    printf("samplerate: %lu\n", (long unsigned int)header.samplerate);
    printf("bit depth: %u\n", (unsigned int)header.bitdepth);

    size_t len_secs = (input_filesize - sizeof(struct waveheader)) / (size_t)header.bytespersec;

    printf("length (based on file size): %zu minutes (%zu seconds)\n", len_secs / 60, len_secs);

    input.close();
    return 0;
}
