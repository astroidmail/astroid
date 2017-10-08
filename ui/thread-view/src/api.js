/* --------
 * Public API
 * --------
 */

import * as R from 'ramda/index-es'
import * as U from 'karet.util'
import * as L from 'partial.lenses'
import * as ui from './ui'
import { buildElements, cleanMessage, Model } from './model'

/* --------
 * Logging helpers
 * --------
 */

const log = {
  info() {
    console.log(`[Astroid::${R.head(arguments)}]`, ...R.tail(arguments))
  }
}

/**
 * Initialize the Astroid System and UI
 */
export function init(context) {
  ui.render(context)

  return {
    add_message: R.curry(add_message)(context),
    clear_messages: R.curry(clear_messages)(context),
    indent_state: R.curry(indent_state)(context),
    remove_message: R.curry(remove_message)(context),
    set_warning: R.curry(set_warning)(context),
    hide_warning: R.curry(hide_warning)(context),
    set_info: R.curry(set_info)(context),
    hide_info: R.curry(hide_info)(context),
    focus_next_element: R.curry(focus_next_element)(context),
    focus_previous_element: R.curry(focus_previous_element)(context)
  }
}

/**
 * Add a message. If it already exists, it will be updated.
 *
 * @param {Astroid.Context} context
 * @param {Message} message - the email message to add
 * @return {void}
 */

function add_message(context, message) {
  const cleanedMessage = cleanMessage(message)
  log.info('add_message', message.id, 'original', message, 'cleaned', cleanedMessage)

  context.state.modify(L.set(Model.messageById(message.id), cleanedMessage))
  context.state.modify(L.set('elements', buildElements(context.state.view('messages').get())))

  if (R.isNil(context.state.view('focused').get())) {
    focus_next_element(context)
  }
}

/**
 * Clears all messages
 *
 * @param {Astroid.Context} context
 * @return {void}
 */
function clear_messages(context) {
  log.info('clear_messages')
  U.view(Model.messages, context.state).set([])
}

/**
 *
 * @param {Astroid.Context} context
 * @param {String} mid the message id to indent
 * @param doIndent
 */
function indent_state(context, mid, doIndent) {
  log.info('indent_state', ...arguments)
}

/**
 * Remove a message from the view
 *
 * @param {Astroid.Context} context
 * @param {String} mid the message id to remove
 */
function remove_message(context, mid) {
  log.info('remove_message', ...arguments)
}

/**
 *
 * @param {Astroid.Context} context
 * @param mid
 * @param text
 */
function set_warning(context, mid, text) {
  log.info('set_warning', ...arguments)
}

/**
 *
 * @param {Astroid.Context} context
 * @param mid
 */
function hide_warning(context, mid) {
  log.info('hide_warning', ...arguments)
}

/**
 *
 * @param {Astroid.Context} context
 * @param mid
 * @param text
 */
function set_info(context, mid, text) {
  log.info('set_info', ...arguments)
}

/**
 * @param {Astroid.Context} context
 * @param mid
 */
function hide_info(context, mid) {
  log.info('hide_info', ...arguments)
}

/**
 * Jump to the next element if no scrolling is necessary, otherwise scroll a small amount.
 *
 * @param {Astroid.Context} context
 * @param {Boolean} force_change if true, then always move to focus the next element
 * @return the id of the currently selected element
 */
function focus_next_element(context, force_change) {
  log.info('focus_next_element', ...arguments)
  // const state = context.state.get()

  const elements      = context.state.view('elements')
  const focusedIdx    = context.state.view(['focusedIdx', L.define(0)])
  const totalElements = elements.get().length

  const newIdx = Math.min(focusedIdx.get() + 1, totalElements - 1)

  return set_focus(context, newIdx)
}

/**
 * Jump to the previous element if no scrolling is necessary, otherwise scroll a small amount.
 *
 * @param {Astroid.Context} context
 * @param {Boolean} force_change if true, then always move to focus the next element
 * @return the id of the currently selected element
 */
function focus_previous_element(context, force_change) {
  log.info('focus_previous_element', ...arguments)

  const focusedIdx = context.state.view(['focusedIdx', L.define(0)])

  const newIdx = Math.max(focusedIdx.get() - 1, 0)

  return set_focus(context, newIdx)
}

function set_focus(context, newIdx) {
  const elements   = context.state.view('elements')
  const focusedIdx = context.state.view(['focusedIdx', L.define(0)])
  const old        = focusedIdx.get()

  context.state.modify(L.set('focusedIdx', newIdx))
  const now     = focusedIdx.get()
  const element = elements.view(L.index(now)).get()
  context.state.modify(L.set('focused', element))

  console.log('focused is now ', now, element, ' but was', old)
  return element
}


