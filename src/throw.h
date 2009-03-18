/*
 * Copyright © 2009 Sam Spilsbury
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Sam Spilsbury not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Sam Spilsbury makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * SAM SPILSBURY DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL SAM SPILSBURY BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Sam Spilsbury <smspillaz@gmail.com
 */

#include <core/core.h>
#include <core/pluginclasshandler.h>

#include <composite/composite.h>

class ThrowScreen :
    public ScreenInterface,
    public CompositeScreenInterface,
    public PluginClassHandler <ThrowScreen, CompScreen>,
    public ThrowOptions
{
    public:

	ThrowScreen (CompScreen *s);

	CompositeScreen *cScreen;

	CompWindowList windows;

	void preparePaint (int);
	void donePaint ();
};

class ThrowWindow :
	public WindowInterface,
	public PluginClassHandler <ThrowWindow, CompWindow>
{
	public:

	    ThrowWindow (CompWindow *w);

	    void grabNotify (int          x,
			     int          y,
			     unsigned int state,
			     unsigned int mask);

	    void ungrabNotify ();
	    void moveNotify (int dx,
			     int dy,
			     bool immediate);
			
	    CompWindow *window;
	    CompositeWindow *cWindow;

	    float  xVelocity;
	    float  yVelocity;
	    int  time;
	    bool moving;
};

#define THROW_SCREEN(s)						       \
    ThrowScreen *ts = ThrowScreen::get (s)

#define THROW_WINDOW(w)							\
    ThrowWindow *tw = ThrowWindow::get (w)

#define WIN_REAL_X(w) (w->x () - w->input ().left)
#define WIN_REAL_Y(w) (w->y () - w->input ().top)

#define WIN_REAL_W(w) (w->width () + w->input ().left + w->input ().right)
#define WIN_REAL_H(w) (w->height () + w->input ().top + w->input ().bottom)

class ThrowPluginVTable :
    public CompPlugin::VTableForScreenAndWindow<ThrowScreen, ThrowWindow>
{
    public:

	bool init ();
};
