/* GTK - The GIMP Toolkit
 * Copyright (C) 2012, One Laptop Per Child.
 * Copyright (C) 2014, Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author(s): Carlos Garnacho <carlosg@gnome.org>
 */
#include "config.h"
#include <gtk/gtk.h>
#include <gtk/gtkgesturelongpress.h>
#include "gtkmarshalers.h"
#include "gtkprivate.h"
#include "gtkintl.h"

#define DEFAULT_TRIGGER_DELAY 500

typedef struct _GtkGestureLongPressPrivate GtkGestureLongPressPrivate;

enum {
  PROP_BUTTON = 1
};

enum {
  PRESSED,
  CANCELLED,
  N_SIGNALS
};

struct _GtkGestureLongPressPrivate
{
  gdouble initial_x;
  gdouble initial_y;

  guint button;
  guint timeout_id;
  guint delay;
  guint cancelled : 1;
  guint triggered : 1;
};

static guint signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE (GtkGestureLongPress, gtk_gesture_long_press, GTK_TYPE_GESTURE)

static void
gtk_gesture_long_press_init (GtkGestureLongPress *gesture)
{
}

static gboolean
gtk_gesture_long_press_check (GtkGesture *gesture)
{
  GtkGestureLongPressPrivate *priv;

  priv = gtk_gesture_long_press_get_instance_private (GTK_GESTURE_LONG_PRESS (gesture));

  if (priv->cancelled)
    return FALSE;

  return gtk_gesture_is_active (gesture);
}

static gboolean
_gtk_gesture_long_press_timeout (gpointer user_data)
{
  GtkGestureLongPress *gesture = user_data;
  GtkGestureLongPressPrivate *priv;
  GdkEventSequence *sequence;
  gdouble x, y;

  priv = gtk_gesture_long_press_get_instance_private (gesture);
  sequence = gtk_gesture_get_last_updated_sequence (GTK_GESTURE (gesture));
  gtk_gesture_get_point (GTK_GESTURE (gesture), sequence, &x, &y);

  priv->timeout_id = 0;
  priv->triggered = TRUE;
  g_signal_emit (gesture, signals[PRESSED], 0, x, y);

  return FALSE;
}

static void
gtk_gesture_long_press_begin (GtkGesture       *gesture,
                              GdkEventSequence *sequence)
{
  GtkGestureLongPressPrivate *priv;
  GdkEvent *event;
  guint button;

  priv = gtk_gesture_long_press_get_instance_private (GTK_GESTURE_LONG_PRESS (gesture));
  event = gtk_gesture_get_last_event (gesture, sequence);

  if (event->type == GDK_BUTTON_PRESS)
    button = event->button.button;
  else if (event->type == GDK_TOUCH_BEGIN)
    button = 1;
  else
    return;

  if (priv->button != 0 && priv->button != button)
    return;

  gtk_gesture_get_point (gesture, sequence,
                         &priv->initial_x, &priv->initial_y);
  priv->timeout_id =
    gdk_threads_add_timeout (DEFAULT_TRIGGER_DELAY,
                             _gtk_gesture_long_press_timeout,
                             gesture);
}

static void
gtk_gesture_long_press_update (GtkGesture       *gesture,
                               GdkEventSequence *sequence)
{
  GtkGestureLongPressPrivate *priv;
  GtkWidget *widget;
  gdouble x, y;

  widget = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (gesture));
  priv = gtk_gesture_long_press_get_instance_private (GTK_GESTURE_LONG_PRESS (gesture));
  gtk_gesture_get_point (gesture, sequence, &x, &y);

  if (gtk_drag_check_threshold (widget, priv->initial_x, priv->initial_y, x, y))
    {
      if (priv->timeout_id)
        {
          g_signal_emit (gesture, signals[CANCELLED], 0);
          g_source_remove (priv->timeout_id);
          priv->timeout_id = 0;
        }

      priv->cancelled = TRUE;
    }
}

static void
gtk_gesture_long_press_end (GtkGesture       *gesture,
                            GdkEventSequence *sequence)
{
  GtkGestureLongPressPrivate *priv;

  priv = gtk_gesture_long_press_get_instance_private (GTK_GESTURE_LONG_PRESS (gesture));

  if (priv->timeout_id)
    {
      if (!priv->triggered)
        g_signal_emit (gesture, signals[CANCELLED], 0);
      g_source_remove (priv->timeout_id);
      priv->timeout_id = 0;
    }

  priv->cancelled = priv->triggered = FALSE;
}

