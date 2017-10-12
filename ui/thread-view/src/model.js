import * as R from 'ramda/index-es'
import * as L from 'partial.lenses'

export const Contact = {
  address: ['address']
}

export const Tag     = {
  tag: 'tag',
  fg: ['fg', L.define('#000000')],
  bg: ['bg', L.define('rgba(225, 171, 21, 0.5)')]
}
export const Message = {

  id: ['id'],

  subject: ['subject', L.define('')],
  gravatar: ['gravatar', L.define('')],
  from: ['from', L.define([])],
  to: ['to', L.define([])],
  cc: ['cc', L.define([])],
  bcc: ['bcc', L.define([])],
  date: ['date'],
  tags: ['tags', L.define([])],
  preview: ['preview', L.define('')],

  body: [
    'body',
    L.define([]),

    L.normalize(R.unnest),
    L.normalize(R.sortBy(R.compose(R.not, L.get('preferred'))))
  ]

}
const setFocus       = focused => mid => L.set(['flags', L.define({}), mid, 'focused'], focused)
const setFocusAllBut = focused => mid => L.set(
  ['flags', L.define({}),
    L.values, L.when((_, k) => k !== mid),
    'focused'
  ],
  focused)

export const Model = {
  messages: [
    'messages',
    L.define([]),
    L.normalize(R.sortBy(L.get('id')))
  ],
  Message: [],

  elements: [
    'elements',
    L.define([])
  ],

  focused: [
    'focused'
  ],

  messageById: (mid) => {
    return [
      'messages',
      L.define([]),
      L.normalize(R.sortBy(L.get('id'))),
      L.find(m => m.id === mid)
    ]

  },

  flagsById: (id) => {
    return [
      'flags',
      L.define({}),
      L.prop(id),
      L.define({ focused: false, expanded: false })
    ]
  },

  focus: setFocus(true),
  defocus: setFocus(false),

  focusAllBut: setFocusAllBut(true),
  defocusAllBut: setFocusAllBut(false)

}

export const Flags = {

}

/*
const cleanMimeContent = (mimecontent) => {
  const cleanedMimeContent = R.clone(mimecontent)
  if (!Array.isArray(mimecontent.children)) {
    cleanedMimeContent.children = undefined
  }
  return cleanedMimeContent
}
*/

// Higher order function: returns a function (value, lens) => value that acts as a reducer over
// lens modification
const fieldReducer = (operator) => (m, lens) => L.modify(lens, operator, m)

// accumulates a new value by applying the operator to all fields in message focused by every lens
// in lenses
const applyToAllLenses = (operator, lenses, message) => R.reduce(fieldReducer(operator), message, lenses)

// A function returning a Lens that will recursively descend a tree structure of Arrays and Objects
// and return the lens focusing on an object key named 'field'
const recurseTreeFor = field => {
  if (!R.is(Array, field)) {
    field = [field]
  }
  return L.lazy(rec =>
    L.iftes(
      R.is(Array), [L.elems, rec],
      R.is(Object), [L.values, L.iftes((_, key) => R.contains(key, field), [], rec)]
    )
  )
}

function fixupArrays(message) {
  // lenses to the fields we want to be arrays
  const fieldsToForceAsArray = [
    'to', 'cc', 'bcc', 'from', 'body',
    recurseTreeFor('children'),
    'mime_messages',
    'attachments'
  ]

  const forceArray = R.when(R.complement(Array.isArray), () => [])
  return applyToAllLenses(forceArray, fieldsToForceAsArray, message)
}

function fixupBooleans(message) {

  // lenses to focus one every boolean field
  const toBools = [
    'focused',
    'missing_content',
    'patch',
    'sibling',
    recurseTreeFor('sibling'),
    recurseTreeFor('preferred'),
    recurseTreeFor('encrypted'),
    recurseTreeFor('signed')
  ]

  const forceBool = R.ifElse(R.equals('true'), () => true, () => false)
  return applyToAllLenses(forceBool, toBools, message)
}

/**
 * Clean up invalid/improper JSON that the C++ layer sends us.
 *
 * Astroid's C++ layer is using boost property tree to create json, however this isn't a true
 * json library. It is not capable of serializing empty JSON arrays. Instead what should be empty
 * JSON arrays are sent as empty strings.
 *
 * It also sends booleans as strings (i.e., 'true'), which we need to fix up.
 *
 * This function will deeply traverse a Message object and replace all fields that _should_ be
 * empty or undefined Arrays with undefined.
 *
 * @param {Message} message
 * @return {Message} cleaned with all array fields guaranteed to be arrays
 */
export function cleanMessage(message) {
  return fixupArrays(fixupBooleans(message))
}

export function buildElements(messages) {
  /*
  returns [ {mid: xx, eid: xx} { mid: xx, eid: 1}, { mid: xx, eid: 2} , {mid: yy, eid: yy}, {mid: yy, eid 3} ...]
   */
  return R.chain(message =>
    R.concat(
      [{ mid: message.id, eid: message.id }],
      R.map(eid => ({ mid: message.id, eid }), L.collect(recurseTreeFor(['eid']), message))
    )
    , messages)
}

window.Model   = Model
window.Message = Message


