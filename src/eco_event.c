#include "e_mod_main.h"
#include <X11/Xlib.h>


typedef struct _Eco_Border_Data Eco_Border_Data;
typedef struct _Eco_Icon Eco_Icon;

struct _Eco_Border_Data
{
  E_Border *border;
  Ecore_Event_Handler *damage_handler;
  Ecore_X_Damage damage;
  Ecore_X_Rectangle damage_rect;
  Ecore_Timer *damage_timeout;
};

struct _Eco_Icon
{
  Evas *evas;
  int  size;
  unsigned int *data;
  unsigned int *icon_ptr;
};

static int  _eco_cb_client_message(void *data, int ev_type, void *ev);
static int  _eco_cb_window_configure(void *data, int ev_type, void *ev);
static int  _eco_cb_zone_desk_count_set(void *data, int ev_type, void *ev);
static int  _eco_cb_border_icon_change(void *data, int ev_type, void *ev);
static int  _eco_cb_border_remove(void *data, int ev_type, void *ev);
static int  _eco_cb_border_show(void *data, int ev_type, void *ev);
static int  _eco_cb_border_desk_set(void *data, int ev_type, void *ev);
static int  _eco_cb_border_focus(void *data, int ev_type, void *ev);
static int  _eco_cb_desk_show(void *data, int ev_type, void *ev);

static void _eco_border_cb_hook_new_border(void *data, void *bd);
static void _eco_border_cb_hook_pre_new_border(void *data, void *bd);
static void _eco_border_cb_hook_post_new_border(void *data, void *bd);
static void _eco_border_cb_hook_pre_fetch(void *data, void *bd);
static void _eco_border_cb_hook_post_fetch(void *data, void *bd);
static void _eco_border_cb_hook_set_desk(void *data, void *bd);
static void _eco_border_cb_hook_grab(void *data, void *bd);
static void _eco_border_cb_hook_grab_begin(void *data, void *bd);
static void _eco_border_cb_hook_ungrab(void *data, void *bd);

static void _eco_message_send(Ecore_X_Window win, long l1, long l2, long l3, long l4, long l5);
static void _eco_message_root_send(Ecore_X_Atom atom, long l1, long l2, long l3, long l4, long l5);

static void _eco_zone_desk_count_set(E_Zone *zone);

static int  _eco_window_icon_init(void);
static void _eco_window_icon_shutdown(void);
static Evas_Object *_eco_border_icon_add(E_Border *bd, Evas *evas, int size);

static Eina_List *handlers = NULL;
static Eina_List *hooks = NULL;
static Eina_List *eco_borders = NULL;
static Eco_Icon *icon = NULL;
static int border_moveresize_active = 0;

static int _eco_border_changes_title = 0;
static int _eco_border_changes_name = 0;
static int _eco_border_changes_state = 0;
static int _eco_border_changes_type = 0;
static int _eco_border_update_state = 0;

static int
_cb_after_restart(void *data)
{
  _eco_message_root_send
    (ECOMORPH_ATOM_MANAGED, ECOMORPH_EVENT_RESTART, 0, 0, 0, 0);

  Eina_List *l;
  E_Border *bd;
      
  EINA_LIST_FOREACH(e_border_client_list(), l, bd)
    {
      _eco_message_send(bd->win, ECOMORPH_EVENT_DESK,
			0, bd->desk->x, bd->desk->y, 0);
    }
  
  return 0;
}


