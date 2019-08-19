# include "astroid_activatable.h"

/**
 * SECTION: astroid_activatable
 * @short_description: Interface for activatable extensions on the shell
 * @see_also: #PeasExtensionSet
 *
 * #AstroidActivatable is an interface which should be implemented by
 * extensions that should be activated on the Liferea main window.
 **/

G_DEFINE_INTERFACE (AstroidActivatable, astroid_activatable, G_TYPE_OBJECT)

void
astroid_activatable_default_init (AstroidActivatableInterface *iface)
{
    static gboolean initialized = FALSE;

  (void)(iface); /* unused */

    if (!initialized) {
        /**
         * AstroidActivatable:window:
         *
         * The window property contains the gtr window for this
         * #AstroidActivatable instance.
         */
    /*
        g_object_interface_install_property (iface,
                           g_param_spec_object ("shell",
                                                "Shell",
                                                "The Liferea shell",
                                                ASTROID_TYPE,
                                                G_PARAM_READWRITE |
                                                G_PARAM_CONSTRUCT_ONLY |
                                                G_PARAM_STATIC_STRINGS));

                                                */
        initialized = TRUE;
    }
}

/**
 * astroid_activatable_activate:
 * @activatable: A #AstroidActivatable.
 *
 * Activates the extension on the shell property.
 */
void
astroid_activatable_activate (AstroidActivatable * activatable)
{
    AstroidActivatableInterface *iface;

    g_return_if_fail (ASTROID_IS_ACTIVATABLE (activatable));

    iface = ASTROID_ACTIVATABLE_GET_IFACE (activatable);
    if (iface->activate)
        iface->activate (activatable);
}

/**
 * astroid_activatable_deactivate:
 * @activatable: A #AstroidActivatable.
 *
 * Deactivates the extension on the shell property.
 */
void
astroid_activatable_deactivate (AstroidActivatable * activatable)
{
    AstroidActivatableInterface *iface;

    g_return_if_fail (ASTROID_IS_ACTIVATABLE (activatable));

    iface = ASTROID_ACTIVATABLE_GET_IFACE (activatable);
    if (iface->deactivate)
        iface->deactivate (activatable);
}

/**
 * astroid_activatable_update_state:
 * @activatable: A #AstroidActivatable.
 *
 * Triggers an update of the extension internal state to take into account
 * state changes in the window, due to some event or user action.
 */
void
astroid_activatable_update_state (AstroidActivatable * activatable)
{
    AstroidActivatableInterface *iface;

    g_return_if_fail (ASTROID_IS_ACTIVATABLE (activatable));

    iface = ASTROID_ACTIVATABLE_GET_IFACE (activatable);
    if (iface->update_state)
        iface->update_state (activatable);
}

/**
 * astroid_activatable_get_user_agent:
 * @activatable: A #AstroidActivatable.
 *
 * Returns: (transfer none): A #string.
 */
const char *
astroid_activatable_get_user_agent (AstroidActivatable * activatable)
{
    AstroidActivatableInterface *iface;

    if (!ASTROID_IS_ACTIVATABLE (activatable)) return NULL;

    iface = ASTROID_ACTIVATABLE_GET_IFACE (activatable);
    if (iface->get_user_agent)
        return iface->get_user_agent (activatable);

  return NULL;
}

/**
 * astroid_activatable_generate_mid:
 * @activatable: A #AstroidActivatable.
 *
 * Returns: (transfer none): A #string.
 */
const char *
astroid_activatable_generate_mid (AstroidActivatable * activatable)
{
    AstroidActivatableInterface *iface;

    if (!ASTROID_IS_ACTIVATABLE (activatable)) return NULL;

    iface = ASTROID_ACTIVATABLE_GET_IFACE (activatable);
    if (iface->generate_mid)
        return iface->generate_mid (activatable);

  return NULL;
}

/**
 * astroid_activatable_get_tag_colors:
 * @activatable: A #AstroidActivatable.
 * @tag : A #utf8.
 * @bg : A #utf8.
 *
 * Returns: (element-type utf8) (transfer container): List of forground and background color for tag.
 */
GList *
astroid_activatable_get_tag_colors (AstroidActivatable * activatable, const char * tag, const char * bg)
{
    AstroidActivatableInterface *iface;

    if (!ASTROID_IS_ACTIVATABLE (activatable)) return NULL;

    iface = ASTROID_ACTIVATABLE_GET_IFACE (activatable);
    if (iface->get_tag_colors)
        return iface->get_tag_colors (activatable, tag, bg);

  return NULL;
}

/**
 * astroid_activatable_get_queries:
 * @activatable: A #AstroidActivatable.
 *
 * Returns: (element-type utf8) (transfer container): List of lists containing a name and a query.
 */
GList *
astroid_activatable_get_queries (AstroidActivatable * activatable)
{
    AstroidActivatableInterface *iface;

    if (!ASTROID_IS_ACTIVATABLE (activatable)) return NULL;

    iface = ASTROID_ACTIVATABLE_GET_IFACE (activatable);
    if (iface->get_queries)
        return iface->get_queries (activatable);

  return NULL;
}

/**
 * astroid_activatable_process:
 * @activatable: A #AstroidActivatable.
 * @fname: A #utf8.
 *
 * Returns: (transfer full): Stream of the processed raw message.
 */
GMimeStream *
astroid_activatable_process (AstroidActivatable * activatable, const char * fname)
{
  AstroidActivatableInterface *iface;

  if (!ASTROID_IS_ACTIVATABLE (activatable)) return NULL;

  iface = ASTROID_ACTIVATABLE_GET_IFACE (activatable);
  if (iface->process) {
    return iface->process (activatable, fname);
  }

  return NULL;
}

