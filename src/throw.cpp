/**
 *
 * Compiz Momentum Windows (Throw)
 *
 * throw.c
 *
 * Copyright (c) 2011 Josh Bialkowski <jbialk@mit.edu>
 *
 * original code:
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
 * If you're moving a window and you let go then the window continues to move 
 * with "momentum".
 *
 * 
 *
 **/

#include <cmath>
#include <cstdio>

#include "throw_options.h"
#include "throw.h"



inline static int winActualX(CompWindow* w){ return w->x () - w->border ().left; }
inline static int winActualY(CompWindow* w){ return w->y () - w->border ().top; }

inline static int winActualW(CompWindow* w){ return w->width () + w->border ().left + w->border ().right; }
inline static int winActualH(CompWindow* w){ return w->height () + w->border ().top + w->border ().bottom; }


/// register the plugin with compiz
COMPIZ_PLUGIN_20090315 (throw, ThrowPluginVTable); // 'throw' is a reserved keyword






/**
 *  It seems this method is called once per frame of compiz rendering. It's
 *  given an integer number of milliseconds, I'm assuming, elapsed since the
 *  previous call
 */
void ThrowScreen::preparePaint (int ms)
{
    CompWindowList::iterator it;
    for( it = screen->windows().begin(); it != screen->windows().end(); it++ )
    {
        CompWindow*     w = *it;
        ThrowWindow*    tw = ThrowWindow::get (w);

        tw->shouldDamage = false;
        tw->state->preparePaint(ms);
    }

    cScreen->preparePaint (ms);
}



void ThrowWindow::State::Grabbed::preparePaint(int ms)
{
    m_tw->time += ms;
    m_tw->shouldDamage = true;
}

void ThrowWindow::State::Ungrabbed::preparePaint(int ms)
{
    // otherwise, if it's not moving but it has some finite velocity, then
    // we update it's position and integrate it's dynamics
    if ((m_tw->xVelocity < 0.0f || m_tw->xVelocity > 0.0f)
        || (m_tw->yVelocity < 0.0f || m_tw->yVelocity > 0.0f) )
    {
        m_tw->shouldDamage = true;

        // we'll stop it from moving when it's velocity get's below this
        // value
        if ((fabsf(m_tw->xVelocity) < 0.05f) ||
            (fabsf(m_tw->yVelocity) < 0.05f))
        {
            m_tw->xVelocity = 0.0f;
            m_tw->yVelocity = 0.0f;
        }

        m_tw->xVelocity /= (1.0 + (m_tw->optionGetFrictionConstant() / 100));
        m_tw->yVelocity /= (1.0 + (m_tw->optionGetFrictionConstant() / 100));
        int dx = roundf (m_tw->xVelocity * (m_tw->optionGetVelocityX ()));
        int dy = roundf (m_tw->yVelocity * (m_tw->optionGetVelocityY ()));

        if (optionGetConstrainX ())
        {
            if ((winActualX(w) + dx) < 0)
            {
                dx = -winActualX(w);
                m_tw->xVelocity = 0.0f;
            }
            else if ((winActualX(w) + winActualW(w) + dx) > screen->width ())
            {
                dx = screen->width() - ( winActualX(w) + winActualW(w) );
                m_tw->xVelocity = 0.0f;
            }
        }

        if (optionGetConstrainY ())
        {
            if ( (winActualY(w) + dy) < 0)
            {
                dy = -winActualY(w);
                m_tw->yVelocity = 0.0f;
            }
            else if ( (winActualY(w) + winActualH(w) + dy) > screen->height ())
            {
                dy = screen->height() - (winActualY(w) + winActualH(w));
                m_tw->yVelocity = 0.0f;
            }
        }

        w->move (dx, dy, true);
        w->syncPosition ();
    }
}





void ThrowScreen::donePaint ()
{
    CompWindowList::iterator it;
    for( it = screen->windows().begin(); it != screen->windows().end(); it++ )
    {
        CompWindow*     w = *it;
        ThrowWindow*    tw = ThrowWindow::get (w);
        if(tw->shouldDamage)
            tw->cWindow->addDamage();
    }

    cScreen->donePaint ();
}






void ThrowWindow::grabNotify (
            int           x,
            int           y,
            unsigned int  state,
            unsigned int  mask)
{

    if (mask & CompWindowGrabMoveMask)
    {
        ThrowWindow* tw = ThrowWindow::get (window);
        tw->state->grabNotify();
    }

    window->grabNotify (x, y, state, mask);

}


void ThrowWindow::State::Ungrabbed::grabNotify()
{
    m_tw->time = 0;
    m_tw->xVelocity = 0.0f;
    m_tw->yVelocity = 0.0f;
    m_tw->state = m_tw->grabbed;
}






void ThrowWindow::ungrabNotify ()
{
    ThrowWindow* tw = ThrowWindow::get (window);
    tw->state->ungrabNotify();
    window->ungrabNotify ();
}

void ThrowWindow::State::Grabbed::unGrabNotify()
{
    m_tw->state = m_tw->ungrabbed;
}





void ThrowWindow::moveNotify (
            int  dx,
            int  dy,
            bool immediate)
{
    ThrowWindow* tw = ThrowWindow::get (window);
    tw->state->moveNotify(dx,dy);
    window->moveNotify (dx, dy, immediate);
}



void ThrowWindow::State::Grabbed::moveNotify(
            int  dx,
            int  dy,
            bool immediate)
{
    if (m_tw->time < 1)
        m_tw->time = 1;

    m_tw->xVelocity = (float) dx / (float) tw->time;
    m_tw->yVelocity = (float) dy / (float) tw->time;

    m_tw->time = 1;
}





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
    moving (false),
    shouldDamage (false)
{
    WindowInterface::setHandler (window);
    m_grabbed   = new ThrowWindow::State::Grabbed(this);
    m_ungrabbed = new ThrowWindow::State::Ungrabbed(this);
    m_state     = m_grabbed;
}








bool ThrowPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION))
        return false;
    return true;
}