EAPI int
eco_event_init(void)
{
   Ecore_Event_Handler *h;
   E_Border_Hook *hook;
   
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE, _eco_cb_client_message, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_ZONE_DESK_COUNT_SET, _eco_cb_zone_desk_count_set, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_ICON_CHANGE, _eco_cb_border_icon_change, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_REMOVE, _eco_cb_border_remove, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_SHOW, _eco_cb_border_show, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_DESK_SET, _eco_cb_border_desk_set, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_DESK_SHOW, _eco_cb_desk_show, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_FOCUS_IN, _eco_cb_border_focus, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_FOCUS_OUT, _eco_cb_border_focus, NULL));
   
   hooks = eina_list_append(hooks, e_border_hook_add(E_BORDER_HOOK_NEW_BORDER, _eco_border_cb_hook_new_border, NULL));
   hooks = eina_list_append(hooks, e_border_hook_add(E_BORDER_HOOK_EVAL_PRE_NEW_BORDER, _eco_border_cb_hook_pre_new_border, NULL));
   hooks = eina_list_append(hooks, e_border_hook_add(E_BORDER_HOOK_EVAL_POST_NEW_BORDER, _eco_border_cb_hook_post_new_border, NULL));
   hooks = eina_list_append(hooks, e_border_hook_add(E_BORDER_HOOK_EVAL_PRE_FETCH, _eco_border_cb_hook_pre_fetch, NULL));
   hooks = eina_list_append(hooks, e_border_hook_add(E_BORDER_HOOK_EVAL_POST_FETCH, _eco_border_cb_hook_post_fetch, NULL));
   hooks = eina_list_append(hooks, e_border_hook_add(E_BORDER_HOOK_SET_DESK, _eco_border_cb_hook_set_desk, NULL));
   hooks = eina_list_append(hooks, e_border_hook_add(E_BORDER_HOOK_MOVE_BEGIN, _eco_border_cb_hook_grab_begin, NULL));
   hooks = eina_list_append(hooks, e_border_hook_add(E_BORDER_HOOK_MOVE_UPDATE, _eco_border_cb_hook_grab, NULL));
   hooks = eina_list_append(hooks, e_border_hook_add(E_BORDER_HOOK_MOVE_END, _eco_border_cb_hook_ungrab, NULL));
   hooks = eina_list_append(hooks, e_border_hook_add(E_BORDER_HOOK_RESIZE_BEGIN, _eco_border_cb_hook_grab_begin, NULL));
   hooks = eina_list_append(hooks, e_border_hook_add(E_BORDER_HOOK_RESIZE_UPDATE, _eco_border_cb_hook_grab, NULL));
   hooks = eina_list_append(hooks, e_border_hook_add(E_BORDER_HOOK_RESIZE_END, _eco_border_cb_hook_ungrab, NULL));

   _eco_window_icon_init();

   _eco_zone_desk_count_set
     (e_util_zone_current_get(e_manager_current_get()));

   ecore_timer_add(1.0, _cb_after_restart, NULL);
   
   return 1;
}

EAPI int
eco_event_shutdown(void)
{
   Ecore_Event_Handler *h;
   E_Border_Hook *hook;
   Eco_Border_Data *bdd;
   Eina_List *l;
   E_Border *bd;
   
   EINA_LIST_FREE (handlers, h)
     ecore_event_handler_del(h);
   
   EINA_LIST_FREE (hooks, hook)
     e_border_hook_del(hook);

   if (restart && evil)
     _eco_message_root_send(ECOMORPH_ATOM_MANAGED,
			    ECOMORPH_EVENT_RESTART,
			    0, 1, 0, 0);
   
   EINA_LIST_FOREACH(e_border_client_list(), l, bd)
     {
       bd->changed = 1;
       bd->changes.pos = 1;
       bd->fx.x = 0;
       bd->fx.y = 0;
		
       ecore_x_window_move(bd->win, bd->x, bd->y);
     }
   
   EINA_LIST_FREE (eco_borders, bdd)
     {
       if (bdd->damage)
	 ecore_x_damage_free(bdd->damage);

       if (bdd->damage_handler)
	 ecore_event_handler_del(bdd->damage_handler);

       e_object_unref(E_OBJECT(bdd->border));

       free(bdd);
     }

   return 1;
}

static void
_eco_message_send(Ecore_X_Window win, long l1, long l2, long l3, long l4, long l5)
{
  ecore_x_client_message32_send
    (win, ECOMORPH_ATOM_MANAGED, SubstructureNotifyMask, l1, l2, l3, l4, l5);
}

static void
_eco_message_root_send(Ecore_X_Atom atom, long l1, long l2, long l3, long l4, long l5)
{
  ecore_x_client_message32_send
    (e_manager_current_get()->root, atom, SubstructureNotifyMask, l1, l2, l3, l4, l5);
}


