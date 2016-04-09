
struct event {

  union {
    struct keyboard_event {
      int key;
      int scancode;
      int action;
      int mods;
      int ascii;
      int ucs4;
    } k;

    struct terminal_event {
      unsigned int x;
      unsigned int y;
      const uint32_t *ch;
      size_t   len;
      unsigned char fg[3];
      unsigned char bg[3];
    } t;

  };
};

