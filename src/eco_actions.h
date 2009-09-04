#ifndef ECO_ACTIONS_H
#define ECO_ACTIONS_H


EAPI void eco_actions_create(void);
EAPI void eco_actions_free(void);
EAPI void eco_action_terminate(void); /*TODO pass info which plugin
					send the terminate notify */

#endif
