#include <Arduino.h>
#include "gifdec.h"

#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))

typedef struct Entry
{
    uint16_t length;
    uint16_t prefix;
    uint8_t suffix;
} Entry;

typedef struct Table
{
    int bulk;
    int nentries;
    Entry *entries;
} Table;

gd_GIF* GIF::info(void) {
    return &infoGIF;
}
size_t buffer_read(Buffer *buffer, uint8_t *out, size_t size)
{
    if (buffer->position + size > buffer->length)
    {
        size = buffer->length - buffer->position;
    }
    memcpy(out, buffer->buffer + buffer->position, size);
    buffer->position += size;
    return size;
}

off_t buffer_seek(Buffer *buffer, off_t offset, int whence)
{
    if (whence == SEEK_SET)
    {
        buffer->position = offset;
    }
    else if (whence == SEEK_CUR)
    {
        buffer->position += offset;
    }
    else if (whence == SEEK_END)
    {
        buffer->position = buffer->length + offset;
    }
    if (buffer->position > buffer->length)
    {
        buffer->position = buffer->length;
    }
    return buffer->position;
}

static uint16_t buffer_read_num(gd_GIF *gif)
{
    uint8_t bytes[2];
    buffer_read(&gif->image, bytes, 2);
    return bytes[0] + (((uint16_t)bytes[1]) << 8);
}

bool GIF::gd_open_gif_memory(const uint8_t *buf, size_t len,int colorSize)
{
    uint8_t sigver[3];
    uint16_t width, height, depth;
    uint8_t fdsz, bgidx, aspect;
    int gct_sz;
    uint8_t *bgcolor;

    // Initialize the buffer
    infoGIF.image.buffer = buf;
    infoGIF.image.length = len;
    infoGIF.image.position = 0;
    infoGIF.colorSize = colorSize;

    // Header
    buffer_read(&infoGIF.image, sigver, 3);
    if (memcmp(sigver, "GIF", 3) != 0)
    {
        Serial.printf("invalid signature\n");
        return false;
    }
    // Version
    buffer_read(&infoGIF.image, sigver, 3);
    if (memcmp(sigver, "89a", 3) != 0)
    {
        Serial.printf("invalid version\n");
        return false;
    }
    // Width x Height
    width = buffer_read_num(&infoGIF);
    height = buffer_read_num(&infoGIF);

    // FDSZ
    buffer_read(&infoGIF.image, &fdsz, 1);
    // Presence of GCT
    if (!(fdsz & 0x80))
    {
        Serial.printf("no global color table\n");
        return false;
    }
    // Color Space's Depth
    depth = ((fdsz >> 4) & 7) + 1;
    // GCT Size
    gct_sz = 1 << ((fdsz & 0x07) + 1);
    // Background Color Index
    buffer_read(&infoGIF.image, &bgidx, 1);
    // Aspect Ratio
    buffer_read(&infoGIF.image, &aspect, 1);
    /* Create gd_GIF Structure. */
    infoGIF.width = width;
    infoGIF.height = height;
    infoGIF.depth = depth;
    /* Read GCT */
    infoGIF.gct.size = gct_sz;
    buffer_read(&infoGIF.image, infoGIF.gct.colors, 3 *infoGIF.gct.size);

    infoGIF.palette = &infoGIF.gct;
    infoGIF.bgindex = bgidx;
    infoGIF.frame = (uint8_t *)calloc(4, width * height);
    if (!infoGIF.frame)
    {
        Serial.printf("Cannot allocate frame\n");
        return false;
    }
    infoGIF.canvas = &infoGIF.frame[width * height];
    if (infoGIF.bgindex)
        memset(infoGIF.frame, infoGIF.bgindex, infoGIF.width * infoGIF.height);
    bgcolor = &infoGIF.palette->colors[infoGIF.bgindex * 3];
    if (bgcolor[0] || bgcolor[1] || bgcolor[2])
        for (int i = 0; i < infoGIF.width * infoGIF.height; i++)
            memcpy(&infoGIF.canvas[i * infoGIF.colorSize], bgcolor, infoGIF.colorSize);
    infoGIF.anim_start = buffer_seek(&infoGIF.image, 0, SEEK_CUR);
    return true;
}

static void
discard_sub_blocks(gd_GIF *gif)
{
    uint8_t size;

    do
    {
        buffer_read(&gif->image, &size, 1);
        buffer_seek(&gif->image, size, SEEK_CUR);
    } while (size);
}

