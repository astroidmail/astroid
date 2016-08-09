# pragma once

# include <glib-object.h>

G_BEGIN_DECLS

#define ASTROID_TYPE_ACTIVATABLE		(astroid_activatable_get_type ())

#define ASTROID_ACTIVATABLE(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), ASTROID_TYPE_ACTIVATABLE, AstroidActivatable))

#define ASTROID_ACTIVATABLE_IFACE(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), ASTROID_TYPE_ACTIVATABLE, AstroidActivatableInterface))

#define ASTROID_IS_ACTIVATABLE(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), ASTROID_TYPE_ACTIVATABLE))
#define ASTROID_ACTIVATABLE_GET_IFACE(obj)	(G_TYPE_INSTANCE_GET_INTERFACE ((obj), ASTROID_TYPE_ACTIVATABLE, AstroidActivatableInterface))

typedef struct _AstroidActivatable AstroidActivatable;
typedef struct _AstroidActivatableInterface AstroidActivatableInterface;

struct _AstroidActivatableInterface
{
	GTypeInterface g_iface;

	void (*activate) (AstroidActivatable * activatable);
	void (*deactivate) (AstroidActivatable * activatable);
	void (*update_state) (AstroidActivatable * activatable);

  const char * (*get_user_agent) (AstroidActivatable * activatable);
  const char * (*generate_mid) (AstroidActivatable * activatable);

};

GType astroid_activatable_get_type (void) G_GNUC_CONST;

void astroid_activatable_activate (AstroidActivatable *activatable);

void astroid_activatable_deactivate (AstroidActivatable *activatable);

void astroid_activatable_update_state (AstroidActivatable *activatable);

const char * astroid_activatable_get_user_agent (AstroidActivatable * activatable);
const char * astroid_activatable_generate_mid (AstroidActivatable * activatable);

G_END_DECLS

