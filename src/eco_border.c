#include <X11/Xlib.h>
#include <X11/extensions/shape.h>
#include "e_mod_main.h"

#define LIST_DATA_BY_MEMBER_FIND(_list, _type, _member, _data, _result)	\
  {									\
     _type *data = NULL;						\
     Eina_List *l;							\
     EINA_LIST_FOREACH(_list, l, data)					\
       if (data && data->_member == _data)				\
	 break;								\
     _result = data;							\
  }

typedef struct _Eco_Border_Data Eco_Border_Data;

struct _Eco_Border_Data
{
  E_Border *border;
  Ecore_Event_Handler *damage_handler;
  Ecore_X_Damage damage;
  Ecore_X_Rectangle damage_rect;
  Ecore_Timer *damage_timeout;
};

static void _eco_border_cb_hook_new_border(void *data, E_Border *bd);
static void _eco_border_cb_hook_pre_new_border(void *data, E_Border *bd);
static void _eco_border_cb_hook_post_new_border(void *data, E_Border *bd);
static void _eco_border_cb_hook_pre_fetch(void *data, E_Border *bd);
static void _eco_border_cb_hook_post_fetch(void *data, E_Border *bd);
static void _eco_border_cb_hook_set_desk(void *data, E_Border *bd);
static void _eco_border_cb_hook_grab(void *data, E_Border *bd);
static void _eco_border_cb_hook_grab_begin(void *data, E_Border *bd);
static void _eco_border_cb_hook_ungrab(void *data, E_Border *bd);

static int _eco_cb_border_remove(void *data, int ev_type, void *ev);
static int _eco_cb_border_show(void *data, int ev_type, void *ev);
static int _eco_cb_border_move_resize(void *data, int ev_type, void *ev);
static int _eco_cb_border_desk_set(void *data, int ev_type, void *ev);
static int _eco_cb_border_focus(void *data, int ev_type, void *ev);
static int _eco_cb_desk_show(void *data, int ev_type, void *ev);

static Eina_List *eco_handlers = NULL;
static Eina_List *eco_border_hooks = NULL;
static Eina_List *eco_borders = NULL;
static Ecore_Timer *border_wait_timer = NULL;
static int border_moveresize_active = 0;

