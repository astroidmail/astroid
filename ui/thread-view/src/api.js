/* --------
 * Public API
 * --------
 */

import * as R from 'ramda/index-es'
import * as U from 'karet.util'
import * as ui from './ui'

/*
const cleanMimeContent = (mimecontent) => {
  const cleanedMimeContent = R.clone(mimecontent)
  if (!Array.isArray(mimecontent.children)) {
    cleanedMimeContent.children = undefined
  }
  return cleanedMimeContent
}
*/

const forceArrayOrNil = R.when(R.complement(Array.isArray), () => undefined)

const toLens = (path) => Array.isArray(path) ? R.lensPath(path) : R.lensProp(path)

/**
 * Clean up invalid/improper JSON that the C++ layer sends us.
 *
 * Astroid's C++ layer is using boost property tree to create json, however this isn't a true
 * json library. It is not capable of serializing empty JSON arrays. Instead what should be empty
 * JSON arrays are sent as empty strings.
 *
 * This function will deeply traverse a Message object and replace all fields that _should_ be
 * empty or undefined Arrays with undefined.
 *
 * @param {Message} message
 * @return {Message} cleaned with all array fields guaranteed to be arrays
 */
const cleanMessage = (message) => {
  const lensToForceAsArray = ['to', 'cc', 'bcc', 'from', 'body'].map(toLens)

  const forceArrayOrNilReducer = (cleanedMessage, lens) => R.over(lens, forceArrayOrNil, cleanedMessage)

  return R.reduce(forceArrayOrNilReducer, message, lensToForceAsArray)
  // cleanedMessage.body = cleanedMessage.body.map(cleanMimeContent)
  // return cleanedMessage
}

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
    focus_next_element: R.curry(focus_next_element)(context)
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
  log.info('add_message', 'received', message)
  const cleanedMessage = cleanMessage(message)
  log.info('add_message', 'cleaned', cleanedMessage)

  U.view('messages', context.state).modify(U.append(cleanedMessage))
}

/**
 * Clears all messages
 *
 * @param {Astroid.Context} context
 * @return {void}
 */
function clear_messages(context) {
  log.info('clear_messages')
  U.view('messages', context.state).set([])
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
}