static int
_eco_cb_client_message(void *data, int ev_type, void *ev)
{
   Ecore_X_Event_Client_Message *e = ev;
   int type = e->data.l[0];
     
   if (e->message_type != ECOMORPH_ATOM_MANAGED)
     return 1;
     
   if (type < 100)
     return 1;

   /* printf("ecomp event %d -- %d %d %d %d\n", type, 
    * 	  e->data.l[1], e->data.l[2],
    * 	  e->data.l[3], e->data.l[4]); */
   
   switch(type)
     {
     case ECOMORPH_ECOMP_PLUGIN_END:
       eco_action_terminate();
       break;
	    
     case ECOMORPH_ECOMP_WINDOW_MOVE:
       {
	 E_Border* bd;
	 E_Desk *desk;
	 int x, y, dx ,dy;

	 if (!(bd = e_border_find_by_window(e->win)))
	   break;

	 x = e->data.l[1];
	 y = e->data.l[2];

	 dx = x / bd->zone->w;
	 dy = y / bd->zone->h;
	 
	 /* remove. here should always be a desk.. */
	 if (dx < 0) dx *= -1;
	 if (dy < 0) dy *= -1;

	 if ((dx != bd->desk->x) || (dy != bd->desk->y))
	   {
	     if ((desk = e_desk_at_xy_get(bd->zone, dx, dy)))
	       e_border_desk_set(bd, desk);
	   }

	 e_border_move(bd, MOD(x, bd->zone->w), MOD(y, bd->zone->h));
	 break;
       }
     case ECOMORPH_ECOMP_WINDOW_STACKING:
       {
	 Ecore_X_Window sibling = e->data.l[2];
	 int stacking = e->data.l[1];
	 E_Border* bd;
	 
	 if (!(bd = e_border_find_by_window(e->win)))
	   break;

	 if (bd->lock_client_stacking)
	   break;
	 
	 if (sibling)
	   {
	     E_Border *obd;

	     if (stacking == ECORE_X_WINDOW_STACK_ABOVE)
	       {
		 obd = e_border_find_by_window(sibling);
		 if (obd) e_border_stack_above(bd, obd);
	       }
	     else if (stacking == ECORE_X_WINDOW_STACK_BELOW)
	       {
		 obd = e_border_find_by_window(sibling);
		 if (obd) e_border_stack_below(bd, obd);
	       }
	   }
	 else
	   {
	     if (stacking == ECORE_X_WINDOW_STACK_ABOVE)
	       {
		 e_border_raise(bd);
	       }
	     else if (stacking == ECORE_X_WINDOW_STACK_BELOW)
	       {
		 e_border_lower(bd);
	       }

	   }
	 break;
       }
     case ECOMORPH_ECOMP_WINDOW_ACTIVATE:
       {	 
	 E_Border* bd;
	 
	 if (!(bd = e_border_find_by_window(e->win)))
	   break;

	 if (bd->desk != e_desk_current_get(bd->zone))
	   e_desk_show(bd->desk);

	 if (bd->shaded)
	   e_border_unshade(bd, E_DIRECTION_UP);
      
	 if (bd->iconic)
	   e_border_uniconify(bd);
      
	 e_border_raise(bd);
	 e_border_focus_set(bd, 1, 1);	    
	 break;
       }
     case ECOMORPH_ECOMP_VIEWPORT:
       {
	 E_Manager *man = e_manager_current_get();
	 E_Zone *zone;
	 int dx, dy;
	 
	 if (e->win != man->root)
	   break;

	 zone = e_util_zone_current_get(man); 
	 dx = e->data.l[1];
	 dy = e->data.l[2];
     
	 if (e_desk_current_get(zone) == e_desk_at_xy_get(zone, dx, dy))
	   break;
	 
	 e_zone_desk_flip_to(zone, dx, dy);
	 break;
       }
     default:	     
       break;
     }
   
   return 1;
}

static void
_eco_zone_desk_count_set(E_Zone *zone)
{
  int geom[2];
  geom[0] = zone->desk_x_count * zone->w;
  geom[1] = zone->desk_y_count * zone->h;

  _eco_message_root_send(ECORE_X_ATOM_NET_DESKTOP_GEOMETRY, geom[0], geom[1], 0, 0, 0); 

  ecore_x_window_prop_card32_set
    (e_manager_current_get()->root, ECORE_X_ATOM_NET_DESKTOP_GEOMETRY, geom, 2);  

}


static int
_eco_cb_zone_desk_count_set(void *data, int ev_type, void *ev)
{
   E_Event_Zone_Desk_Count_Set *e = ev;
   _eco_zone_desk_count_set(e->zone);
   return 1;
}

static void
_eco_desk_event_desk_after_show_free(void *data, void *event)
{
  E_Event_Desk_After_Show *ev;

  ev = event;
  e_object_unref(E_OBJECT(ev->desk));
  free(ev);
}

