/**
 *
 * Compiz lazypointer
 *
 * lazypointer.c
 *
 * Copyright (c) 2008 Sam Spilsbury <smspillaz@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Warps your pointer based on window events and actions so that
 * you don't have to.
 **/

#include "math.h"
#include "stdio.h"

#include "throw_options.h"
#include "throw.h"

#define PI 3.1415926

COMPIZ_PLUGIN_20090315 (throw, ThrowPluginVTable); // 'throw' is a reserved keyword

void
ThrowScreen::preparePaint (int ms)
{
    foreach (CompWindow *w, screen->windows ())
    {
	THROW_WINDOW (w);

	if (tw->moving)
	    tw->time += ms;

	if (!tw->moving && (
	    (tw->xVelocity < 0.0f || tw->xVelocity > 0.0f) ||
	    (tw->yVelocity < 0.0f || tw->yVelocity > 0.0f)))
	{

	    /* Catchoff net */
	    if ((fabsf(tw->xVelocity) < 0.2f) ||
	        (fabsf(tw->yVelocity) < 0.2f))
	    {
		tw->xVelocity = 0.0f;
		tw->yVelocity = 0.0f;
	    }

	    tw->xVelocity /= (1.0 + (optionGetFrictionConstant () / 100));
	    tw->yVelocity /= (1.0 + (optionGetFrictionConstant () / 100));
	    int dx = roundf (tw->xVelocity * (optionGetVelocityX ()));
	    int dy = roundf (tw->yVelocity * (optionGetVelocityY ()));

	    if (optionGetConstrainX ())
	    {
		if ((WIN_REAL_X (w) + dx) < 0)
		    dx = 0;
		else if ((WIN_REAL_X (w) + WIN_REAL_W (w) + dx) > screen->width ())
		    dx = 0;
	    }
	    if (optionGetConstrainY ())
	    {
		if ((WIN_REAL_Y (w) + dy) < 0)
		    dy = 0;
		else if ((WIN_REAL_Y (w) + WIN_REAL_H (w) + dy) > screen->height ())
		    dy = 0;
	    }

	    w->move (dx, dy, true);
	    w->syncPosition ();
	}

    }
	
    cScreen->preparePaint (ms);
}

void
ThrowScreen::donePaint ()
{
    foreach (CompWindow *w, screen->windows ())
    {
	THROW_WINDOW (w);
	if (tw->moving ||
	    (tw->xVelocity < 0.0f || tw->xVelocity > 0.0f) ||
	    (tw->yVelocity < 0.0f || tw->yVelocity > 0.0f))
	{
	    tw->cWindow->addDamage ();
	}
    }

    cScreen->donePaint ();
}

void
ThrowWindow::grabNotify (int           x,
			 int           y,
			 unsigned int  state,
			 unsigned int  mask)
{
    if (mask & CompWindowGrabMoveMask)
    {
	THROW_WINDOW (window);

	tw->moving = true;

	tw->time = 0;
	tw->xVelocity = 0.0f;
	tw->yVelocity = 0.0f;
    }

    window->grabNotify (x, y, state, mask);

}

void
ThrowWindow::ungrabNotify ()
{
    THROW_WINDOW (window);

    tw->moving = false;

    window->ungrabNotify ();
}

void
ThrowWindow::moveNotify (int  dx,
			 int  dy,
			 bool immediate)
{
    THROW_WINDOW (window);

    if (tw->moving)
    {
	if (tw->time < 1)
	    tw->time = 1;

	tw->xVelocity = (float) dx / (float) tw->time;
	tw->yVelocity = (float) dy / (float) tw->time;

	tw->time = 1;
    }

    window->moveNotify (dx, dy, immediate);

}
/* Constructor */

ThrowScreen::ThrowScreen (CompScreen *screen) :
    PluginClassHandler<ThrowScreen, CompScreen> (screen),
    cScreen (CompositeScreen::get (screen)),
    windows ()
{
    ScreenInterface::setHandler (screen);
    CompositeScreenInterface::setHandler (cScreen);
}

ThrowWindow::ThrowWindow (CompWindow *window) :
    PluginClassHandler<ThrowWindow, CompWindow> (window),
    window (window),
    cWindow (CompositeWindow::get (window)),
    xVelocity (0.0f),
    yVelocity (0.0f),
    time (0),
    moving (false)
{
    WindowInterface::setHandler (window);
}

bool
ThrowPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION))
	 return false;

    return true;
}
