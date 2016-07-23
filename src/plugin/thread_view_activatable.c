# include "thread_view_activatable.h"
# include <gtk/gtk.h>
# include <webkit/webkit.h>

/**
 * SECTION: astroid_threadview_activatable
 * @short_description: Interface for activatable extensions on the shell
 * @see_also: #PeasExtensionSet
 *
 * #AstroidthreadviewActivatable is an interface which should be implemented by
 * extensions that should be activated on the Liferea main window.
 **/

G_DEFINE_INTERFACE (AstroidThreadViewActivatable, astroid_threadview_activatable, G_TYPE_OBJECT)

void
astroid_threadview_activatable_default_init (AstroidThreadViewActivatableInterface *iface)
{
	static gboolean initialized = FALSE;

	if (!initialized) {
		/**
		 * AstroidthreadviewActivatable:window:
		 *
		 * The window property contains the gtr window for this
		 * #AstroidActivatable instance.
		 */
		g_object_interface_install_property (iface,
                           g_param_spec_object ("thread_view",
                                                "thread_view",
                                                "The threadview box",
                                                GTK_TYPE_BOX,
                                                G_PARAM_READWRITE));

		g_object_interface_install_property (iface,
                           g_param_spec_object ("web_view",
                                                "web_view",
                                                "The WebKit Webview",
                                                WEBKIT_TYPE_WEB_VIEW,
                                                G_PARAM_READWRITE));

		initialized = TRUE;
	}
}

/**
 * astroid_threadview_activatable_activate:
 * @activatable: A #AstroidThreadViewActivatable.
 *
 * Activates the extension on the shell property.
 */
void
astroid_threadview_activatable_activate (AstroidThreadViewActivatable * activatable)
{
	AstroidThreadViewActivatableInterface *iface;

	g_return_if_fail (ASTROID_IS_THREADVIEW_ACTIVATABLE (activatable));

	iface = ASTROID_THREADVIEW_ACTIVATABLE_GET_IFACE (activatable);
	if (iface->activate)
		iface->activate (activatable);
}

/**
 * astroid_threadview_activatable_deactivate:
 * @activatable: A #AstroidThreadViewActivatable.
 *
 * Deactivates the extension on the shell property.
 */
void
astroid_threadview_activatable_deactivate (AstroidThreadViewActivatable * activatable)
{
	AstroidThreadViewActivatableInterface *iface;

	g_return_if_fail (ASTROID_IS_THREADVIEW_ACTIVATABLE (activatable));

	iface = ASTROID_THREADVIEW_ACTIVATABLE_GET_IFACE (activatable);
	if (iface->deactivate)
		iface->deactivate (activatable);
}

/**
 * astroid_threadview_activatable_update_state:
 * @activatable: A #AstroidThreadViewActivatable.
 *
 * Triggers an update of the extension internal state to take into account
 * state changes in the window, due to some event or user action.
 */
void
astroid_threadview_activatable_update_state (AstroidThreadViewActivatable * activatable)
{
	AstroidThreadViewActivatableInterface *iface;

	g_return_if_fail (ASTROID_IS_THREADVIEW_ACTIVATABLE (activatable));

	iface = ASTROID_THREADVIEW_ACTIVATABLE_GET_IFACE (activatable);
	if (iface->update_state)
		iface->update_state (activatable);
}


/**
 * astroid_activatable_get_avatar_uri:
 * @activatable: A #AstroidThreadViewActivatable.
 * @email: A #utf8.
 * @type:  A #string.
 * @size:  A #int.
 * @message: A #GMime.Message.
 *
 * Returns: (transfer none): A #string.
 */
char *
astroid_threadview_activatable_get_avatar_uri (AstroidThreadViewActivatable * activatable, const char * email, const char * type, int size, GMimeMessage * message)
{
	AstroidThreadViewActivatableInterface *iface;

	if (!ASTROID_IS_THREADVIEW_ACTIVATABLE (activatable)) return NULL;

	iface = ASTROID_THREADVIEW_ACTIVATABLE_GET_IFACE (activatable);
	if (iface->get_avatar_uri)
		return iface->get_avatar_uri (activatable, email, type, size, message);

  return NULL;
}

/**
 * astroid_threadview_activatable_get_allowed_uris:
 * @activatable: A #AstroidThreadViewActivatable.
 *
 * Returns: (element-type utf8) (transfer container): List of allowed uris.
 */
GList *
astroid_threadview_activatable_get_allowed_uris (AstroidThreadViewActivatable * activatable)
{
	AstroidThreadViewActivatableInterface *iface;

	if (!ASTROID_IS_THREADVIEW_ACTIVATABLE (activatable)) return NULL;

	iface = ASTROID_THREADVIEW_ACTIVATABLE_GET_IFACE (activatable);
	if (iface->get_allowed_uris)
		return iface->get_allowed_uris (activatable);

  return NULL;
}

/**
 * astroid_threadview_activatable_format_tags:
 * @activatable: A #AstroidThreadViewActivatable.
 * @bg : A #utf8.
 * @tags: (element-type utf8) (transfer none): List of #utf8.
 * @selected: A #bool.
 *
 */
char *
astroid_threadview_activatable_format_tags (AstroidThreadViewActivatable * activatable, const char * bg, GList * tags, bool selected)
{
	AstroidThreadViewActivatableInterface *iface;

	if (!ASTROID_IS_THREADVIEW_ACTIVATABLE (activatable)) return NULL;

	iface = ASTROID_THREADVIEW_ACTIVATABLE_GET_IFACE (activatable);
	if (iface->format_tags)
		return iface->format_tags (activatable, bg, tags, selected);

  return NULL;
}

