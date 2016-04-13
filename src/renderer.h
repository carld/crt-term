
struct renderer;

struct renderer * renderer_create(struct options o);
void renderer_destroy(struct renderer *s);

void render(float time, struct renderer * s);
void renderer_putc(struct renderer *scn, unsigned ch, unsigned posx, unsigned posy, unsigned char fg, unsigned char bg);
void renderer_get_glyph_wh(struct renderer *scn, int wh[]) ;
void renderer_set_framebuffer_size(struct renderer *, int, int);
