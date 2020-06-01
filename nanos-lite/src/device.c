#include "common.h"

#define NAME(key) \
  [_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [_KEY_NONE] = "NONE",
  _KEYS(NAME)
};

size_t events_read(void *buf, size_t len) {
  return 0;
}

static char dispinfo[128] __attribute__((used));

void dispinfo_read(void *buf, off_t offset, size_t len) {
    strncpy(buf, dispinfo + offset, len);
}

extern void getScreen(int* p_width, int* p_height);

void fb_write(const void *buf, off_t offset, size_t len) {
    assert(offset % 4 == 0 && len % 4 == 0);
    int index, screen_x1, screen_y1, screen_y2;
    int width = 0, height = 0;
    getScreen(&width, &height);

    index = offset / 4;
    screen_y1 = index / width;
    screen_x1 = index % width;

    index = (offset + len) / 4;
    screen_y2 = index / width;

    assert(screen_y2 >= screen_y1);

    if (screen_y2 == screen_y1) {
        _draw_rect(buf, screen_x1, screen_y1, len / 4, 1);
        return;
    }
    int tmpw = width - screen_x1;
    if (screen_y2 - screen_y1 == 1) {
        _draw_rect(buf, screen_x1, screen_y1, tmpw, 1);
        _draw_rect(buf + tmpw * 4, 0, screen_y2, len / 4 - tmpw, 1);
        return;
    }

    _draw_rect(buf, screen_x1, screen_y1, tmpw, 1);
    int tmpy = screen_y2 - screen_y1 - 1;
    _draw_rect(buf + tmpw * 4, 0, screen_y1 + 1, width, tmpy);
    _draw_rect(buf + tmpw * 4 + tmpy * width * 4, 0, screen_y2, len / 4 - tmpw - tmpy * width, 1);
}

void init_device() {
  _ioe_init();

  // TODO: print the string to array `dispinfo` with the format
  // described in the Navy-apps convention
  int width = 0, height = 0;
  getScreen(&width, &height);
  sprintf(dispinfo, "WIDTH: %d\nHEIGHT: %d\n", width, height);
}
