import preact, { Component } from 'preact'
import * as R from 'ramda/index-es'
import * as L from 'partial.lenses'
import * as U from 'karet.util'
import { k, kx } from './ui'
import { Message, Tag as TagModel } from './model'
import './thread-view.scss'

/* --------
 * Helper functions
 * --------
 */

// const isBlank           = (v) => U.or(U.isEmpty(v), U.isNil(v))
// const isBlank           = U.either(U.isEmpty(U.__), U.isNil(U.__))
const isBlank       = U.either(U.isEmpty, U.isNil)
const commaSeparate = U.intersperse(',')
const BRACKET_OPEN  = '\u003C'
const BRACKET_CLOSE = '\u003E'

// get the outer height of an element including its margin
const outerHeight = el => {
  const style = getComputedStyle(el)
  return el.offsetHeight + parseInt(style.marginTop) + parseInt(style.marginBottom)
}

const formatContactWithName = ({ name, address }) => U.ifte(isBlank(name), address, `${name} ${BRACKET_OPEN}${address}${BRACKET_CLOSE}`)
const failSafeName          = ({ name, address }) => U.ifte(isBlank(name), address, name)
const areAnyPropsBlank      = U.compose(U.any(isBlank), R.values)
const areAllPropsNotBlank   = U.compose(U.not, areAnyPropsBlank)
const unlessBlank           = (component, args) => U.ift(areAllPropsNotBlank(args), k(component, args))

/* --------
 * UI Components
 * --------
 */

const NameAddressLink = ({ contact }) => {
  const address = U.view('address', contact)
  const name    = U.view('name', contact)
  return kx`
   <a href="mailto:${formatContactWithName({ name, address })}">
     <strong>${failSafeName({ name, address })}</strong>
 ${BRACKET_OPEN}${address}${BRACKET_CLOSE}</a>`
}

const HeaderAddressList = ({ label, contacts, isImportant }) => kx`
<div className="${R.concat('field', isImportant ? 'important' : '')}" id="${label}">
  <div className="title">${label}:</div>
  <div className="value">
    ${
  U.seq(contacts, U.mapElems((contact, i) => k(NameAddressLink, { key: i, contact })),
    commaSeparate)}
  </div>
</div>`

const HeaderDate = ({ label, date }) => kx`
<div class="field important" id="${label}">
    <div class="title">${label}:</div>
    <div class="value">
        <span class="hidden_only">${U.view('pretty', date)}</span>
        <span class="not_hidden_only">${U.view('verbose', date)}</span>
    </div>
</div>
`

const HeaderText = ({ label, value }) => kx`
<div class="field" id="${value}">
    <div class="title">${label}:</div>
    <div class="value">${value}</div>
</div>
`

const Tag = ({ tag }) => {
  const style = {
    backgroundColor: U.view(TagModel.bg, tag),
    color: U.view(TagModel.fg, tag),
    whiteSpace: 'pre'
  }
  return kx`<span style=${style}>${U.view(TagModel.tag, tag)}</span>`
}

const HeaderTags = ({ label, tags }) => kx`<div class="field noprint" id="${label}">
    <div class="title">${label}:</div>
    <div class="value tags-list">
      ${U.seq(tags, U.mapElems((tag, i) => k(Tag, { tag, key: i })))}
    </div>
</div>
`

const EmailHeaders = ({ message }) => kx`
<div class="header">
  ${unlessBlank(HeaderAddressList, { label: 'From', contacts: U.view(Message.from, message) })}
  ${unlessBlank(HeaderAddressList, { label: 'To', contacts: U.view(Message.to, message) })}
  ${unlessBlank(HeaderAddressList, { label: 'Cc', contacts: U.view(Message.cc, message) })}
  ${unlessBlank(HeaderAddressList, { label: 'Bcc', contacts: U.view(Message.bcc, message) })}
  ${unlessBlank(HeaderDate, { label: 'Date', date: U.view(Message.date, message) })}
  ${unlessBlank(HeaderText, { label: 'Subject', value: U.view(Message.subject, message) })}
  ${unlessBlank(HeaderTags, { label: 'Tags', tags: U.view(Message.tags, message) })}
</div>
`

const Gravatar = ({ gravatar }) => kx`<img src=${gravatar} class="avatar" />`

// this is a higher-order component that wraps a component and renders it into a sandboxed iframe
// the afterRenderHook is called after the content has been rendered
const SandboxedComponent = (wrappedComponent, afterRenderHook) => class extends Component {

  get frameDocument() {
    // return ReactDOM.findDOMNode(this).contentDocument
    return this.base.contentDocument
  }

  get frameBody() {
    return this.frameDocument.body
  }

  updateIFrameContents() {
    preact.render(k(wrappedComponent, { ...this.props }), this.frameBody, this.contentElement)
    afterRenderHook(this.base, this.frameBody)
  }

  render({ iframeId }) {
    return kx`<iframe class="message-iframe message-part-ID message-part-html" id=${iframeId}
      sandbox="allow-same-origin allow-top-navigation allow-popups allow-popups-to-escape-sandbox"
        seamless target="_blank"></iframe>
      `
  }

  componentDidMount() {
    this.contentElement = document.createElement('div')
    this.frameBody.appendChild(this.contentElement)
    const cssLink = Object.assign(document.createElement('style'),
      { innerText: window.Astroid.emailBodyCSS, type: 'text/css' })
    this.frameDocument.head.appendChild(cssLink)
    this.updateIFrameContents()
  }

  componentDidUpdate() {
    if(this.frameDocument)
      this.updateIFrameContents()
  }
}

