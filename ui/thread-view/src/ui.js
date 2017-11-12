import karet from 'karet'
import hyperx from 'hyperx/index'
import preact from 'preact'

// preact's render function
export const render = preact.render
// preact's createElement function wrapped by Karet to be Observeable-aware
export const k      = karet.createElement
// tagged template string virtual dom builder
export const kx     = hyperx(k)
