# pragma once

# include <stdbool.h>
# include <glib-object.h>
# include <gmime/gmime.h>

G_BEGIN_DECLS

#define ASTROID_THREADVIEW_TYPE_ACTIVATABLE		(astroid_threadview_activatable_get_type ())

#define ASTROID_THREADVIEW_ACTIVATABLE(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), ASTROID_THREADVIEW_TYPE_ACTIVATABLE, AstroidThreadViewActivatable))

#define ASTROID_THREADVIEW_ACTIVATABLE_IFACE(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), ASTROID_THREADVIEW_TYPE_ACTIVATABLE, AstroidThreadViewctivatableInterface))

#define ASTROID_IS_THREADVIEW_ACTIVATABLE(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), ASTROID_THREADVIEW_TYPE_ACTIVATABLE))
#define ASTROID_THREADVIEW_ACTIVATABLE_GET_IFACE(obj)	(G_TYPE_INSTANCE_GET_INTERFACE ((obj), ASTROID_THREADVIEW_TYPE_ACTIVATABLE, AstroidThreadViewActivatableInterface))

typedef struct _AstroidThreadViewActivatable AstroidThreadViewActivatable;
typedef struct _AstroidThreadViewActivatableInterface AstroidThreadViewActivatableInterface;

struct _AstroidThreadViewActivatableInterface
{
	GTypeInterface g_iface;

	void (*activate) (AstroidThreadViewActivatable * activatable);
	void (*deactivate) (AstroidThreadViewActivatable * activatable);
	void (*update_state) (AstroidThreadViewActivatable * activatable);

  char* (*get_avatar_uri) (AstroidThreadViewActivatable * activatable, const char * email, const char * type, int size, GMimeMessage * message);
  GList* (*get_allowed_uris) (AstroidThreadViewActivatable * activatable);

  char* (*format_tags) (AstroidThreadViewActivatable * activatable, const char *bg, GList * tags, bool selected);
};

GType astroid_threadview_activatable_get_type (void) G_GNUC_CONST;

void astroid_threadview_activatable_activate (AstroidThreadViewActivatable *activatable);

void astroid_threadview_activatable_deactivate (AstroidThreadViewActivatable *activatable);

void astroid_threadview_activatable_update_state (AstroidThreadViewActivatable *activatable);


char * astroid_threadview_activatable_get_avatar_uri (AstroidThreadViewActivatable * activatable, const char *email, const char * type, int size, GMimeMessage * message);
GList * astroid_threadview_activatable_get_allowed_uris (AstroidThreadViewActivatable * activatable);

char * astroid_threadview_activatable_format_tags (AstroidThreadViewActivatable * activatable, const char * bg, GList * tags, bool selected);

G_END_DECLS

