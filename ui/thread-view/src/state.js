/* --------
 * State management
 * --------
 */

import * as U from 'karet.util'

/**
 * @name Astroid.Meta
 * @type {{
 *     version: String
 * }}
 */

/**
 * @name Astroid.Flags
 * @type {{
 *     focused: Boolean,
 *     expanded: Boolean
 * }}
 */

/**
 * Global app state
 *
 * Contains all the state used to render the thread view, messages, elements, focus flags etc
 * @name Astroid.Context
 * @type {{
 *     meta: Astroid.Meta,
 *     state: {
 *       messages: Array<Astroid.Message>,
 *       elements: Astroid.Elements,
 *       flags: Object.<string, Astroid.Flags>
 *     }
 * }}
 */

/**
 * @type {Astroid.Meta}
 */
const meta = {
  version: '0.1'
}

export const newState = (initial_state) => U.atom(initial_state.value)

/**
 * Create a new context for the app
 *
 * @param initial_state
 * @returns {Astroid.Context}
 */
export function context(initial_state) {
  return {
    meta,
    state: newState(initial_state)
  }
}

