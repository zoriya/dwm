static Systray *systray = NULL;
static unsigned long systrayorientation = _NET_SYSTEM_TRAY_ORIENTATION_HORZ;

int
width_systray(Bar *bar, BarWidthArg *a)
{
	unsigned int w = 0;
	Client *i;
	Atom flags;

	if (!systray)
		return 1;
	if (showsystray) {
		for (i = systray->icons; i; i = i->next) {
			if (!(flags = getatomprop(i, netatom[NetWmStateSkipTaskbar])))
				w += i->w + systrayspacing;
		}
	}
	return w ? w + lrpad - systrayspacing : 0;
}

int
draw_systray(Bar *bar, BarDrawArg *a)
{
	if (!showsystray) {
		if (systray)
			XMoveWindow(dpy, systray->win, -500, bar->by);
		return a->x;
	}

	XSetWindowAttributes wa;
	Client *i;
	unsigned int w;
	Atom flags;

	if (!systray) {
		/* init systray */
		if (!(systray = (Systray *)calloc(1, sizeof(Systray))))
			die("fatal: could not malloc() %u bytes\n", sizeof(Systray));

		wa.override_redirect = True;
		wa.event_mask = ButtonPressMask|ExposureMask;
		wa.border_pixel = 0;
		#if BAR_ALPHA_PATCH
		wa.background_pixel = 0;
		wa.colormap = cmap;
		systray->win = XCreateWindow(dpy, root, bar->bx + a->x + lrpad / 2, bar->by + vertpadbar / 2, MAX(a->w + 40, 1), drw->fonts->h, 0, depth,
						InputOutput, visual,
						CWOverrideRedirect|CWBorderPixel|CWBackPixel|CWColormap|CWEventMask, &wa); // CWBackPixmap
		#else
		wa.background_pixel = scheme[SchemeNorm][ColBg].pixel;
		systray->win = XCreateSimpleWindow(dpy, root, bar->bx + a->x + lrpad / 2, bar->by + vertpadbar / 2, MIN(a->w, 1), drw->fonts->h, 0, 0, scheme[SchemeNorm][ColBg].pixel);
		XChangeWindowAttributes(dpy, systray->win, CWOverrideRedirect|CWBackPixel|CWBorderPixel|CWEventMask, &wa);
		#endif // BAR_ALPHA_PATCH

		XSelectInput(dpy, systray->win, SubstructureNotifyMask);
		XChangeProperty(dpy, systray->win, netatom[NetSystemTrayOrientation], XA_CARDINAL, 32,
				PropModeReplace, (unsigned char *)&systrayorientation, 1);
		#if BAR_ALPHA_PATCH
		XChangeProperty(dpy, systray->win, netatom[NetSystemTrayVisual], XA_VISUALID, 32,
				PropModeReplace, (unsigned char *)&visual->visualid, 1);
		#endif // BAR_ALPHA_PATCH
		XChangeProperty(dpy, systray->win, netatom[NetWMWindowType], XA_ATOM, 32,
				PropModeReplace, (unsigned char *)&netatom[NetWMWindowTypeDock], 1);
		XMapRaised(dpy, systray->win);
		XSetSelectionOwner(dpy, netatom[NetSystemTray], systray->win, CurrentTime);
		if (XGetSelectionOwner(dpy, netatom[NetSystemTray]) == systray->win) {
			sendevent(root, xatom[Manager], StructureNotifyMask, CurrentTime, netatom[NetSystemTray], systray->win, 0, 0);
			XSync(dpy, False);
		} else {
			fprintf(stderr, "dwm: unable to obtain system tray.\n");
			free(systray);
			systray = NULL;
			return a->x;
		}
	}

	systray->bar = bar;

	drw_setscheme(drw, scheme[SchemeNorm]);
	for (w = 0, i = systray->icons; i; i = i->next) {
		if ((flags = getatomprop(i, netatom[NetWmStateSkipTaskbar])))
			continue;
		#if BAR_ALPHA_PATCH
		wa.background_pixel = 0;
		#else
		wa.background_pixel = scheme[SchemeNorm][ColBg].pixel;
		#endif // BAR_ALPHA_PATCH
		XChangeWindowAttributes(dpy, i->win, CWBackPixel, &wa);
		XMapRaised(dpy, i->win);
		i->x = w;
		XMoveResizeWindow(dpy, i->win, i->x, 0, i->w, i->h);
		w += i->w;
		if (i->next)
			w += systrayspacing;
		if (i->mon != bar->mon)
			i->mon = bar->mon;
	}

	XMoveResizeWindow(dpy, systray->win, bar->bx + a->x + lrpad / 2, (w ? bar->by : -bar->by) + vertpadbar / 2, MAX(w, 1), drw->fonts->h);
	return a->x + a->w;
}

int
click_systray(Bar *bar, Arg *arg, BarClickArg *a)
{
	return -1;
}

void
removesystrayicon(Client *i)
{
	Client **ii;

	if (!showsystray || !i)
		return;
	for (ii = &systray->icons; *ii && *ii != i; ii = &(*ii)->next);
	if (ii)
		*ii = i->next;
	free(i);
	drawbarwin(systray->bar);
}

void
resizerequest(XEvent *e)
{
	XResizeRequestEvent *ev = &e->xresizerequest;
	Client *i;

	if ((i = wintosystrayicon(ev->window))) {
		updatesystrayicongeom(i, ev->width, ev->height);
		drawbarwin(systray->bar);
	}
}

void
updatesystrayicongeom(Client *i, int w, int h)
{
	if (i) {
		i->h = drw->fonts->h;
		if (w == h)
			i->w = drw->fonts->h;
		else if (h == drw->fonts->h)
			i->w = w;
		else
			i->w = (int) ((float)drw->fonts->h * ((float)w / (float)h));
		applysizehints(i, &(i->x), &(i->y), &(i->w), &(i->h), False);
		/* force icons into the systray dimensions if they don't want to */
		if (i->h > drw->fonts->h) {
			if (i->w == i->h)
				i->w = drw->fonts->h;
			else
				i->w = (int) ((float)drw->fonts->h * ((float)i->w / (float)i->h));
			i->h = drw->fonts->h;
		}
		if (i->w > 2*drw->fonts->h)
			i->w = drw->fonts->h;
	}
}

void
updatesystrayiconstate(Client *i, XPropertyEvent *ev)
{
	long flags;
	int code = 0;

	if (!showsystray || !systray || !i || ev->atom != xatom[XembedInfo] ||
			!(flags = getatomprop(i, xatom[XembedInfo])))
		return;

	if (flags & XEMBED_MAPPED && !i->tags) {
		i->tags = 1;
		code = XEMBED_WINDOW_ACTIVATE;
		XMapRaised(dpy, i->win);
		setclientstate(i, NormalState);
	}
	else if (!(flags & XEMBED_MAPPED) && i->tags) {
		i->tags = 0;
		code = XEMBED_WINDOW_DEACTIVATE;
		XUnmapWindow(dpy, i->win);
		setclientstate(i, WithdrawnState);
	}
	else
		return;
	sendevent(i->win, xatom[Xembed], StructureNotifyMask, CurrentTime, code, 0,
			systray->win, XEMBED_EMBEDDED_VERSION);
}

Client *
wintosystrayicon(Window w)
{
	if (!systray)
		return NULL;
	Client *i = NULL;
	if (!showsystray || !w)
		return i;
	for (i = systray->icons; i && i->win != w; i = i->next);
	return i;
}