static void
read_plain_text_ext(gd_GIF *gif)
{
    if (gif->plain_text)
    {
        uint16_t tx, ty, tw, th;
        uint8_t cw, ch, fg, bg;
        off_t sub_block;
        buffer_seek(&gif->image, 1, SEEK_CUR); /* block size = 12 */
        tx = buffer_read_num(gif);
        ty = buffer_read_num(gif);
        tw = buffer_read_num(gif);
        th = buffer_read_num(gif);
        buffer_read(&gif->image, &cw, 1);
        buffer_read(&gif->image, &ch, 1);
        buffer_read(&gif->image, &fg, 1);
        buffer_read(&gif->image, &bg, 1);
        sub_block = buffer_seek(&gif->image, 0, SEEK_CUR);
        gif->plain_text(gif, tx, ty, tw, th, cw, ch, fg, bg);
        buffer_seek(&gif->image, sub_block, SEEK_SET);
    }
    else
    {
        /* Discard plain text metadata. */
        buffer_seek(&gif->image, 13, SEEK_CUR);
    }
    /* Discard plain text sub-blocks. */
    discard_sub_blocks(gif);
}

static void
read_graphic_control_ext(gd_GIF *gif)
{
    uint8_t rdit;

    /* Discard block size (always 0x04). */
    buffer_seek(&gif->image, 1, SEEK_CUR);
    buffer_read(&gif->image, &rdit, 1);
    gif->gce.disposal = (rdit >> 2) & 3;
    gif->gce.input = rdit & 2;
    gif->gce.transparency = rdit & 1;
    gif->gce.delay = buffer_read_num(gif);
    buffer_read(&gif->image, &gif->gce.tindex, 1);
    /* Skip block terminator. */
    buffer_seek(&gif->image, 1, SEEK_CUR);
}

static void
read_comment_ext(gd_GIF *gif)
{
    if (gif->comment)
    {
        off_t sub_block = buffer_seek(&gif->image, 0, SEEK_CUR);
        gif->comment(gif);
        buffer_seek(&gif->image, sub_block, SEEK_SET);
    }
    /* Discard comment sub-blocks. */
    discard_sub_blocks(gif);
}

static void
read_application_ext(gd_GIF *gif)
{
    uint8_t app_id[8];
    uint8_t app_auth_code[3];

    /* Discard block size (always 0x0B). */
    buffer_seek(&gif->image, 1, SEEK_CUR);
    /* Application Identifier. */
    buffer_read(&gif->image, app_id, 8);
    /* Application Authentication Code. */
    buffer_read(&gif->image, app_auth_code, 3);
    if (!strncmp((char *)app_id, "NETSCAPE", sizeof(app_id)))
    {
        /* Discard block size (0x03) and constant byte (0x01). */
        buffer_seek(&gif->image, 2, SEEK_CUR);
        gif->loop_count = buffer_read_num(gif);
        /* Skip block terminator. */
        buffer_seek(&gif->image, 1, SEEK_CUR);
    }
    else if (gif->application)
    {
        off_t sub_block = buffer_seek(&gif->image, 0, SEEK_CUR);
        gif->application(gif, app_id, app_auth_code);
        buffer_seek(&gif->image, sub_block, SEEK_SET);
        discard_sub_blocks(gif);
    }
    else
    {
        discard_sub_blocks(gif);
    }
}

static void
read_ext(gd_GIF *gif)
{
    uint8_t label;

    buffer_read(&gif->image, &label, 1);
    switch (label)
    {
    case 0x01:
        read_plain_text_ext(gif);
        break;
    case 0xF9:
        read_graphic_control_ext(gif);
        break;
    case 0xFE:
        read_comment_ext(gif);
        break;
    case 0xFF:
        read_application_ext(gif);
        break;
    default:
        fprintf(stderr, "unknown extension: %02X\n", label);
    }
}

static Table *
new_table(int key_size)
{
    int key;
    int init_bulk = MAX(1 << (key_size + 1), 0x100);
    Table *table = (Table *)malloc(sizeof(*table) + sizeof(Entry) * init_bulk);
    if (table)
    {
        table->bulk = init_bulk;
        table->nentries = (1 << key_size) + 2;
        table->entries = (Entry *)&table[1];
        for (key = 0; key < (1 << key_size); key++)
            table->entries[key] = (Entry){1, 0xFFF, key};
    }
    return table;
}

/* Add table entry. Return value:
 *  0 on success
 *  +1 if key size must be incremented after this addition
 *  -1 if could not realloc table */
static int
add_entry(Table **tablep, uint16_t length, uint16_t prefix, uint8_t suffix)
{
    Table *table = *tablep;
    if (table->nentries == table->bulk)
    {
        table->bulk *= 2;
        table = (Table *)realloc(table, sizeof(*table) + sizeof(Entry) * table->bulk);
        if (!table)
            return -1;
        table->entries = (Entry *)&table[1];
        *tablep = table;
    }
    table->entries[table->nentries] = (Entry){length, prefix, suffix};
    table->nentries++;
    if ((table->nentries & (table->nentries - 1)) == 0)
        return 1;
    return 0;
}