EAPI
int eco_border_init(void)
{
   Ecore_Event_Handler *h;
   E_Border_Hook *hook;
   
   h = ecore_event_handler_add(E_EVENT_BORDER_REMOVE, _eco_cb_border_remove, NULL);
   if (h) eco_handlers = eina_list_append(eco_handlers, h);
   h = ecore_event_handler_add(E_EVENT_BORDER_SHOW, _eco_cb_border_show, NULL);
   if (h) eco_handlers = eina_list_append(eco_handlers, h);
   h = ecore_event_handler_add(E_EVENT_BORDER_MOVE, _eco_cb_border_move_resize, NULL);
   if (h) eco_handlers = eina_list_append(eco_handlers, h);
   h = ecore_event_handler_add(E_EVENT_BORDER_RESIZE, _eco_cb_border_move_resize, NULL);
   if (h) eco_handlers = eina_list_append(eco_handlers, h);
   h = ecore_event_handler_add(E_EVENT_BORDER_DESK_SET, _eco_cb_border_desk_set, NULL);
   if (h) eco_handlers = eina_list_append(eco_handlers, h);
   h = ecore_event_handler_add(E_EVENT_DESK_SHOW, _eco_cb_desk_show, NULL);
   if (h) eco_handlers = eina_list_append(eco_handlers, h);
   h = ecore_event_handler_add(E_EVENT_BORDER_FOCUS_IN, _eco_cb_border_focus, NULL);
   if (h) eco_handlers = eina_list_append(eco_handlers, h);
   h = ecore_event_handler_add(E_EVENT_BORDER_FOCUS_OUT, _eco_cb_border_focus, NULL);
   if (h) eco_handlers = eina_list_append(eco_handlers, h);
   
   hook = e_border_hook_add(E_BORDER_HOOK_NEW_BORDER, _eco_border_cb_hook_new_border, NULL);
   if (hook) eco_border_hooks = eina_list_append(eco_border_hooks, hook);
   hook = e_border_hook_add(E_BORDER_HOOK_EVAL_PRE_NEW_BORDER, _eco_border_cb_hook_pre_new_border, NULL);
   if (hook) eco_border_hooks = eina_list_append(eco_border_hooks, hook);
   hook = e_border_hook_add(E_BORDER_HOOK_EVAL_POST_NEW_BORDER, _eco_border_cb_hook_post_new_border, NULL);
   if (hook) eco_border_hooks = eina_list_append(eco_border_hooks, hook);
   hook = e_border_hook_add(E_BORDER_HOOK_EVAL_PRE_FETCH, _eco_border_cb_hook_pre_fetch, NULL);
   if (hook) eco_border_hooks = eina_list_append(eco_border_hooks, hook);
   hook = e_border_hook_add(E_BORDER_HOOK_EVAL_POST_FETCH, _eco_border_cb_hook_post_fetch, NULL);
   if (hook) eco_border_hooks = eina_list_append(eco_border_hooks, hook);
   hook = e_border_hook_add(E_BORDER_HOOK_SET_DESK, _eco_border_cb_hook_set_desk, NULL);
   if (hook) eco_border_hooks = eina_list_append(eco_border_hooks, hook);
   hook = e_border_hook_add(E_BORDER_HOOK_MOVE_BEGIN, _eco_border_cb_hook_grab_begin, NULL);
   if (hook) eco_border_hooks = eina_list_append(eco_border_hooks, hook);
   hook = e_border_hook_add(E_BORDER_HOOK_MOVE_UPDATE, _eco_border_cb_hook_grab, NULL);
   if (hook) eco_border_hooks = eina_list_append(eco_border_hooks, hook);
   hook = e_border_hook_add(E_BORDER_HOOK_MOVE_END, _eco_border_cb_hook_ungrab, NULL);
   if (hook) eco_border_hooks = eina_list_append(eco_border_hooks, hook);
   hook = e_border_hook_add(E_BORDER_HOOK_RESIZE_BEGIN, _eco_border_cb_hook_grab_begin, NULL);
   if (hook) eco_border_hooks = eina_list_append(eco_border_hooks, hook);
   hook = e_border_hook_add(E_BORDER_HOOK_RESIZE_UPDATE, _eco_border_cb_hook_grab, NULL);
   if (hook) eco_border_hooks = eina_list_append(eco_border_hooks, hook);
   hook = e_border_hook_add(E_BORDER_HOOK_RESIZE_END, _eco_border_cb_hook_ungrab, NULL);
   if (hook) eco_border_hooks = eina_list_append(eco_border_hooks, hook);
}

static int
_eco_cb_border_focus(void *data, int ev_type, void *event)
{
   E_Event_Border_Desk_Set *ev = event;
   E_Border *bd = ev->border;

   if (ev_type == E_EVENT_BORDER_FOCUS_IN)
     {
	ecore_x_client_message32_send
	  (bd->win, ECORE_X_ATOM_NET_ACTIVE_WINDOW,
	   SubstructureNotifyMask,
	   1, 0, 0, 0, 0);
     }
   else if (ev_type == E_EVENT_BORDER_FOCUS_OUT)
     {
	ecore_x_client_message32_send
	  (bd->win, ECORE_X_ATOM_NET_ACTIVE_WINDOW,
	   SubstructureNotifyMask,
	   2, 0, 0, 0, 0);
     }

   return 1;
}


static void
_eco_border_cb_hook_grab_begin(void *data, E_Border *bd)
{
   if (border_moveresize_active)
     _eco_border_cb_hook_ungrab(NULL, bd);

   border_moveresize_active = 1;
}

static void
_eco_border_cb_hook_grab(void *data, E_Border *bd)
{
   if (border_moveresize_active == 1)
     {
	border_moveresize_active++;
	return;
     }

   if (border_moveresize_active == 2)
     {
	border_moveresize_active++;
	
	ecore_x_client_message32_send
	  (bd->win, ECOMORPH_ATOM_MANAGED,
	   SubstructureNotifyMask,
	   ECOMORPH_EVENT_GRAB, 0, 1, 0, 0);
     }
}

static void
_eco_border_cb_hook_ungrab(void *data, E_Border *bd)
{
   border_moveresize_active = 0;

   ecore_x_client_message32_send
     (bd->win, ECOMORPH_ATOM_MANAGED,
      SubstructureNotifyMask,
      ECOMORPH_EVENT_GRAB, 0, 0, 0, 0);
}

