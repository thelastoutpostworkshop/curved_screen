#ifndef GIFDEC_H
#define GIFDEC_H

// In your header file
#ifdef __cplusplus
extern "C"
{
#endif

#define colorOutputSize 2 // 8 bit color as output

    typedef struct
    {
        const uint8_t *buffer;
        size_t length;
        size_t position;
    } Buffer;

    typedef struct gd_Palette
    {
        int size;
        uint8_t colors[0x100 * 3];
    } gd_Palette;

    typedef struct gd_GCE
    {
        uint16_t delay;
        uint8_t tindex;
        uint8_t disposal;
        int input;
        int transparency;
    } gd_GCE;

    typedef struct gd_GIF
    {
        Buffer image;
        off_t anim_start;
        uint16_t width, height;
        uint16_t depth;
        uint16_t loop_count;
        gd_GCE gce;
        gd_Palette *palette;
        gd_Palette lct, gct;
        void (*plain_text)(
            struct gd_GIF *gif, uint16_t tx, uint16_t ty,
            uint16_t tw, uint16_t th, uint8_t cw, uint8_t ch,
            uint8_t fg, uint8_t bg);
        void (*comment)(struct gd_GIF *gif);
        void (*application)(struct gd_GIF *gif, uint8_t id[8], uint8_t auth[3]);
        uint16_t fx, fy, fw, fh;
        uint8_t bgindex;
        uint8_t *canvas, *frame;
    } gd_GIF;

    class GIF
    {
    public:
        bool gd_open_gif_memory(const uint8_t *buf, size_t len);
        gd_GIF* info();
        int gd_get_frame();
        void gd_render_frame(uint8_t *buffer);
        int gd_is_bgcolor(gd_GIF *gif, uint8_t color[3]);
        void gd_rewind();
        void gd_close_gif();
    };

#ifdef __cplusplus
}
#endif
#endif /* GIFDEC_H */