static uint16_t
get_key(gd_GIF *gif, int key_size, uint8_t *sub_len, uint8_t *shift, uint8_t *byte)
{
    int bits_read;
    int rpad;
    int frag_size;
    uint16_t key;

    key = 0;
    for (bits_read = 0; bits_read < key_size; bits_read += frag_size)
    {
        rpad = (*shift + bits_read) % 8;
        if (rpad == 0)
        {
            /* Update byte. */
            if (*sub_len == 0)
            {
                buffer_read(&gif->image, sub_len, 1); /* Must be nonzero! */
                if (*sub_len == 0)
                    return 0x1000;
            }
            buffer_read(&gif->image, byte, 1);
            (*sub_len)--;
        }
        frag_size = MIN(key_size - bits_read, 8 - rpad);
        key |= ((uint16_t)((*byte) >> rpad)) << bits_read;
    }
    /* Clear extra bits to the left. */
    key &= (1 << key_size) - 1;
    *shift = (*shift + key_size) % 8;
    return key;
}

/* Compute output index of y-th input line, in frame of height h. */
static int
interlaced_line_index(int h, int y)
{
    int p; /* number of lines in current pass */

    p = (h - 1) / 8 + 1;
    if (y < p) /* pass 1 */
        return y * 8;
    y -= p;
    p = (h - 5) / 8 + 1;
    if (y < p) /* pass 2 */
        return y * 8 + 4;
    y -= p;
    p = (h - 3) / 4 + 1;
    if (y < p) /* pass 3 */
        return y * 4 + 2;
    y -= p;
    /* pass 4 */
    return y * 2 + 1;
}

/* Decompress image pixels.
 * Return 0 on success or -1 on out-of-memory (w.r.t. LZW code table). */
static int
read_image_data(gd_GIF *gif, int interlace)
{
    uint8_t sub_len, shift, byte;
    int init_key_size, key_size, table_is_full;
    int frm_off, frm_size, str_len, i, p, x, y;
    uint16_t key, clear, stop;
    int ret;
    Table *table;
    Entry entry;
    off_t start, end;

    buffer_read(&gif->image, &byte, 1);
    key_size = (int)byte;
    if (key_size < 2 || key_size > 8)
        return -1;

    start = buffer_seek(&gif->image, 0, SEEK_CUR);
    discard_sub_blocks(gif);
    end = buffer_seek(&gif->image, 0, SEEK_CUR);
    buffer_seek(&gif->image, start, SEEK_SET);
    clear = 1 << key_size;
    stop = clear + 1;
    table = new_table(key_size);
    key_size++;
    init_key_size = key_size;
    sub_len = shift = 0;
    key = get_key(gif, key_size, &sub_len, &shift, &byte); /* clear code */
    frm_off = 0;
    ret = 0;
    frm_size = gif->fw * gif->fh;
    while (frm_off < frm_size)
    {
        if (key == clear)
        {
            key_size = init_key_size;
            table->nentries = (1 << (key_size - 1)) + 2;
            table_is_full = 0;
        }
        else if (!table_is_full)
        {
            ret = add_entry(&table, str_len + 1, key, entry.suffix);
            if (ret == -1)
            {
                free(table);
                return -1;
            }
            if (table->nentries == 0x1000)
            {
                ret = 0;
                table_is_full = 1;
            }
        }
        key = get_key(gif, key_size, &sub_len, &shift, &byte);
        if (key == clear)
            continue;
        if (key == stop || key == 0x1000)
            break;
        if (ret == 1)
            key_size++;
        entry = table->entries[key];
        str_len = entry.length;
        for (i = 0; i < str_len; i++)
        {
            p = frm_off + entry.length - 1;
            x = p % gif->fw;
            y = p / gif->fw;
            if (interlace)
                y = interlaced_line_index((int)gif->fh, y);
            gif->frame[(gif->fy + y) * gif->width + gif->fx + x] = entry.suffix;
            if (entry.prefix == 0xFFF)
                break;
            else
                entry = table->entries[entry.prefix];
        }
        frm_off += str_len;
        if (key < table->nentries - 1 && !table_is_full)
            table->entries[table->nentries - 1].suffix = entry.suffix;
    }
    free(table);
    if (key == stop)
        buffer_read(&gif->image, &sub_len, 1); /* Must be zero! */
    buffer_seek(&gif->image, end, SEEK_SET);
    return 0;
}

