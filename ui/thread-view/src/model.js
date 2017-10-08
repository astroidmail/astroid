import * as R from 'ramda/index-es'
import * as L from 'partial.lenses'

export const Contact = {
  address: ['address']
}

export const Tag = {
  tag: 'tag',
  fg: ['fg', L.define('#000000')],
  bg: ['bg', L.define('rgba(225, 171, 21, 0.5)')]
}
export const Message = {

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

export const Model = {
  messages: [
    'messages',
    L.define([]),
    L.normalize(R.sortBy(L.get('id')))
  ],
  Message: []
}

window.Model   = Model
window.Message = Message


