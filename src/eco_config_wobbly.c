#include "e_mod_main.h"


/* Apply Function */
static void
_apply(E_Config_Dialog_Data *cfdata)
{
  eco_config_group_apply("wobbly"); 
}

/* Main creation function */
EAPI void
eco_config_wobbly(void *data)
{
  ECO_PAGE_BEGIN("wobbly");
  ECO_PAGE_TABLE("Options");
  ECO_CREATE_SLIDER_DOUBLE(0, friction, "Spring friction", 0.1, 10.0, "%1.1f", 0, 0);
  ECO_CREATE_SLIDER_DOUBLE(0, spring_k, "Spring Konstant", 0.1, 10.0, "%1.1f", 0, 1);
  ECO_CREATE_SLIDER_INT(0, grid_resolution, "Grid Resolution", 4.0, 64.0, "%1.0f", 0, 2);
  ECO_CREATE_SLIDER_INT(0, min_grid_size, "Minimum Grid Size", 4.0, 128.0, "%1.0f", 0, 3);
  ECO_CREATE_CHECKBOX(0, maximize_effect, "Maximize Effect", 0, 4);
  ECO_PAGE_TABLE_END;
  
  ECO_PAGE_TABLE("Window Matching");
  /* ECO_CREATE_ENTRY(0, map_window_match, "Map Windows", 0, 1);
   * ECO_CREATE_ENTRY(0, focus_window_match, "Focus Windows", 0, 2);
   * ECO_CREATE_ENTRY(0, grab_window_match, "Grab Windows", 0, 3); */
  ECO_CREATE_ENTRY(0, move_window_match, "Move Windows ", 0, 4);
  ECO_PAGE_TABLE_END;

  ECO_PAGE_END;
}




