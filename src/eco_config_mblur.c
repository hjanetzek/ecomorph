#include "e_mod_main.h"


#define NUM_MODES 2
static char *filters[NUM_MODES] = {"Texture Copy", "Accumulation buffer"};

/* Apply Function */
static void
_apply(E_Config_Dialog_Data *cfdata)
{
   eco_config_group_apply("mblur");  
}

/* Main creation function */
EAPI void
eco_config_mblur(void *data)
{
   ECO_PAGE_BEGIN("mblur");
   ECO_PAGE_TABLE("Options");

   ECO_CREATE_CHECKBOX(0, on_transformed_screen, "Motion Blur on Transformed Screen", 0, 0);
   ECO_CREATE_RADIO_GROUP(0, mode, "Motion Blur mode", filters, NUM_MODES, 0, 1);

   ECO_CREATE_SLIDER_DOUBLE(0, strength, "Motion Blur Strength", 0.01, 1.0, "%1.2f", 0, 2);

   ECO_PAGE_TABLE_END;
   ECO_PAGE_END;

}