static void
_eco_border_cb_hook_set_desk(void *data, E_Border *bd)
{
   if (!bd || !bd->desk) return;
   
   if (!(bd->desk->visible) || (bd->sticky))
     bd->visible = 0;
}

EAPI int
eco_border_shutdown(void)
{
   Ecore_Event_Handler *h;
   E_Border_Hook *hook;
   Eco_Border_Data *bdd;
   
   EINA_LIST_FREE (eco_handlers, h)
     {
       if (h) ecore_event_handler_del(h);
     }

   EINA_LIST_FREE (eco_border_hooks, hook)
     {
       if (hook) e_border_hook_del(hook);
     }

   EINA_LIST_FREE (eco_borders, bdd)
     {
       if (bdd->damage) ecore_x_damage_free(bdd->damage);
       if (bdd->damage_handler) ecore_event_handler_del(bdd->damage_handler);
       free(bdd);
     }
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
   E_Border_List *bl;
   
   ev = event;
   desk = ev->desk;
   
   EINA_LIST_FOREACH(e_border_client_list(), l, bd)
     {
	if ((!bd->desk->visible) && (!bd->sticky))
	  e_container_shape_hide(bd->shape);
	   
	/* misuse of button_grab flag in e_actions..
	 * means 'move window by' */
	/* ((e_config->always_click_to_raise ||
	 *   e_config->always_click_to_focus ||
	 *   (e_config->focus_policy == E_FOCUS_CLICK)) ? 
	 *  !bd->button_grabbed : bd->button_grabbed) || */
	   
	if (bd->moving) bd2 = bd;
     }


   if(bd2) printf ("move window 0x%x\n", (unsigned int) bd2->win);
   if(bd2) move_type = bd2->moving ? 1 : 2;
       
   /* trigger plugins for deskswitch animation */ 
   /* this message is automatically ignored when the viewport is
      already active. i.e. when expo changed the viewport and sent
      a message to change the desktop accordingly */
   ecore_x_client_message32_send
     (e_manager_current_get()->root, ECORE_X_ATOM_NET_DESKTOP_VIEWPORT,
      SubstructureNotifyMask, 0,
      desk->x * desk->zone->w,
      desk->y * desk->zone->h, 
      (bd2 ? bd2->win : 0), move_type);

   /* set property on root window so that ecomp get this information
    * after restarting  */
   geom[0] = desk->x * desk->zone->w;
   geom[1] = desk->y * desk->zone->h;

   ecore_x_window_prop_card32_set(e_manager_current_get()->root,
				  ECORE_X_ATOM_NET_DESKTOP_VIEWPORT,
				  geom, 2);  

   bl = e_container_border_list_first(desk->zone->container);
   while ((bd = e_container_border_list_next(bl)))
     {
	if (bd->moving)
	  e_border_desk_set(bd, desk);
	else if (!bd->sticky)
	  {
	     bd->fx.x = (bd->desk->x - bd->zone->desk_x_current) * bd->zone->w;
	     bd->fx.y = (bd->desk->y - bd->zone->desk_y_current) * bd->zone->h;
	     ecore_x_window_move(bd->win, bd->fx.x + bd->x, bd->fx.y + bd->y); 
	  }
     }
   e_container_border_list_free(bl);

   return 1;
}

static int
_eco_cb_border_desk_set(void *data, int ev_type, void *event)
{
   E_Event_Border_Desk_Set *ev = event;
   E_Border *bd = ev->border;
   E_Zone *zone = bd->zone;
   
   /* e_border_show(bd); */

   bd->fx.x = (bd->desk->x - zone->desk_x_current) * zone->w; 
   bd->fx.y = (bd->desk->y - zone->desk_y_current) * zone->h;

   ecore_x_window_move(bd->win, bd->fx.x + bd->x, bd->fx.y + bd->y); 	
	
   ecore_x_client_message32_send
     (bd->win, ECOMORPH_ATOM_MANAGED,SubstructureNotifyMask,
      ECOMORPH_EVENT_DESK, 0, bd->desk->x, bd->desk->y, bd->moving);

   ecore_x_netwm_desktop_set(bd->win, bd->desk->x + (zone->desk_x_count * bd->desk->y));
	
   if ((!bd->desk->visible) && (!bd->sticky))
     e_container_shape_hide(bd->shape);
   
   return 1;
}