static int
_eco_cb_desk_show(void *data, int ev_type, void *event)
{
  E_Event_Desk_Show *ev;
  E_Border *bd, *bd2 = NULL;
  Eina_List *l;
  int geom[2];
  int move_type = 0;
  E_Desk *desk;
  E_Zone *zone;
  E_Border_List *bl;
  E_Event_Desk_After_Show *eev;

  ev = event;
  desk = ev->desk;
  zone = desk->zone;
   
  EINA_LIST_FOREACH(e_border_client_list(), l, bd)
    {
      if ((!bd->desk->visible) && (!bd->sticky))
	{
	  bd->visible = 0;
	  e_container_shape_hide(bd->shape);
	}
      else
	{
	  e_container_shape_move(bd->shape, bd->x, bd->y);
	  e_container_shape_show(bd->shape);
	}
      
      if (bd->moving) bd2 = bd;
    }

  if(bd2) printf ("move window 0x%x\n", (unsigned int) bd2->win);
  if(bd2) move_type = bd2->moving ? 1 : 2;
       
  /* trigger plugins for deskswitch animation */ 
  /* this message is automatically ignored when the viewport is
     already active. i.e. when expo changed the viewport and sent
     a message to change the desktop accordingly */

  _eco_message_root_send(ECORE_X_ATOM_NET_DESKTOP_VIEWPORT,
			 0, desk->x * zone->w, desk->y * zone->h, 
			 (bd2 ? bd2->win : 0), move_type);
  
  /* set property on root window so that ecomp get this information
   * after restarting  */
  geom[0] = desk->x * zone->w;
  geom[1] = desk->y * zone->h;

  ecore_x_window_prop_card32_set
    (e_manager_current_get()->root,
     ECORE_X_ATOM_NET_DESKTOP_VIEWPORT, geom, 2);  

  bl = e_container_border_list_first(zone->container);
  while ((bd = e_container_border_list_next(bl)))
    {
      if (bd->moving)
	{
	  e_border_desk_set(bd, desk);
	}
      else if (!bd->sticky)
	{
	  bd->fx.x = (bd->desk->x - zone->desk_x_current) * zone->w;
	  bd->fx.y = (bd->desk->y - zone->desk_y_current) * zone->h;
	  ecore_x_window_move(bd->win, bd->fx.x + bd->x, bd->fx.y + bd->y); 

	  ecore_x_icccm_move_resize_send(bd->client.win,
					 bd->x + bd->fx.x + bd->client_inset.l,
					 bd->y + bd->fx.y + bd->client_inset.t,
					 bd->client.w,
					 bd->client.h);
	}

      /* set visible again. as we had to fake hidden state to not
       unmap the window in desk_set border hook */
      if (!bd->visible && (bd->desk == desk) &&
	  !bd->iconic && !bd->shaded && !bd->changes.visible)
	e_border_show(bd);
    }
  e_container_border_list_free(bl);

  eev = E_NEW(E_Event_Desk_After_Show, 1);
  eev->desk = desk;
  e_object_ref(E_OBJECT(desk));
  ecore_event_add(E_EVENT_DESK_AFTER_SHOW, eev, 
		  _eco_desk_event_desk_after_show_free, NULL);

  return 1;
}


/************************************************************************/

static int
_eco_cb_border_focus(void *data, int ev_type, void *event)
{
  E_Event_Border_Desk_Set *ev = event;
  E_Border *bd = ev->border;

  if (ev_type == E_EVENT_BORDER_FOCUS_IN)
    {
      _eco_message_send(bd->win, ECOMORPH_EVENT_FOCUS, 0, 1, 0, 0);
    }
  else if (ev_type == E_EVENT_BORDER_FOCUS_OUT)
    {
      _eco_message_send(bd->win, ECOMORPH_EVENT_FOCUS, 0, 0, 0, 0);
    }

  return 1;
}


static void
_eco_border_cb_hook_grab_begin(void *data, void *border)
{
  E_Border *bd = border;
  
  if (border_moveresize_active)
    _eco_border_cb_hook_ungrab(NULL, bd);

  border_moveresize_active = 1;

  if (bd->desk != e_desk_current_get(bd->zone))
    e_border_desk_set(bd, e_desk_current_get(bd->zone));

}

static void
_eco_border_cb_hook_grab(void *data, void *border)
{
  E_Border *bd = border;
  
  if (border_moveresize_active == 1)
    {
      border_moveresize_active++;
      return;
    }

  if (border_moveresize_active == 2)
    {
      border_moveresize_active++;
	
      _eco_message_send(bd->win, ECOMORPH_EVENT_GRAB, 0, 1, 0, 0);
    }
}

static void
_eco_border_cb_hook_ungrab(void *data, void *border)
{
  E_Border *bd = border;
  
  border_moveresize_active = 0;

  _eco_message_send(bd->win, ECOMORPH_EVENT_GRAB, 0, 0, 0, 0);
}

static void
_eco_border_cb_hook_set_desk(void *data, void *border)
{
  E_Border *bd = border;
  
  if (!bd || !bd->desk) return;
   
  if (!(bd->desk->visible || bd->sticky))
    bd->visible = 0;
  
  ecore_x_netwm_desktop_set(bd->win, bd->desk->x + (bd->zone->desk_x_count * bd->desk->y));
}

