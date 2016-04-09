
struct terminal;

struct terminal * terminal_create(struct terminal **term);

void terminal_resize(struct terminal *term, int w, int h);

void terminal_input(struct terminal *term, struct event *ev);

int terminal_get_fd(struct terminal *term);

void terminal_flush(struct terminal *term);

void terminal_set_callback(struct terminal *term, void (*fn) (struct event *, void *), void *user);

