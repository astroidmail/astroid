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
    expand_message : R.curry(expand_message)(context),
    collapse_message : R.curry(collapse_message)(context),
    remove_message: R.curry(remove_message)(context),
    set_warning: R.curry(set_warning)(context),
    hide_warning: R.curry(hide_warning)(context),
    set_info: R.curry(set_info)(context),
    hide_info: R.curry(hide_info)(context),
    focus_next_element: R.curry(focus_next_element)(context),
    focus_previous_element: R.curry(focus_previous_element)(context)
  }
}

// https://gist.github.com/davidtheclark/5515733
function isAnyPartOfElementInViewport(el) {

  const rect         = el.getBoundingClientRect()
  // DOMRect { x: 8, y: 8, width: 100, height: 100, top: 8, right: 108, bottom: 108, left: 8 }
  const windowHeight = (window.innerHeight || document.documentElement.clientHeight)
  const windowWidth  = (window.innerWidth || document.documentElement.clientWidth)

  // http://stackoverflow.com/questions/325933/determine-whether-two-date-ranges-overlap
  const vertInView = (rect.top <= windowHeight) && ((rect.top + rect.height) >= 0)
  const horInView  = (rect.left <= windowWidth) && ((rect.left + rect.width) >= 0)

  return (vertInView && horInView)
}

function isElementContainedInViewport(el) {
  if (!el) {
    return undefined
  }
  const rect = el.getBoundingClientRect()
  return (
    rect.top >= 0 &&
    rect.left >= 0 &&
    rect.bottom <= (window.innerHeight || document.documentElement.clientHeight) &&
    rect.right <= (window.innerWidth || document.documentElement.clientWidth)
  )
}

/**
 * Add a message. If it already exists, it will be updated.
 *
 * @param {Astroid.Context} context
 * @param {Message} message - the email message to add
 * @return {void}
 */

function add_message(context, message) {
  const { state } = context

  // clean the message
  const cleanedMessage = cleanMessage(message)
  log.info('add_message', message.id, 'original', message, 'cleaned', cleanedMessage)

  // add it to state.messages
  state.modify(L.set(Model.messageById(message.id), cleanedMessage))

  // rebuild state.elements
  const elements = buildElements(state.view('messages').get())
  state.modify(L.set('elements', elements))

  // set state.flags for message
  state.modify(L.set(Model.flagsById(message.id), undefined))

  log.info('new flags', state.view('flags').get())


  if (R.isNil(context.state.view('focused').get())) {
    console.log('SETTING FIRST FOCUS')
    set_focused(context, elements[0], 0)
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
 *
 * @param {Astroid.Context} context
 * @param {String} mid the message id to expand
 */
function expand_message(context, mid) {
  log.info('expand_message', ...arguments)
  set_expanded (context, mid, true);
}

/**
 *
 * @param {Astroid.Context} context
 * @param {String} mid the message id to collapse
 */
function collapse_message(context, mid) {
  log.info('collapse_message', ...arguments)
  set_expanded (context, mid, false);
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

  const newIdx     = Math.min(focusedIdx.get() + 1, totalElements - 1)
  const newElement = elements.view(L.index(newIdx)).get()

  return move_focus(context, newElement, newIdx)
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
  const elements   = context.state.view('elements')

  const newIdx     = Math.max(focusedIdx.get() - 1, 0)
  const newElement = elements.view(L.index(newIdx)).get()

  return move_focus(context, newElement, newIdx)
}

/**
 * Move focus towards the given element
 * @param context
 * @param targetElement
 * @param targetIndex
 */
function move_focus(context, targetElement, targetIndex) {
  const { state }        = context
  const elements         = state.view('elements')
  const flags            = state.view('flags')
  const oldIdx           = state.view(['focusedIdx', L.define(undefined)]).get() // can be undefined
  const nothingIsFocused = R.isNil(oldIdx)
  const oldElement       = nothingIsFocused || elements.view(L.index(oldIdx)).get()
  const directionDown    = nothingIsFocused ? true : (targetIndex - oldIdx) > 0
  const $el              = document.getElementById(targetElement.eid)

  // check if element's message is expanded

  const targetFlags = state.view(Model.flagsById(targetElement.mid)).get()
  if (targetFlags.expanded) {
    console.log('target\'s msg is expanded')
  } else {
    console.log('target\'s msg is collapsed')
  }

  console.log('want to focus: ', targetElement, $el)
  if ($el && isAnyPartOfElementInViewport(($el))) {
    console.log(targetElement.eid, ' visible? yes')
    set_focused(context, targetElement, targetIndex)
  } else {
    console.log(targetElement.eid, ' visible? no')

    if (directionDown) {
      scroll_down()
    } else {
      scroll_up()
    }
  }

  var _focused_el = get_focused_element(context);
  console.log ('returning:',  _focused_el.eid);

  return _focused_el.mid + ',' + _focused_el.eid;
}

function set_focused(context, targetElement, targetIndex) {
  const { state } = context

  state.modify(L.set('focusedIdx', targetIndex))
  state.modify(L.set('focused', targetElement))

  const { mid, eid } = targetElement

  state.modify(Model.focus(mid))
  state.modify(Model.defocusAllBut(mid))

  console.log('focusing', targetElement, targetIndex)
}

function get_focused_element(context) {
  const { state }        = context
  const elements         = state.view('elements')
  const oldIdx           = state.view(['focusedIdx', L.define(undefined)]).get() // can be undefined
  const nothingIsFocused = R.isNil(oldIdx)
  return nothingIsFocused || elements.view(L.index(oldIdx)).get()
}

function scroll_down() {
  console.log('going... DOWN')
  window.scrollBy(0, 50)
}

function scroll_up() {
  console.log('going... UP')
  window.scrollBy(0, -50)
}

function set_expanded (context, mid, expand) {
  const { state } = context

  if (expand) {
    state.modify(Model.expand(mid))
  } else {
    state.modify(Model.collapse(mid))
  }

  console.log('expand', mid, expand)
}