static int
_eco_cb_border_desk_set(void *data, int ev_type, void *event)
{
  E_Event_Border_Desk_Set *ev = event;
  E_Border *bd = ev->border;
  E_Zone *zone = bd->zone;

  bd->fx.x = (bd->desk->x - bd->zone->desk_x_current) * bd->zone->w;
  bd->fx.y = (bd->desk->y - bd->zone->desk_y_current) * bd->zone->h;

  ecore_x_window_move(bd->win, bd->fx.x + bd->x, bd->fx.y + bd->y); 	

  ecore_x_icccm_move_resize_send(bd->client.win,
				 bd->x + bd->fx.x + bd->client_inset.l,
				 bd->y + bd->fx.y + bd->client_inset.t,
				 bd->client.w,
				 bd->client.h);
  
  _eco_message_send(bd->win, ECOMORPH_EVENT_DESK,
		    0, bd->desk->x, bd->desk->y, bd->moving);
   
  ecore_x_netwm_desktop_set(bd->win, bd->desk->x + (zone->desk_x_count * bd->desk->y));
	
  if ((!bd->desk->visible) && (!bd->sticky))
    e_container_shape_hide(bd->shape);
   
  return 1;
}

/* hacks for this issue http://lists.freedesktop.org/archives/xorg/2008-August/038022.html */
/* delay mapping until damage arrives + some delay for slow drawers */
static int
_eco_borderdamage_wait_time_out(void *data)
{
  E_Border *bd;
  Eco_Border_Data *bdd = data;
  E_OBJECT_TYPE_CHECK_RETURN(bdd->border, E_BORDER_TYPE, 0);

  bd = bdd->border;
   
  if(bdd->damage)
    {
      _eco_message_send(bd->win, ECOMORPH_EVENT_MAPPED,
			0, ECOMORPH_WINDOW_STATE_VISIBLE, 0, 0);
       
      ecore_x_damage_free(bdd->damage);
      bdd->damage = 0;
      bdd->damage_timeout = NULL;
      bdd->damage_handler = NULL;
    }
   
  return 0;
}

static int 
_eco_bordercb_damage_notify(void *data, int ev_type, void *ev)
{
  Ecore_X_Event_Damage *e;
  Eco_Border_Data *bdd;
  E_Border *bd;
  
  e = ev;
  bdd = data;
  bd = bdd->border;

  if(e->area.x < bdd->damage_rect.x) bdd->damage_rect.x = e->area.x;
  if(e->area.y < bdd->damage_rect.y) bdd->damage_rect.y = e->area.y;
  if(e->area.width + e->area.x > bdd->damage_rect.width)
    bdd->damage_rect.width = e->area.width + e->area.x;
  if(e->area.height + e->area.y > bdd->damage_rect.height)
    bdd->damage_rect.height = e->area.height + e->area.y;

  if((bdd->damage_rect.width  >= bd->w - (bd->client_inset.l + bd->client_inset.r)) &&
     (bdd->damage_rect.height >= bd->h - (bd->client_inset.t + bd->client_inset.b)))
    {
      ecore_event_handler_del(bdd->damage_handler);
      bdd->damage_handler = NULL;

      _eco_message_send(bd->win, ECOMORPH_EVENT_MAPPED,
			0, ECOMORPH_WINDOW_STATE_VISIBLE, 0, 0); 

      ecore_x_damage_free(bdd->damage);
	  
      bdd->damage = 0;
      if (bdd->damage_timeout)
	ecore_timer_del(bdd->damage_timeout);
      bdd->damage_timeout = NULL;
	      
      return 0;
    }
   
  return 1;
}

static void
_eco_borderwait_damage(E_Border *bd)
{
  Eco_Border_Data *bdd;

  LIST_DATA_BY_MEMBER_FIND(eco_borders, border, bd, bdd);
   
  if (!bdd)
    {
      bdd = calloc(1, sizeof(Eco_Border_Data));
      bdd->border = bd;
      LIST_PUSH(eco_borders, bdd);
      e_object_ref(E_OBJECT(bd));
    }

  if (bdd->damage) ecore_x_damage_free(bdd->damage);      
  if (bdd->damage_handler) ecore_event_handler_del(bdd->damage_handler);
  if (bdd->damage_timeout) ecore_timer_del(bdd->damage_timeout);
  bdd->damage = ecore_x_damage_new(bd->client.win, ECORE_X_DAMAGE_REPORT_RAW_RECTANGLES);
  bdd->damage_handler = ecore_event_handler_add
    (ECORE_X_EVENT_DAMAGE_NOTIFY, _eco_bordercb_damage_notify, bdd);
  bdd->damage_timeout = ecore_timer_add(1.0, _eco_borderdamage_wait_time_out, bdd);
   
  bdd->damage_rect.x = 0xffffff;
  bdd->damage_rect.y = 0xffffff;
  bdd->damage_rect.width = 0;
  bdd->damage_rect.height = 0;   
}

