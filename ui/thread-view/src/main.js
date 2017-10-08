/*
                     ██                   ██      ██
                    ░██                  ░░      ░██
  ██████    ██████ ██████ ██████  ██████  ██     ░██
 ░░░░░░██  ██░░░░ ░░░██░ ░░██░░█ ██░░░░██░██  ██████
  ███████ ░░█████   ░██   ░██ ░ ░██   ░██░██ ██░░░██
 ██░░░░██  ░░░░░██  ░██   ░██   ░██   ░██░██░██  ░██
░░████████ ██████   ░░██ ░███   ░░██████ ░██░░██████
 ░░░░░░░░ ░░░░░░     ░░  ░░░     ░░░░░░  ░░  ░░░░░░

  main.js: entry point for the js layer. executed when the script is loaded.
*/

import * as Kefir from 'kefir'
import * as L from 'partial.lenses'
import * as R from 'ramda'
import * as U from 'karet.util'
import * as State from './state'
import * as Astroid from './api'
import * as UI from './ui'
import { EmailView } from './thread-view'
// import * as React    from "karet"

/*
const initialState = {
  value: JSON.parse(
    document
      .getElementById('app-initial-state')
      .innerText || {}
  )
}
*/

/**
 * The application context.
 *
 * Contains the state and meta information used throughout the app.
 *
 * @type {Astroid.Context}
 */
const context = State.context({})

if (process.env.NODE_ENV !== 'production') {
  window.Kefir   = Kefir
  window.L       = L
  window.R       = R
  window.U       = U
  window.context = context
  window.state   = context.state
  window.State   = State
  window.UI      = UI

  // state.log('state')
  // console.log('context is', context)
}

/* The C++ layer can only access global objects, so we attach our API in the global namespace */

window['Astroid'] = Astroid.init(context)

UI.render(
  UI.k(EmailView, { messages: U.view('messages', context.state) }),
  document.body,
  document.getElementById('app')
)

console.log('--- Astroid Ready ---')