static int
_eco_border_move_resize(int ev_type, E_Border *bd)
{
   ecore_x_client_message32_send
     (bd->win, ECOMORPH_ATOM_MANAGED,
      SubstructureNotifyMask,
      ev_type, bd->fx.x + bd->x, bd->fx.y + bd->y, bd->w, bd->h);

   return 1;
}

static int
_eco_border_wait_cb(void *data)
{
   E_Border *bd = data;
   
   _eco_border_move_resize(ECOMORPH_EVENT_MOVE_RESIZE, bd);
   e_object_unref(E_OBJECT(bd));

   border_wait_timer = NULL;
   return 0;
}

static int
_eco_cb_border_move_resize(void *data, int ev_type, void *event)
{
   if (ev_type == E_EVENT_BORDER_MOVE)
     {
	E_Event_Border_Move *ev = event;
	_eco_border_move_resize(ECOMORPH_EVENT_MOVE, ev->border);
	
     }
   else if (ev_type == E_EVENT_BORDER_RESIZE)
     {
	E_Event_Border_Resize *ev = event;
	_eco_border_move_resize(ECOMORPH_EVENT_MOVE_RESIZE, ev->border);
	if (/* (!border_wait_timer) && */
	    ((ev->border->internal) ||
	     (ev->border->shading) ||
	     (ev->border->changes.shading)))
	  {
	     e_object_ref(E_OBJECT(ev->border));
	     border_wait_timer = ecore_timer_add(0.05, _eco_border_wait_cb, ev->border);
	  }
     }
   
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
	_eco_border_move_resize(E_EVENT_BORDER_RESIZE, bd);
	
	ecore_x_client_message32_send
	  (bd->win, ECOMORPH_ATOM_MANAGED,
	   SubstructureNotifyMask,
	   ECOMORPH_EVENT_MAPPED, 0,
	   ECOMORPH_WINDOW_STATE_VISIBLE, 0, 0);

	_eco_border_move_resize(E_EVENT_BORDER_RESIZE, bd);
	
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

      /* XXX not sure if this triggers a bug in ecomp or
	 if xul is just weird */
      if (bd->client.icccm.transient_for &&
      	  (bd->client.icccm.class) &&
      	  (!bd->remember || !bd->remember->prop.w) &&
      	  (!strcmp(bd->client.icccm.class, "Firefox")))
      	{
      	   _eco_border_move_resize(E_EVENT_BORDER_RESIZE, bd);
      	   
      	   if (bdd->damage_timeout)
      	     ecore_timer_del(bdd->damage_timeout);
      	   
      	   bdd->damage_timeout = ecore_timer_add
      	     (0.1, _eco_borderdamage_wait_time_out, bdd);
      
      	   return 0;
      	}
      
      ecore_x_client_message32_send
	(bd->win, ECOMORPH_ATOM_MANAGED,
	 SubstructureNotifyMask,
	 ECOMORPH_EVENT_MAPPED, 0,
	 ECOMORPH_WINDOW_STATE_VISIBLE, 0, 0);

      _eco_border_move_resize(E_EVENT_BORDER_RESIZE, bd);
      
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

   LIST_DATA_BY_MEMBER_FIND(eco_borders, Eco_Border_Data, border, bd, bdd);
   
   if (!bdd)
     {
	bdd = calloc(1, sizeof(Eco_Border_Data));
	bdd->border = bd;
	eco_borders = eina_list_append(eco_borders, bdd);
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


/* XXX this is not called in the case that e is 'stopping' so cleanup
   on shutdown. move border members to own struct */
static int
_eco_cb_border_remove(void *data, int ev_type, void *ev)
{
   E_Event_Border_Remove *e = ev;
   E_Border *bd = e->border;
   Eco_Border_Data *bdd;

   LIST_DATA_BY_MEMBER_FIND(eco_borders, Eco_Border_Data, border, bd, bdd);
   if (!bdd) return 1;

   eco_borders = eina_list_remove(eco_borders, bdd);
   if (bdd->damage) ecore_x_damage_free(bdd->damage);
   if (bdd->damage_handler) ecore_event_handler_del(bdd->damage_handler);
   if (bdd->damage_timeout) ecore_timer_del(bdd->damage_timeout);
   free(bdd);

   /* HACK disable e's deskflip animation... */
   e_config->desk_flip_animate_mode = -1;
   return 1;
}

static int
_eco_cb_border_show(void *data, int ev_type, void *ev)
{
   E_Event_Border_Show *e = ev;
   E_Border *bd = e->border;

   _eco_borderwait_damage(bd);
}

#define MOD(a,b) ((a) < 0 ? ((b) - ((-(a) - 1) % (b))) - 1 : (a) % (b))

static void
_eco_border_cb_hook_new_border(void *data, E_Border *bd)
{
   E_Container *con = bd->zone->container;

   e_canvas_del(bd->bg_ecore_evas);
   ecore_evas_free(bd->bg_ecore_evas);
   ecore_x_window_free(bd->client.shell_win);
   e_bindings_mouse_ungrab(E_BINDING_CONTEXT_BORDER, bd->win);
   e_bindings_wheel_ungrab(E_BINDING_CONTEXT_BORDER, bd->win);
   ecore_x_window_free(bd->win);
   
   bd->win = ecore_x_window_manager_argb_new(con->win, 0, 0, bd->w, bd->h);
   ecore_x_window_prop_card32_set(bd->win, ECOMORPH_ATOM_MANAGED, &(bd->client.win), 1);
   
   e_bindings_mouse_grab(E_BINDING_CONTEXT_BORDER, bd->win);
   e_bindings_wheel_grab(E_BINDING_CONTEXT_BORDER, bd->win);
   e_focus_setup(bd);
   
   bd->bg_ecore_evas = e_canvas_new(e_config->evas_engine_borders, bd->win,
   				    0, 0, bd->w, bd->h, 1, 0,
   				    &(bd->bg_win));
   e_canvas_add(bd->bg_ecore_evas);
   bd->event_win = ecore_x_window_input_new(bd->win, 0, 0, bd->w, bd->h);
   bd->bg_evas = ecore_evas_get(bd->bg_ecore_evas);
   ecore_evas_name_class_set(bd->bg_ecore_evas, "E", "Frame_Window");
   ecore_evas_title_set(bd->bg_ecore_evas, "Enlightenment Frame");

   bd->client.shell_win = ecore_x_window_manager_argb_new(bd->win, 0, 0, 1, 1);
   ecore_x_window_container_manage(bd->client.shell_win);
   
   bd->x = MOD(bd->x, bd->zone->w);
   bd->y = MOD(bd->y, bd->zone->h);
}

#define MOD(a,b) ((a) < 0 ? ((b) - ((-(a) - 1) % (b))) - 1 : (a) % (b))

static void
_eco_border_cb_hook_pre_new_border(void *data, E_Border *bd)
{
   if (bd->new_client)
     {
	E_Zone *zone = bd->zone;
	
	/* the border was moved according to its viewport */
	bd->fx.x = (bd->desk->x - zone->desk_x_current) * zone->w; 
	bd->fx.y = (bd->desk->y - zone->desk_y_current) * zone->h;
	
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
   if (bd->visible)
     _eco_border_move_resize(ECOMORPH_EVENT_MOVE_RESIZE, bd);
}

static void
_eco_border_cb_hook_post_new_border(void *data, E_Border *bd)
{
  if (bd->new_client && bd->client.icccm.request_pos)
    {
      bd->x = MOD(bd->x, bd->zone->w);
      bd->y = MOD(bd->y, bd->zone->h);
    }

  if (bd->new_client)
    {
       if (!bd->iconic)
	 e_border_show(bd);

       ecore_x_netwm_desktop_set(bd->win, bd->desk->x +
				 (bd->desk->zone->desk_x_count * bd->desk->y));
       _eco_border_move_resize(ECOMORPH_EVENT_MOVE_RESIZE, bd);
    }
}

static int _eco_border_changes_title = 0;
static int _eco_border_changes_name = 0;
static int _eco_border_changes_state = 0;
static int _eco_border_changes_type = 0;
static int _eco_border_update_state = 0;

static void
_eco_border_cb_hook_pre_fetch(void *data, E_Border *bd)
{

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
_eco_border_cb_hook_post_fetch(void *data, E_Border *bd)
{
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
