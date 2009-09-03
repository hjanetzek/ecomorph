#include "e_mod_main.h"


/* Apply Function */
static void
_apply(E_Config_Dialog_Data *cfdata)
{
   eco_config_group_apply("cubereflex");  
}

/* Main creation function */
EAPI void
eco_config_cubereflex(void *data)
{
   ECO_PAGE_BEGIN("cubereflex");
   ECO_PAGE_TABLE("Options");

   ECO_CREATE_SLIDER_DOUBLE(0, ground_size, "Reflection ground size", 0.0, 1.0, "%1.11f", 0, 0);
   ECO_CREATE_SLIDER_DOUBLE(0, intensity, "Reflection intensity", 0.0, 1.0, "%1.11f", 0, 1);
   ECO_CREATE_CHECKBOX(0, auto_zoom, "Zoom out to make the cube fit to the screen", 0, 2);
   ECO_CREATE_CHECKBOX(0, zoom_manual_only, "Auto zoom only on Mouse Rotate", 0, 3);

   ECO_PAGE_TABLE_END;
   ECO_PAGE_END;
}