const EmailBodyDangerous = ({ bodyContent }) => {
  // see https://github.com/developit/preact/commit/a9f28e66b4f34a8fa8621b3a0350df2754fa5308
  return k('div', { id: 'content', dangerouslySetInnerHTML: U.template({ __html: bodyContent }) })
}

const EmailBodySandboxed = SandboxedComponent(EmailBodyDangerous, (frame, frameBody) => {

  // rewrite all links to open in a a new window
  frameBody.querySelectorAll('a').forEach((a) => {
    if (a.href.indexOf('mailto:') === 0) {
      a.href = ('/message/compose/?to=' + a.href.substring(7))
    }
    else {
      a.setAttribute('target', '_blank')
    }
  })

  setTimeout(() => {
    // adjust the height of the frame to be the height of the content
    frame.style.height = `${(outerHeight(frame.contentDocument.body))}px`
  }, 100)
})

const AlternativePart = ({ body }) => {
  const mime_type = U.view('mime_type', body)

  const msg = U.ift(U.seq(mime_type, U.toLower, U.contains('html')), ' - potentially sketchy')
  return kx`
<div class="sibling_container" id=${U.view('eid', body)}>
  <button class="message">Alternative part (type: ${mime_type})${msg}</button>
</div>
`
}

const BodySection = ({ message }) => {
  // the body parts are sorted with the preferred at the head of the list
  // if there is no preferred we take the first one anyways
  const bodyParts    = U.view(Message.body, message)
  const selectedBody = U.head(bodyParts)
  const bodyContent  = U.view('content', selectedBody)
  const bodyEid      = U.view('eid', selectedBody)
  const alternatives = U.tail(bodyParts)
  return kx`
  <div class="body">
      <span class="body_part">
          ${k(EmailBodySandboxed, { bodyContent, iframeId: bodyEid })}
      </span>
      ${U.seq(alternatives, U.mapElems(alt => k(AlternativePart, { body: alt })))}
  </div>
`
}

// focused hide
const emailClass = (elements, focused, message) => U.string`email ${U.ifte(U.equals(U.view('mid', focused), U.view('id', message)), 'focused', 'hide')}`

const EmailMessage = ({ message, focused, elements }) => {
  const gravatar = U.view(Message.gravatar, message)
  return kx`
<div class=${emailClass(elements, focused, message)}>
  <div class="compressed_note"></div>
  <div class="geary_spacer"></div>
  <div class="email_container">
    <div class="email_warning"></div>
    <div class="email_info"></div>
    <div class="header_container">
      ${U.ift(true, k(Gravatar, { gravatar }))}
      <div class="button_bar"></div>
      <img class="attachment icon first"/>
      <img class="marked icon first"/>
      
      ${k(EmailHeaders, { message })}

      <img class="attachment icon sec"/>
      <img
        src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACMAAAAjCAYAAAAe2bNZAAAABHNCSVQICAgIfAhkiAAAAYJJREFUWIXt1j9KA0EUBvBvnCK7B9BGEOz1CknIaBCsvYCdkMLCws46RVorPYEKgrCS4iUkxZ5AbSwEWy+wkMRn48qy7PzZzUZB5itn3g4/3rzdBPDx8fH5B4njOCSiTVvd2qohURQ1kiS5EULEk8lk21QrVg0JguAOwOH30ruUst1qtd5+FVMASaMFrQRjgBhBtc9MHMdhGIYPBggAbC0Wi6P8oqwTEkVRA8AtgANTHTMPlFIX+XXnzhBRdzQaHev2HTsCZu4rpc6K9pwwRNQVQtwDuCKik/x++voy874FMlBKnev2rQOcgYTpmQB6nU7nMoVYhjULKeyIE6YA8nM2gF6SJNd1QYyY8XjcZuZHAIGm5BPAC4AdC6RvuppstDMjpXxm5lfLszaIcUbyMV7TdDpdn81mJITYdT0wB7FejTOmKqgKxAlTFlQV4oxxBS0DKYWxgZaFACV/KJvN5sd8Pt8D8JSDaD/xZVLpL0S2Q3V0ZOkMh8MNIjr9U4SPj48mX+Wi1A+9j/jxAAAAAElFTkSuQmCC"
        class="marked icon sec">
      <div class="tags tags-list">
        ${U.seq(U.view(Message.tags, message), U.mapElems((tag, i) => k(Tag, { tag, key: i })))}
      </div>
      <div class="subject">${U.view(Message.subject, message)}</div>
      <div class="preview">${U.view(Message.preview, message)}</div>
    </div>
    <div class="remote_images"><img class="close_show_images button"/></div>
      ${k(BodySection, { message })}
    <div class="draft_edit"><span class="draft_edit_button button"></span></div>
  </div>
</div>
  `
}

export const EmailView = ({ messages, elements, focused }) => {
  return kx`
<div id="message_container">
  ${
    U.seq(messages, U.mapElems((message) => k(EmailMessage, {
      message, focused, elements, key: U.view(Message.id, message)
    })))}
</div>
  `
}

