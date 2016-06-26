# pragma once

# include <stdbool.h>
# include <glib-object.h>

G_BEGIN_DECLS

#define ASTROID_THREADINDEX_TYPE_ACTIVATABLE		(astroid_threadindex_activatable_get_type ())

#define ASTROID_THREADINDEX_ACTIVATABLE(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), ASTROID_THREADINDEX_TYPE_ACTIVATABLE, AstroidThreadIndexActivatable))

#define ASTROID_THREADINDEX_ACTIVATABLE_IFACE(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), ASTROID_THREADINDEX_TYPE_ACTIVATABLE, AstroidThreadIndexActivatableInterface))

#define ASTROID_IS_THREADINDEX_ACTIVATABLE(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), ASTROID_THREADINDEX_TYPE_ACTIVATABLE))
#define ASTROID_THREADINDEX_ACTIVATABLE_GET_IFACE(obj)	(G_TYPE_INSTANCE_GET_INTERFACE ((obj), ASTROID_THREADINDEX_TYPE_ACTIVATABLE, AstroidThreadIndexActivatableInterface))

typedef struct _AstroidThreadIndexActivatable AstroidThreadIndexActivatable;
typedef struct _AstroidThreadIndexActivatableInterface AstroidThreadIndexActivatableInterface;

struct _AstroidThreadIndexActivatableInterface
{
	GTypeInterface g_iface;

	void (*activate) (AstroidThreadIndexActivatable * activatable);
	void (*deactivate) (AstroidThreadIndexActivatable * activatable);
	void (*update_state) (AstroidThreadIndexActivatable * activatable);

  char* (*format_tags) (AstroidThreadIndexActivatable * activatable, const char *bg, GList * tags, bool selected);
};

GType astroid_threadindex_activatable_get_type (void) G_GNUC_CONST;

void astroid_threadindex_activatable_activate (AstroidThreadIndexActivatable *activatable);

void astroid_threadindex_activatable_deactivate (AstroidThreadIndexActivatable *activatable);

void astroid_threadindex_activatable_update_state (AstroidThreadIndexActivatable *activatable);

char * astroid_threadindex_activatable_format_tags (AstroidThreadIndexActivatable * activatable, const char * bg, GList * tags, bool selected);


G_END_DECLS

