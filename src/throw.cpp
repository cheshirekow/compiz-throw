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

        tw->m_shouldDamage = false;
        tw->m_state->preparePaint(this, ms);
    }

    cScreen->preparePaint (ms);
}



void ThrowWindow::State::Grabbed::preparePaint(ThrowScreen* ts, int ms)
{
    m_ring.bake(ms);
    m_tw->m_shouldDamage = true;
}

void ThrowWindow::State::Ungrabbed::preparePaint(ThrowScreen* ts, int ms)
{
    // otherwise, if it's not moving but it has some finite velocity, then
    // we update it's position and integrate it's dynamics
    if ((m_tw->m_vx < 0.0f || m_tw->m_vx > 0.0f)
        || (m_tw->m_vy < 0.0f || m_tw->m_vy > 0.0f) )
    {
        CompWindow* w = m_tw->window;

        m_tw->m_shouldDamage = true;

        // we'll stop it from moving when it's velocity get's below this
        // value
        if (fabsf(m_tw->m_vx) < 0.005f)
            m_tw->m_vx = 0.0f;

        if (fabsf(m_tw->m_vy) < 0.005f)
            m_tw->m_vy = 0.0f;

        // velocity decreases with time according to friction constant
        m_tw->m_vx /= (1.0 + (ts->optionGetFrictionConstant() / 100));
        m_tw->m_vy /= (1.0 + (ts->optionGetFrictionConstant() / 100));

        // velocity is stored in pixels per millisecond
        float dx = roundf (m_tw->m_vx * ms);
        float dy = roundf (m_tw->m_vy * ms);

        // update the stored precise position with the deltas
        m_tw->m_x += dx;
        m_tw->m_y += dy;

        // clamp the precise position to an integer
        int x = roundf(m_tw->m_x);
        int y = roundf(m_tw->m_y);


        // if the option is set to constrain position within the current screen
        // then clamp any positions that are outside that
        if ( ts->optionGetConstrainX() )
        {
            if ( x <  w->border().left )
            {
                x = w->border().left;
                m_tw->m_x  = x;
                m_tw->m_vx = 0.0f;
            }
            else if ( (x + w->border().left + winActualW(w) ) > screen->width() )
            {
                x = screen->width() - winActualW(w) - w->border().left;
                m_tw->m_x  = x;
                m_tw->m_vx = 0.0f;
            }
        }

        if ( ts->optionGetConstrainY() )
        {
            if ( y < w->border().top )
            {
                //printf("clamping y because %i < %i\n",y,w->border().top);
                y = w->border().top;
                m_tw->m_y  = y;
                m_tw->m_vy = 0.0f;
            }
            else if ( ( y - w->border().top + winActualH(w) ) > screen->height ())
            {
                //printf("clamping y because %i > %i\n",( y - w->border().top + winActualH(w) ),screen->height ());
                y = screen->height() - winActualH(w) + w->border().top;
                m_tw->m_y  = y;
                m_tw->m_vy = 0.0f;
            }
        }

        w->moveToViewportPosition(x, y, true);
    }
}





void ThrowScreen::donePaint ()
{
    CompWindowList::iterator it;
    for( it = screen->windows().begin(); it != screen->windows().end(); it++ )
    {
        CompWindow*     w = *it;
        ThrowWindow*    tw = ThrowWindow::get (w);
        if(tw->m_shouldDamage)
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
        tw->m_state->grabNotify();
    }

    window->grabNotify (x, y, state, mask);
}


void ThrowWindow::State::Ungrabbed::grabNotify()
{
    m_tw->m_x  = m_tw->window->x();
    m_tw->m_y  = m_tw->window->y();
    m_tw->m_vx = 0.0f;
    m_tw->m_vy = 0.0f;
    m_tw->m_state = m_tw->m_grabbed;

    //printf("Window state changing from ungrabbed to grabbed\n");
}




void ThrowWindow::ungrabNotify ()
{
    ThrowWindow* tw = ThrowWindow::get (window);
    tw->m_state->ungrabNotify();
    window->ungrabNotify ();
}


void ThrowWindow::State::Grabbed::ungrabNotify()
{
    m_tw->m_state = m_tw->m_ungrabbed;
    m_tw->m_x = m_tw->window->x();
    m_tw->m_y = m_tw->window->y();
    m_ring.getAverage(m_tw->m_vx,m_tw->m_vy);
    m_ring.reset();

    //printf("Window state changing from grabbed to ungrabbed\n");
    //printf("   velocity: (%5.3f,%5.3f)\n",m_tw->m_vx,m_tw->m_vy);
}





void ThrowWindow::moveNotify (
            int  dx,
            int  dy,
            bool immediate)
{
    ThrowWindow* tw = ThrowWindow::get (window);
    tw->m_state->moveNotify(dx,dy);
    window->moveNotify (dx, dy, immediate);
}



void ThrowWindow::State::Grabbed::moveNotify(
            int  dx,
            int  dy)
{
    m_ring.store(dx,dy);
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
    m_vx (0.0f),
    m_vy (0.0f),
    m_shouldDamage (false)
{
    WindowInterface::setHandler (window);
    m_grabbed   = new ThrowWindow::State::Grabbed(this);
    m_ungrabbed = new ThrowWindow::State::Ungrabbed(this);
    m_state     = m_grabbed;
}

ThrowWindow::~ThrowWindow()
{
    delete m_grabbed;
    delete m_ungrabbed;
}








bool ThrowPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION))
        return false;
    return true;
}



void ThrowWindow::Ring::reset()
{
    i_next  = 0;
    m_full  = false;
    m_dx[0] = 0;
    m_dy[0] = 0;
}

void ThrowWindow::Ring::store(int dx, int dy)
{
    m_dx[i_next] += dx;
    m_dy[i_next] += dy;
}

void ThrowWindow::Ring::bake(int dt)
{
    m_dt[i_next++] = dt;

    if(i_next > THROW_WINDOW_RING_SIZE)
    {
        i_next = 0;
        m_full = true;
    }

    m_dx[i_next] = 0;
    m_dy[i_next] = 0;
    m_dt[i_next] = 0;
}


void ThrowWindow::Ring::getAverage(float& vx, float& vy)
{
    vx = 0;
    vy = 0;

    if(!m_full && i_next == 0)
        return;

    double dt = 0;

    int i_end = m_full ? THROW_WINDOW_RING_SIZE : i_next-1;

    for(int i=0; i < i_end; i++)
    {
        vx += m_dx[i];
        vy += m_dy[i];
        dt += m_dt[i];
    }

    vx /= dt;
    vy /= dt;
}