/* Read image.
 * Return 0 on success or -1 on out-of-memory (w.r.t. LZW code table). */
static int
read_image(gd_GIF *gif)
{
    uint8_t fisrz;
    int interlace;

    /* Image Descriptor. */
    gif->fx = buffer_read_num(gif);
    gif->fy = buffer_read_num(gif);

    if (gif->fx >= gif->width || gif->fy >= gif->height)
        return -1;

    gif->fw = buffer_read_num(gif);
    gif->fh = buffer_read_num(gif);

    gif->fw = MIN(gif->fw, gif->width - gif->fx);
    gif->fh = MIN(gif->fh, gif->height - gif->fy);

    buffer_read(&gif->image, &fisrz, 1);
    interlace = fisrz & 0x40;
    /* Ignore Sort Flag. */
    /* Local Color Table? */
    if (fisrz & 0x80)
    {
        /* Read LCT */
        Serial.printf("Local color palette\n");
        gif->lct.size = 1 << ((fisrz & 0x07) + 1);
        buffer_read(&gif->image, gif->lct.colors, 3 * gif->lct.size);
        gif->palette = &gif->lct;
    }
    else
        gif->palette = &gif->gct;
    /* Image Data. */
    return read_image_data(gif, interlace);
}

static void
render_frame_rect(gd_GIF *gif, uint8_t *buffer)
{
    int i, j, k;
    uint8_t index, *color;
    i = gif->fy * gif->width + gif->fx;
    for (j = 0; j < gif->fh; j++)
    {
        for (k = 0; k < gif->fw; k++)
        {
            index = gif->frame[(gif->fy + j) * gif->width + gif->fx + k];
            color = &gif->palette->colors[index * 3];
            if (!gif->gce.transparency || index != gif->gce.tindex)
            {
                // Convert 24 bit color to RGB565
                uint16_t usRGB565;
                unsigned char *p;
                usRGB565 = (((*color) >> 3) << 11);     // R
                usRGB565 |= ((*(color + 1) >> 2) << 5); // G
                usRGB565 |= (*(color + 2) >> 3);        // B
                usRGB565 = __builtin_bswap16(usRGB565);
                memcpy(&buffer[(i + k) * gif->colorSize], &usRGB565, gif->colorSize);
            }
        }
        i += gif->width;
    }
}

static void
dispose(gd_GIF *gif)
{
    int i, j, k;
    uint8_t *bgcolor;
    switch (gif->gce.disposal)
    {
    case 2: /* Restore to background color. */
        bgcolor = &gif->palette->colors[gif->bgindex * 3];

        // Convert 24 bit color to RGB565
        uint16_t usRGB565;
        unsigned char *p;
        usRGB565 = ((bgcolor[gif->bgindex] >> 3) << 11); // R
        usRGB565 |= ((p[gif->bgindex + 1] >> 2) << 5);   // G
        usRGB565 |= (p[gif->bgindex + 2] >> 3);          // B

        i = gif->fy * gif->width + gif->fx;
        for (j = 0; j < gif->fh; j++)
        {
            for (k = 0; k < gif->fw; k++)
                memcpy(&gif->canvas[(i + k) * gif->colorSize], &usRGB565, gif->colorSize);
            i += gif->width;
        }
        break;
    case 3: /* Restore to previous, i.e., don't update canvas.*/
        break;
    default:
        /* Add frame non-transparent pixels to canvas. */
        render_frame_rect(gif, gif->canvas);
    }
}

/* Return 1 if got a frame; 0 if got GIF trailer; -1 if error. */
int GIF::gd_get_frame()
{
    uint8_t sep;

    dispose(&infoGIF);
    buffer_read(&infoGIF.image, &sep, 1);
    while (sep != ',')
    {
        if (sep == ';')
            return 0;
        if (sep == '!')
            read_ext(&infoGIF);
        else
            return -1;
        buffer_read(&infoGIF.image, &sep, 1);
    }
    if (read_image(&infoGIF) == -1)
        return -1;
    return 1;
}

void GIF::gd_render_frame(uint8_t *buffer)
{
    memcpy(buffer, infoGIF.canvas, infoGIF.width * infoGIF.height * infoGIF.colorSize);
    render_frame_rect(&infoGIF, buffer);
}

int gd_is_bgcolor(gd_GIF *gif, uint8_t color[3])
{
    return !memcmp(&gif->palette->colors[gif->bgindex * 3], color, 3);
}

void GIF::gd_rewind()
{
    buffer_seek(&infoGIF.image,infoGIF.anim_start, SEEK_SET);
}

void GIF::gd_close_gif()
{
    free(infoGIF.frame);
}
