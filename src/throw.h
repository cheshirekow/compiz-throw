/*
 * Copyright © 2011 Josh Bialkowski
 * 
 * Original code:
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
 * Author: Josh Bialkowski <jbialk@mit.edu>
 * 
 * original:
 * Author: Sam Spilsbury <smspillaz@gmail.com
 *
 * How the plugin works:
 * The plugin recognizes three different states for a top-level window:
 * "grabbing", "grabbed", "ungrabbing", and "ungrabbed". At each frame of
 * compiz rendering, we do different things depending on the state.
 * * In the "grabbed" state, we sample how much the user has moved the window
 *   since the last frame, and record that delta to a ring buffer
 * * In the "released" state, we simulate the dynamics of the window
 *
 * In addition to the frame callback from compiz, we also perform actions on
 * movement events from the user.
 * * In the "grabbed" state we update the current position of the window, so
 *   that we can record a delta in the frame
 */

#include <core/core.h>
#include <core/pluginclasshandler.h>

#include <composite/composite.h>


/**
 *  If I'm understanding this correctly, a ThrowScreen object is a wrapper
 *  for a compiz native screen object. We create one of these objects and
 *  associate it with a compiz screen object so that we can add some data to
 *  the compiz object
 */
class ThrowScreen :
    public ScreenInterface,
    public CompositeScreenInterface,
    public PluginClassHandler <ThrowScreen, CompScreen>,
    public ThrowOptions
{
    public:
        CompositeScreen *cScreen;
        CompWindowList windows;

        ThrowScreen (CompScreen *s);
        void preparePaint (int ms);
        void donePaint ();
};



/**
 *  Like ThrowScreen, a wrapper object which adds data to the compiz data 
 *  structure for a window. A window is a top-level application window, meaning
 *  a child (in the sense of X) of the root window... or the window manager 
 *  (in this case compiz)
 */
class ThrowWindow :
    public WindowInterface,
    public PluginClassHandler <ThrowWindow, CompWindow>
{
    public:
        //forward declaration, see definition below
        class State;

        CompWindow         *window;
        CompositeWindow    *cWindow;

        float  xVelocity;
        float  yVelocity;
        int    time;
        bool   moving;
        bool   shouldDamage;

        // pointer to all the states, for use in construction and destruction
        State* m_grabbed;
        State* m_ungrabbed;
        State* m_state;

        ThrowWindow (CompWindow *w);

        /// when a window is grabbed we need to stop simulating it's motion 
        /// and start recording its movement
        void grabNotify ( int x, int y, unsigned int state, unsigned int mask);

        /// when a window is released (ungrabbed) then we need to look at it's
        /// motion over the past few frames to determine it's current
        /// velocity
        void ungrabNotify ();

        /// when the user moves the window, we need to record the motion so
        /// that we can determine the windows "velocity"
        void moveNotify (int dx, int dy, bool immediate);
};

/**
 *
 */
class ThrowWindow::State
{
    public:
        // forward declarations, see definintions below
        class Grabbed;
        class Ungrabbed;

        /// pointer to the window that owns this state
        ThrowWindow*            m_tw;

        /// constructor simply stores a pointer to the window that owns this
        /// state object
        State(ThrowWindow* window): m_tw(window) {}

        virtual void grabNotify( ){};
        virtual void unGrabNotify( ){};
        virtual void moveNotify(int dx, int dy, bool immediate){};
        virtual void preparePiaint(int ms)=0;
};


class ThrowWindow::State::Grabbed :
    public ThrowWindow::State
{
    public:
        Grabbed(ThrowWindow* window): State(window) {}
        virtual void unGrabNotify( );
        virtual void moveNotify(int dx, int dy, bool immediate);
        virtual void preparePiaint(int ms);
};


class ThrowWindow::State::Ungrabbed :
    public ThrowWindow::State
{
    public:
        Ungrabbed(ThrowWindow* window): State(window) {}
        virtual void grabNotify( );
        virtual void preparePiaint(int ms);
};


/**
 *  This class is just is for the compiz plugin API
 */
class ThrowPluginVTable :
    public CompPlugin::VTableForScreenAndWindow<ThrowScreen, ThrowWindow>
{
    public:
      bool init ();
};