static int
_eco_cb_border_remove(void *data, int ev_type, void *ev)
{
  E_Event_Border_Remove *e = ev;
  E_Border *bd = e->border;
  Eco_Border_Data *bdd;

  LIST_DATA_BY_MEMBER_FIND(eco_borders, border, bd, bdd);
  if (!bdd) return 1;

  e_object_unref(E_OBJECT(bd));
  
  LIST_REMOVE_DATA(eco_borders, bdd);
  if (bdd->damage) ecore_x_damage_free(bdd->damage);
  if (bdd->damage_handler) ecore_event_handler_del(bdd->damage_handler);
  if (bdd->damage_timeout) ecore_timer_del(bdd->damage_timeout);
  free(bdd);

  /* XXX hack to disable e's own deskflip */
  e_config->desk_flip_animate_mode = -1;
  
  return 1;
}

static int
_eco_cb_border_show(void *data, int ev_type, void *ev)
{
  E_Event_Border_Show *e = ev;
  E_Border *bd = e->border;

  //  _eco_borderwait_damage(bd);
}

static void
_eco_border_cb_hook_new_border(void *data, void *border)
{
  E_Border *bd = border;
  E_Container *con = bd->zone->container;

  e_canvas_del(bd->bg_ecore_evas);
  ecore_evas_free(bd->bg_ecore_evas);
  ecore_x_window_free(bd->client.shell_win);
  e_bindings_mouse_ungrab(E_BINDING_CONTEXT_BORDER, bd->win);
  e_bindings_wheel_ungrab(E_BINDING_CONTEXT_BORDER, bd->win);
  ecore_x_window_free(bd->win);
  e_focus_setdown(bd);

  bd->client.argb = 1;
  
  if (bd->client.argb)
    bd->win = ecore_x_window_manager_argb_new(con->win, 0, 0, bd->w, bd->h);
  else
    {
      bd->win = ecore_x_window_override_new(con->win, 0, 0, bd->w, bd->h);
      ecore_x_window_shape_events_select(bd->win, 1);
    }

  ecore_x_window_prop_card32_set(bd->win, ECOMORPH_ATOM_MANAGED, &(bd->client.win), 1);
   
  e_bindings_mouse_grab(E_BINDING_CONTEXT_BORDER, bd->win);
  e_bindings_wheel_grab(E_BINDING_CONTEXT_BORDER, bd->win);
  e_focus_setup(bd);
   
  bd->bg_ecore_evas = e_canvas_new
    (e_config->evas_engine_borders, bd->win,
     0, 0, bd->w, bd->h, 1, 0, &(bd->bg_win));
   
  e_canvas_add(bd->bg_ecore_evas);
  bd->event_win = ecore_x_window_input_new(bd->win, 0, 0, bd->w, bd->h);
  bd->bg_evas = ecore_evas_get(bd->bg_ecore_evas);
  ecore_evas_name_class_set(bd->bg_ecore_evas, "E", "Frame_Window");
  ecore_evas_title_set(bd->bg_ecore_evas, "Enlightenment Frame");

  if (bd->client.argb)
    bd->client.shell_win = ecore_x_window_manager_argb_new(bd->win, 0, 0, 1, 1);
  else
    bd->client.shell_win = ecore_x_window_override_new(bd->win, 0, 0, 1, 1);
  ecore_x_window_container_manage(bd->client.shell_win);
  
  bd->x = MOD(bd->x, bd->zone->w);
  bd->y = MOD(bd->y, bd->zone->h);
}

#define MOD(a,b) ((a) < 0 ? ((b) - ((-(a) - 1) % (b))) - 1 : (a) % (b))

static void
_eco_border_cb_hook_pre_new_border(void *data, void *border)
{
  E_Border *bd = border;
  
  if (bd->new_client)
    {
      E_Zone *zone = bd->zone;

      bd->fx.x = (bd->desk->x - bd->zone->desk_x_current) * bd->zone->w;
      bd->fx.y = (bd->desk->y - bd->zone->desk_y_current) * bd->zone->h;
	
      bd->changes.pos = 1;
      bd->changed = 1;

      if (bd->client.netwm.state.hidden &&
	  !bd->client.netwm.state.shaded)
	{
	  bd->visible = 1;
	  bd->iconic = 0;
	  e_border_iconify(bd);
	}
    }
}