static void
gtk_gesture_long_press_finalize (GObject *object)
{
  GtkGestureLongPressPrivate *priv;

  priv = gtk_gesture_long_press_get_instance_private (GTK_GESTURE_LONG_PRESS (object));

  if (priv->timeout_id)
    g_source_remove (priv->timeout_id);

  G_OBJECT_CLASS (gtk_gesture_long_press_parent_class)->finalize (object);
}

static void
gtk_gesture_long_press_set_property (GObject      *object,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  switch (prop_id)
    {
    case PROP_BUTTON:
      gtk_gesture_long_press_set_button (GTK_GESTURE_LONG_PRESS (object),
                                         g_value_get_uint (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gtk_gesture_long_press_get_property (GObject    *object,
                                     guint       prop_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  GtkGestureLongPressPrivate *priv;
  GtkGestureLongPress *gesture;

  gesture = GTK_GESTURE_LONG_PRESS (object);
  priv = gtk_gesture_long_press_get_instance_private (gesture);

  switch (prop_id)
    {
    case PROP_BUTTON:
      g_value_set_uint (value, priv->button);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gtk_gesture_long_press_class_init (GtkGestureLongPressClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkGestureClass *gesture_class = GTK_GESTURE_CLASS (klass);

  object_class->finalize = gtk_gesture_long_press_finalize;
  object_class->set_property = gtk_gesture_long_press_set_property;
  object_class->get_property = gtk_gesture_long_press_get_property;

  gesture_class->check = gtk_gesture_long_press_check;
  gesture_class->begin = gtk_gesture_long_press_begin;
  gesture_class->update = gtk_gesture_long_press_update;
  gesture_class->end = gtk_gesture_long_press_end;

  g_object_class_install_property (object_class,
                                   PROP_BUTTON,
                                   g_param_spec_uint ("button",
                                                      P_("Button number"),
                                                      P_("Button number to listen to"),
                                                      0, G_MAXUINT, 0,
                                                      GTK_PARAM_READWRITE));
  signals[PRESSED] =
    g_signal_new ("pressed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GtkGestureLongPressClass, pressed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 2, G_TYPE_DOUBLE, G_TYPE_DOUBLE);
  signals[CANCELLED] =
    g_signal_new ("cancelled",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GtkGestureLongPressClass, cancelled),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);
}

/**
 * gtk_gesture_long_press_new:
 * @widget: a #GtkWidget
 *
 * Returns a newly created #GtkGesture that recognizes long presses
 *
 * Returns: a newly created #GtkGestureLongPress
 *
 * Since: 3.14
 **/
GtkGesture *
gtk_gesture_long_press_new (GtkWidget *widget)
{
  return g_object_new (GTK_TYPE_GESTURE_LONG_PRESS,
                       "widget", widget,
                       NULL);
}

/**
 * gtk_gesture_long_press_set_button:
 * @gesture: a #GtkGestureLongPress
 * @button: button number to listen to, or 0 for any button
 *
 * Sets the button number @gesture listens to. If non-0, every
 * button press from a different button number will be ignored.
 * Touch events implicitly match with button 1.
 *
 * Since: 3.14
 **/
void
gtk_gesture_long_press_set_button (GtkGestureLongPress *gesture,
                                   guint                button)
{
  GtkGestureLongPressPrivate *priv;

  g_return_if_fail (GTK_IS_GESTURE_LONG_PRESS (gesture));

  priv = gtk_gesture_long_press_get_instance_private (gesture);

  if (priv->button == button)
    return;

  priv->button = button;
  g_object_notify (G_OBJECT (gesture), "button");
}

/**
 * gtk_gesture_long_press_get_button:
 * @gesture: a #GtkGestureLongPress
 *
 * Returns the button number @gesture listens for, or 0 if @gesture
 * reacts to any button press.
 *
 * Returns: The button number, or 0 for any button.
 *
 * Since: 3.14
 **/
guint
gtk_gesture_long_press_get_button (GtkGestureLongPress *gesture)
{
  GtkGestureLongPressPrivate *priv;

  g_return_val_if_fail (GTK_IS_GESTURE_LONG_PRESS (gesture), 0);

  priv = gtk_gesture_long_press_get_instance_private (gesture);

  return priv->button;
}