#include "Log.h"
#include "wavplay.h"

uint8_t wav_read(struct wav_ctx* ctx, const char *path)
{
    UINT bw; 
    uint8_t uRes = 0;
	uRes = f_open(&ctx->file, path, FA_READ);
    if (uRes) return uRes;
    
    uRes = f_read(&ctx->file, &ctx->wave_header, sizeof(ctx->wave_header), &bw);
    if (uRes != FR_OK){
        LOG(EERROR, "error: '%s' does not contain a riff/wave header", path);
        f_close(&ctx->file);
        return uRes;
    }
    if ((ctx->wave_header.riff_id != ID_RIFF) ||
        (ctx->wave_header.wave_id != ID_WAVE)) {
        LOG(EERROR, "error: '%s' is not a riff/wave file", path);
        f_close(&ctx->file);
        return uRes;
    }
    unsigned int more_chunks = 1;
    do {
        uRes = f_read(&ctx->file, &ctx->chunk_header, sizeof(ctx->chunk_header), &bw);
        if (uRes != FR_OK){
            LOG(EERROR, "error: '%s' does not contain a data chunk", path);
            f_close(&ctx->file);
            return uRes;
        }
        switch (ctx->chunk_header.id) {
        case ID_FMT:
            uRes = f_read(&ctx->file, &ctx->chunk_fmt, sizeof(ctx->chunk_fmt), &bw);
            if (uRes != FR_OK){
                LOG(EERROR, "error: '%s' has incomplete format chunk", path);
                f_close(&ctx->file);
                return -1;
            }
            /* If the format header is larger, skip the rest */
            if (ctx->chunk_header.sz > sizeof(ctx->chunk_fmt))
                f_lseek(&ctx->file, ctx->chunk_header.sz - sizeof(ctx->chunk_fmt));
            break;
        case ID_DATA:
            /* Stop looking for chunks */
            more_chunks = 0;
            break;
        default:
            /* Unknown chunk, skip bytes */
            f_lseek(&ctx->file, ctx->chunk_header.sz);
        }
    } while (more_chunks);

    int channels = ctx->chunk_fmt.num_channels;
    int rate = ctx->chunk_fmt.sample_rate;
    int bits = ctx->chunk_fmt.bits_per_sample;
    int size = ctx->wave_header.riff_sz - 36;
    
    LOG(EDEBUG, "rate:%d, bit:%d, channels:%d, size:%d", 
        rate, bits, channels, size);

    return uRes;
}

void wav_write(const char *path, int rate, int bits, int channels, void *pdata, int len)
{
    FILE* file = fopen(path, "wb");

    struct wav_header header;
    header.riff_id = ID_RIFF;
    header.riff_sz = 0;
    header.riff_fmt = ID_WAVE;
    header.fmt_id = ID_FMT;
    header.fmt_sz = 16;
    header.audio_format = FORMAT_PCM;
    header.num_channels = channels;
    header.sample_rate = rate;
    header.bits_per_sample = bits;
    header.byte_rate = (header.bits_per_sample / 8) * channels * rate;
    header.block_align = channels * (header.bits_per_sample / 8);
    header.data_id = ID_DATA;
    header.data_sz = len;
    header.riff_sz = header.data_sz + sizeof(header) - 8;
    fseek(file, 0, SEEK_SET);
    fwrite(&header, sizeof(struct wav_header), 1, file);
    fwrite(pdata, len, 1, file);
    fclose(file);
}
