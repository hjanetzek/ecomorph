#include <e.h>

#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#define D(x) //do { printf(__FILE__ ":%d:\t", __LINE__); printf x;
	     //fflush(stdout); } while(0)

#define _(str) dgettext(PACKAGE, str)

#define ECOMORPH_EVENT_MAPPED       0
#define ECOMORPH_EVENT_STATE        1
#define ECOMORPH_EVENT_DESK         2
#define ECOMORPH_EVENT_RESTART      3
#define ECOMORPH_EVENT_GRAB         4
#define ECOMORPH_EVENT_MOVE         5
#define ECOMORPH_EVENT_MOVE_RESIZE  6
#define ECOMORPH_EVENT_FOCUS        7
#define ECOMORPH_WINDOW_STATE_INVISIBLE 0
#define ECOMORPH_WINDOW_STATE_VISIBLE 1

#define ECO_PLUGIN_TERMINATE_NOTIFY 1

#include "eco_config.h"
#include "eco_actions.h"
#include "eco_event.h"

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);

extern Ecore_X_Atom ECOMORPH_ATOM_MANAGED;
extern Ecore_X_Atom ECOMORPH_ATOM_PLUGIN;
#endif
