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
 * Global app state
 * @name Astroid.Context
 * @type {{
 *     meta: Astroid.Meta,
 *     state: *
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

