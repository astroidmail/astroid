# include "thread_index_activatable.h"
# include <gtk/gtk.h>

/**
 * SECTION: astroid_threadindex_activatable
 * @short_description: Interface for activatable extensions on the shell
 * @see_also: #PeasExtensionSet
 *
 * #AstroidThreadIndexActivatable is an interface which should be implemented by
 * extensions that should be activated on the Liferea main window.
 **/

G_DEFINE_INTERFACE (AstroidThreadIndexActivatable, astroid_threadindex_activatable, G_TYPE_OBJECT)

void
astroid_threadindex_activatable_default_init (AstroidThreadIndexActivatableInterface *iface)
{
	static gboolean initialized = FALSE;

	if (!initialized) {
		/**
		 * AstroidThreadIndexActivatable:window:
		 *
		 * The window property contains the gtr window for this
		 * #AstroidActivatable instance.
		 */
		g_object_interface_install_property (iface,
                           g_param_spec_object ("thread_index",
                                                "thread_index",
                                                "The ThreadIndex box",
                                                GTK_TYPE_BOX,
                                                G_PARAM_READWRITE));

		initialized = TRUE;
	}
}

/**
 * astroid_threadindex_activatable_activate:
 * @activatable: A #AstroidThreadIndexActivatable.
 *
 * Activates the extension on the shell property.
 */
void
astroid_threadindex_activatable_activate (AstroidThreadIndexActivatable * activatable)
{
	AstroidThreadIndexActivatableInterface *iface;

	g_return_if_fail (ASTROID_IS_THREADINDEX_ACTIVATABLE (activatable));

	iface = ASTROID_THREADINDEX_ACTIVATABLE_GET_IFACE (activatable);
	if (iface->activate)
		iface->activate (activatable);
}

/**
 * astroid_threadindex_activatable_deactivate:
 * @activatable: A #AstroidThreadIndexActivatable.
 *
 * Deactivates the extension on the shell property.
 */
void
astroid_threadindex_activatable_deactivate (AstroidThreadIndexActivatable * activatable)
{
	AstroidThreadIndexActivatableInterface *iface;

	g_return_if_fail (ASTROID_IS_THREADINDEX_ACTIVATABLE (activatable));

	iface = ASTROID_THREADINDEX_ACTIVATABLE_GET_IFACE (activatable);
	if (iface->deactivate)
		iface->deactivate (activatable);
}

/**
 * astroid_threadindex_activatable_update_state:
 * @activatable: A #AstroidThreadIndexActivatable.
 *
 * Triggers an update of the extension internal state to take into account
 * state changes in the window, due to some event or user action.
 */
void
astroid_threadindex_activatable_update_state (AstroidThreadIndexActivatable * activatable)
{
	AstroidThreadIndexActivatableInterface *iface;

	g_return_if_fail (ASTROID_IS_THREADINDEX_ACTIVATABLE (activatable));

	iface = ASTROID_THREADINDEX_ACTIVATABLE_GET_IFACE (activatable);
	if (iface->update_state)
		iface->update_state (activatable);
}

/**
 * astroid_threadindex_activatable_format_tags:
 * @activatable: A #AstroidThreadIndexActivatable.
 * @bg : A #utf8.
 * @tags: (element-type utf8) (transfer none): List of #utf8.
 * @selected: A #bool.
 *
 */
char *
astroid_threadindex_activatable_format_tags (AstroidThreadIndexActivatable * activatable, const char * bg, GList * tags, bool selected)
{
	AstroidThreadIndexActivatableInterface *iface;

	if (!ASTROID_IS_THREADINDEX_ACTIVATABLE (activatable)) return NULL;

	iface = ASTROID_THREADINDEX_ACTIVATABLE_GET_IFACE (activatable);
	if (iface->format_tags)
		return iface->format_tags (activatable, bg, tags, selected);

  return NULL;
}

