int
width_tags(Bar *bar, BarWidthArg *a)
{
	int w, i;
	Client *c;
	unsigned int occ = 0;
	for (c = bar->mon->cl->clients; c; c = c->next)
		occ |= c->tags == 255 ? 0 : c->tags;

	for (w = 0, i = 0; i < LENGTH(tags); i++) {
		if (!(occ & 1 << i || bar->mon->tagset[bar->mon->seltags] & 1 << i))
			continue;
		w += TEXTW(tags[i]);
	}
	return w;
}

int
draw_tags(Bar *bar, BarDrawArg *a)
{
	int invert;
	int w, x = a->x;
	unsigned int i, occ = 0, urg = 0;
	Client *c;
	Monitor *m = bar->mon;

	for (c = m->cl->clients; c; c = c->next) {
		occ |= c->tags == 255 ? 0 : c->tags;
		if (c->isurgent)
			urg |= c->tags;
	}

	for (i = 0; i < LENGTH(tags); i++) {
		if (!(occ & 1 << i || m->tagset[m->seltags] & 1 << i))
			continue;
		invert = urg & 1 << i;
		w = TEXTW(tags[i]);
		drw_setscheme(drw, scheme[m->tagset[m->seltags] & 1 << i ? SchemeSel : SchemeNorm]);
		drw_text(drw, x, 0, w, bh, lrpad / 2, tags[i], invert);
		x += w;
	}

	return x;
}

int
click_tags(Bar *bar, Arg *arg, BarClickArg *a)
{
	int i = 0, x = lrpad / 2;
	Client *c;
	unsigned int occ = 0;
	for (c = bar->mon->cl->clients; c; c = c->next)
		occ |= c->tags == 255 ? 0 : c->tags;


	do {
		if (!(occ & 1 << i || bar->mon->tagset[bar->mon->seltags] & 1 << i))
			continue;

		x += TEXTW(tags[i]);
	} while (a->rel_x >= x && ++i < LENGTH(tags));
	if (i < LENGTH(tags)) {
		arg->ui = 1 << i;
	}
	return ClkTagBar;
}