static void
_eco_border_cb_hook_post_new_border(void *data, void *border)
{
  E_Border *bd = border;
  E_Zone *zone = bd->zone;
  
  if (bd->new_client && bd->client.icccm.request_pos)
    {
      bd->x = MOD(bd->x, zone->w);
      bd->y = MOD(bd->y, zone->h);
    }

  if (bd->new_client)
    {
      if (!bd->iconic)
	e_border_show(bd);

      ecore_x_netwm_desktop_set
	(bd->win, bd->desk->x + (zone->desk_x_count * bd->desk->y));

      if (bd->client.netwm.type)
	ecore_x_netwm_window_type_set(bd->win, bd->client.netwm.type);
    }
}

static void
_eco_border_cb_hook_pre_fetch(void *data, void *border)
{
  E_Border *bd = border;
  
  if (bd->client.icccm.fetch.title)
    _eco_border_changes_title = 1;
  if (bd->client.netwm.fetch.name)
    _eco_border_changes_name = 1;
  if (bd->client.netwm.fetch.state)
    _eco_border_changes_state = 1;
  if (bd->client.netwm.fetch.type)
    _eco_border_changes_type = 1;
  if (bd->client.netwm.update.state)
    _eco_border_changes_state = 1;
}

static void
_eco_border_cb_hook_post_fetch(void *data, void *border)
{
  E_Border *bd = border;
  
  if (_eco_border_changes_title)
    {
      if(bd->client.icccm.title)
	ecore_x_netwm_name_set(bd->win, bd->client.icccm.title);
      _eco_border_changes_title = 0;

    }

  if (_eco_border_changes_name)
    {
      if(bd->client.netwm.name)
	ecore_x_netwm_name_set(bd->win, bd->client.netwm.name);
      _eco_border_changes_name = 0;
    }

  if (_eco_border_changes_type)
    {
      ecore_x_netwm_window_type_set(bd->win, bd->client.netwm.type);
      _eco_border_changes_type = 0;
    }

  if (_eco_border_changes_state)
    {
      Ecore_X_Window_State state[10];
      int num = 0;

      if (bd->client.netwm.state.modal)
	state[num++] = ECORE_X_WINDOW_STATE_MODAL;
      if (bd->client.netwm.state.sticky)
	state[num++] = ECORE_X_WINDOW_STATE_STICKY;
      if (bd->client.netwm.state.maximized_v)
	state[num++] = ECORE_X_WINDOW_STATE_MAXIMIZED_VERT;
      if (bd->client.netwm.state.maximized_h)
	state[num++] = ECORE_X_WINDOW_STATE_MAXIMIZED_HORZ;
      if (bd->client.netwm.state.shaded)
	state[num++] = ECORE_X_WINDOW_STATE_SHADED;
      if (bd->client.netwm.state.skip_taskbar)
	state[num++] = ECORE_X_WINDOW_STATE_SKIP_TASKBAR;
      if (bd->client.netwm.state.skip_pager)
	state[num++] = ECORE_X_WINDOW_STATE_SKIP_PAGER;
      if (bd->client.netwm.state.hidden)
	state[num++] = ECORE_X_WINDOW_STATE_HIDDEN;
      if (bd->client.netwm.state.fullscreen)
	state[num++] = ECORE_X_WINDOW_STATE_FULLSCREEN;

      switch (bd->client.netwm.state.stacking)
	{
	case E_STACKING_ABOVE:
	  state[num++] = ECORE_X_WINDOW_STATE_ABOVE;
	  break;
	case E_STACKING_BELOW:
	  state[num++] = ECORE_X_WINDOW_STATE_BELOW;
	  break;
	case E_STACKING_NONE:
	default:
	  break;
	}
      ecore_x_netwm_window_state_set(bd->win, state, num);	
      _eco_border_changes_state = 0;
    }
}



/*********************************************************************/

