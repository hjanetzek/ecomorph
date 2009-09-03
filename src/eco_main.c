#include "e_mod_main.h"
#include <X11/Xlib.h>

static void  _e_mod_run_cb(void *data, E_Menu *m, E_Menu_Item *mi);
static void  _e_mod_menu_add(void *data, E_Menu *m);

Ecore_X_Atom ECOMORPH_ATOM_MANAGED = 0;
Ecore_X_Atom ECOMORPH_ATOM_MOVE_RESIZE = 0;
Ecore_X_Atom ECOMORPH_ATOM_PLUGIN = 0;

static E_Module *conf_module = NULL;
static E_Int_Menu_Augmentation *maug = NULL;

/*static char *eco_config_filename = NULL;
 * static Eet_File *eco_config_file = NULL; */
Eet_Data_Descriptor *eco_edd_group, *eco_edd_option;
/* Eet_Data_Descriptor_Class eddc_option;
 * Eet_Data_Descriptor_Class eddc_group; */

/* module setup */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
     "Ecomorph"
};

static Eina_Hash*
eet_eina_hash_add(Eina_Hash *hash, const char *key, const void *data)
{
  if (!hash) hash = eina_hash_string_superfast_new(NULL);
  if (!hash) return NULL;

  eina_hash_add(hash, key, data);
  return hash;
}
static int
_e_main_cb_after_restart(void *data)
{ 
   if(e_manager_current_get())
     ecore_x_client_message32_send
       (e_manager_current_get()->root,
	ECOMORPH_ATOM_MANAGED, SubstructureNotifyMask,
	ECOMORPH_EVENT_RESTART, 0, 0, 0, 0);
   return 0;  
}

EAPI void *
e_modapi_init(E_Module *m)
{
   char buf[PATH_MAX];

   /* Location of message catalogs for localization */
   snprintf(buf, sizeof(buf), "%s/locale", e_module_dir_get(m));
   bindtextdomain(PACKAGE, buf);
   bind_textdomain_codeset(PACKAGE, "UTF-8");

   /* Location of theme to load for this module */
   snprintf(buf, sizeof(buf), "%s/e-module-ecomorph.edj", e_module_dir_get(m));
   edje_file = eina_stringshare_add(buf);

   e_configure_registry_category_add("appearance", 10, _("Look"),
                                     NULL, "enlightenment/appearance");
   e_configure_registry_item_add("appearance/eco", 50, _("Ecomorph"),
                                 NULL, buf, e_int_config_eco);


   maug = e_int_menus_menu_augmentation_add("config/1", _e_mod_menu_add, NULL, NULL, NULL);
    
   eco_edd_group = eet_data_descriptor_new("group", sizeof(Eco_Group),
   				       NULL, NULL, NULL, NULL,
   				       (void  (*) (void *, int (*) (void *, const char *, void *, void *), void *))eina_hash_foreach,
   				       (void *(*) (void *, const char *, void *))eet_eina_hash_add,
   				       (void  (*) (void *))eina_hash_free);
   
   eco_edd_option = eet_data_descriptor_new("option", sizeof(Eco_Option),
   					(void *(*) (void *))eina_list_next,
   					(void *(*) (void *, void *)) eina_list_append,
   					(void *(*) (void *))eina_list_data_get,
   					(void *(*) (void *))eina_list_free,
   					NULL, NULL, NULL);

   /* EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc_option, Eco_Option);
    * EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc_group,  Eco_Group);
    * eco_edd_option = eet_data_descriptor_stream_new(&eddc_option);
    * eco_edd_group =  eet_data_descriptor_stream_new(&eddc_group); */
   
   EET_DATA_DESCRIPTOR_ADD_BASIC(eco_edd_option, Eco_Option, "type",	 type,		  EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(eco_edd_option, Eco_Option, "int",	 intValue,	  EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(eco_edd_option, Eco_Option, "double",	 doubleValue, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(eco_edd_option, Eco_Option, "string",	 stringValue, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_LIST (eco_edd_option, Eco_Option, "list",	 listValue,	  eco_edd_option);

   EET_DATA_DESCRIPTOR_ADD_HASH (eco_edd_group,  Eco_Group,  "options", data, eco_edd_option);


   ECOMORPH_ATOM_MANAGED = ecore_x_atom_get("__ECOMORPH_WINDOW_MANAGED");
   ECOMORPH_ATOM_PLUGIN  = ecore_x_atom_get("__ECOMORPH_PLUGIN");
   ECOMORPH_ATOM_MOVE_RESIZE  = ecore_x_atom_get("__ECOMORPH_MOVE_RESIZE");

   ecore_x_netwm_window_type_set
     (e_container_current_get(e_manager_current_get())->bg_win,
      ECORE_X_WINDOW_TYPE_DESKTOP);

   /* if (after_restart) */
   ecore_timer_add(1.0, _e_main_cb_after_restart, NULL);

   eco_actions_create();
   eco_event_init();
   eco_border_init();
   
   conf_module = m;
   e_module_delayed_set(m, 0);

   e_config->desk_flip_animate_mode = -1;
   
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   E_Config_Dialog *cfd;
   while ((cfd = e_config_dialog_get("E", "_config_eco_dialog"))) e_object_del(E_OBJECT(cfd));
   e_configure_registry_item_del("appearance/eco");
   e_configure_registry_category_del("appearance");

   /* remove module-supplied menu additions */
   if (maug)
     {
	e_int_menus_menu_augmentation_del("config/1", maug);
	maug = NULL;
     }
   eco_actions_free();
   eco_event_shutdown();
   eco_border_shutdown();

   if (restart)
     ecore_x_client_message32_send
       (e_manager_current_get()->root, ECOMORPH_ATOM_MANAGED,
	SubstructureNotifyMask,  ECOMORPH_EVENT_RESTART, 0, 1, 0, 0);   
   conf_module = NULL;
   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   return 1;
}


/* menu item callback(s) */
static void 
_e_mod_run_cb(void *data, E_Menu *m, E_Menu_Item *mi)
{
   e_int_config_eco(e_container_current_get(e_manager_current_get()), NULL);
}

/* menu item add hook */
static void
_e_mod_menu_add(void *data, E_Menu *m)
{
   E_Menu_Item *mi;
   
   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Ecomorph"));
   e_menu_item_icon_edje_set(mi, edje_file, "icon");
   e_menu_item_callback_set(mi, _e_mod_run_cb, NULL);
}