static int
_eco_window_icon_init(void)
{
   Evas_Engine_Info_Buffer *einfo;
   int rmethod;

   icon = calloc(sizeof(Eco_Icon),1);
   icon->size = 64;
  
   icon->evas = evas_new();
   if (!icon->evas)
     {
	return 0;
     }
   rmethod = evas_render_method_lookup("buffer");
   evas_output_method_set(icon->evas, rmethod);
   evas_output_size_set(icon->evas, icon->size, icon->size);
   evas_output_viewport_set(icon->evas, 0, 0, icon->size, icon->size);

   icon->data = malloc((2 + icon->size * icon->size) * sizeof(int));
   if (!icon->data)
     {
	evas_free(icon->evas);
	icon->evas = NULL;
	return 0;
     }

   icon->data[0] = 64;
   icon->data[1] = 64;
   icon->icon_ptr = icon->data + 2;
  
   einfo = (Evas_Engine_Info_Buffer *)evas_engine_info_get(icon->evas);
   if (!einfo)
     {
	free(icon->data);
	icon->data = NULL;
	evas_free(icon->evas);
	icon->evas = NULL;
	return 0;
     }
   einfo->info.depth_type = EVAS_ENGINE_BUFFER_DEPTH_ARGB32;
   einfo->info.dest_buffer = icon->icon_ptr; //icon->pixels;
   einfo->info.dest_buffer_row_bytes = icon->size * sizeof(int);
   einfo->info.use_color_key = 0;
   einfo->info.alpha_threshold = 0;
   einfo->info.func.new_update_region = NULL;
   einfo->info.func.free_update_region = NULL;
   evas_engine_info_set(icon->evas, (Evas_Engine_Info *)einfo);
}

static void
_eco_window_icon_shutdown(void)
{
   if (icon)
     {
	if (icon->evas) evas_free(icon->evas);
	if (icon->data) free(icon->data);
	free(icon);
     }
}

static Evas_Object *
_eco_border_icon_add(E_Border *bd, Evas *evas, int size)
{
   Evas_Object *o;
 
   E_OBJECT_CHECK_RETURN(bd, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(bd, E_BORDER_TYPE, NULL);

   o = NULL;
   if (bd->internal)
     {
	o = edje_object_add(evas);
	if (!bd->internal_icon) 
	  e_util_edje_icon_set(o, "enlightenment/e");
	else
	  {
	     if (!bd->internal_icon_key)
	       {
		  char *ext;
		  ext = strrchr(bd->internal_icon, '.');
		  if ((ext) && ((!strcmp(ext, ".edj"))))
		    {
		       if (!edje_object_file_set(o, bd->internal_icon, "icon"))
			 e_util_edje_icon_set(o, "enlightenment/e");	       
		    }
		  else if (ext)
		    {
		       evas_object_del(o);
		       o = e_icon_add(evas);
		       e_icon_file_set(o, bd->internal_icon);
		    }
		  else 
		    {
		       if (!e_util_edje_icon_set(o, bd->internal_icon))
			 e_util_edje_icon_set(o, "enlightenment/e"); 
		    }
	       }
	     else
	       {
		  edje_object_file_set(o, bd->internal_icon,
				       bd->internal_icon_key);
	       }
	  }
     }

   if (!o)
     {
	if ((bd->desktop) && (bd->icon_preference != E_ICON_PREF_NETWM))
	  {
	     o = e_util_desktop_icon_add(bd->desktop, 256, evas);
	  }
	else if (bd->client.netwm.icons)
	  {
	     int i, diff, tmp, found = 0;
	     o = e_icon_add(evas);

	     diff = abs(bd->client.netwm.icons[0].width - size);

	     for (i = 1; i < bd->client.netwm.num_icons; i++)
	       {
		  if ((tmp = abs(bd->client.netwm.icons[i].width - size)) < diff)
		    {
		       diff = tmp;
		       found = i;
		    }
	       }
	     
	     e_icon_data_set(o, bd->client.netwm.icons[found].data,
			     bd->client.netwm.icons[found].width,
			     bd->client.netwm.icons[found].height);
	     e_icon_alpha_set(o, 1);
	  }
     }

   if (!o)
     {
	o = edje_object_add(evas);
	e_util_edje_icon_set(o, "enlightenment/unknown");
     }

   //Evas_Object *o = _eco_border_icon_add(e->border, icon->evas, 64);
   evas_object_move(o, 0, 0);
   evas_object_resize(o, icon->size, icon->size);
   evas_object_show(o);
  
   Eina_List *updates = evas_render_updates(icon->evas);

   if (updates)
     {
	ecore_x_window_prop_card32_set(bd->win, 
				       ECORE_X_ATOM_NET_WM_ICON, 
				       icon->data, 
				       icon->size * icon->size + 2);
      
	evas_render_updates_free(updates);
     }
 
   evas_object_del(o);
}

static 
int _eco_cb_border_icon_change(void *data, int ev_type, void *ev)
{
   E_Event_Border_Icon_Change *e = ev;

   if (!icon) return 0;
  
   _eco_border_icon_add(e->border, icon->evas, 128);
   
   return 1;
}
