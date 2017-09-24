'use strict'
/**********************************************************
 * Astroid - js layer library
 *********************************************************/

// wrap all code in a function as a pseudo module to prevent pollution of the global name space
const AstroidModule = function() {

  /* --------
   * Dependency 'imports'
   * --------
   */
  const { h, Component } = preact
  const hx               = hyperx(h)

  /* --------
   * Helper functions
   * --------
   */
  const hc                = R.curry(h)
  const isBlank           = R.or(R.isNil, R.isEmpty)
  const commaSeparate     = R.intersperse(',')
  const anyValuesAreBlank = R.compose(R.any(isBlank), R.values)
  const BRACKET_OPEN      = '\u003C'
  const BRACKET_CLOSE     = '\u003E'

// get the outer height of an element including its margin
  const outerHeight = el => {
    const style = getComputedStyle(el)
    return el.offsetHeight + parseInt(style.marginTop) + parseInt(style.marginBottom)
  }

  const formatContactWithName = ({ name, address }) => isBlank(name) ? address : `${name} ${BRACKET_OPEN}${address}${BRACKET_CLOSE}`
  const failSafeName          = ({ name, address }) => isBlank(name) ? address : name
  const unlessHeaderBlank     = (component, args) => R.unless(anyValuesAreBlank, hc(component), args)

  /* --------
   * UI Components
   * --------
   */

  const NameAddressLink = ({ name, address }) => hx`
  <a href="mailto:${formatContactWithName({ name, address })}">
    <strong>${failSafeName({ name, address })}</strong>
${BRACKET_OPEN}${address}${BRACKET_CLOSE}</a>`

  const HeaderAddressList = ({ label, contacts, isImportant }) => hx`
  <div class="field ${isImportant && 'important'}" id="${label}">
    <div class="title">${label}:</div>
    <div class="value">
        ${commaSeparate(contacts.map((contact) => h(NameAddressLink, contact)))}
    </div>
  </div>`

  const HeaderDate = ({ label, date }) => hx`
<div class="field important" id="${label}">
    <div class="title">${label}:</div>
    <div class="value">
        <span class="hidden_only">${date.pretty}</span>
        <span class="not_hidden_only">${date.verbose}</span>
    </div>
</div>
`

  const HeaderText = ({ label, value }) => hx`
<div class="field" id="${value}">
    <div class="title">${label}:</div>
    <div class="value">${value}</div>
</div>
`

  const Tag = ({ label }) => hx`
<span style="background-color: rgba(225, 171, 21, 0.5); color: #000000ff !important; white-space: pre;">${label}</span>
`

  const HeaderTags = ({ label, tags }) => hx`<div class="field noprint" id="${label}">
    <div class="title">${label}:</div>
    <div class="value tags-list">
      ${tags.map((tag) => h(Tag, tag))}
    </div>
</div>
`

  const EmailHeaders = ({ message }) => hx`<div class="header">
  ${unlessHeaderBlank(HeaderAddressList,
    { label: 'From', contacts: [message.from], isImportant: true })}
  ${unlessHeaderBlank(HeaderAddressList, { label: 'To', contacts: message.to })}
  ${unlessHeaderBlank(HeaderAddressList, { label: 'Cc', contacts: message.cc })}
  ${unlessHeaderBlank(HeaderDate, { label: 'Date', date: message.date })}
  ${unlessHeaderBlank(HeaderText, { label: 'Subject', value: message.subject })}
  ${unlessHeaderBlank(HeaderTags, { label: 'Tags', tags: message.tags })}
</div>
`
  const Gravatar     = ({ gravatar }) => R.unless(isBlank, () => hx`<img src="${gravatar}" class="avatar" />`, gravatar)

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
      preact.render(h(wrappedComponent, Object.assign({}, this.props)/*{ ... this.props }*/), this.frameBody, this.contentElement)
      // const iframe = ReactDOM.findDOMNode(this)
      const iframe = this.base
      afterRenderHook(iframe, this.frameBody)
    }

    render({ message }) {
      return hx`<iframe id="message-iframe message-part-${message.id}"
      sandbox="allow-same-origin allow-top-navigation allow-popups allow-popups-to-escape-sandbox"
        seamless class="message-part-html" target="_blank"></iframe>
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
      this.updateIFrameContents()
    }
  }

  const EmailBodyDangerous = ({ message }) => {
    // see https://github.com/developit/preact/commit/a9f28e66b4f34a8fa8621b3a0350df2754fa5308
    return h('div', { dangerouslySetInnerHTML: { __html: message.body } })
  }

  const EmailBodySandboxed = SandboxedComponent(EmailBodyDangerous, (frame, frameBody) => {

    // rewrite all links to open in a a new window
    frameBody.querySelectorAll('a').forEach((a, i) => {
      if (a.href.indexOf('mailto:') === 0) {
        a.href = ('/message/compose/?to=' + a.href.substring(7))
      }
      else {
        a.setAttribute('target', '_blank')
      }
    })

    // adjust the height of the frame to be the height of the content
    frame.style.height = `${(outerHeight(frame.contentDocument.body))}px`
  })

  const emailClass = (message) => message.focused ? 'focused' : 'hide'

  const EmailMessage = ({ message }) => hx`
<div class="email ${emailClass(message)}">
    <div class="compressed_note"></div>
    <div class="geary_spacer"></div>
    <div class="email_container">
        <div class="email_warning"></div>
        <div class="email_info"></div>
        <div class="header_container">
            ${h(Gravatar, message)}
            <div class="button_bar">
            </div>
            <img class="attachment icon first" />
            <img class="marked icon first" />

            ${h(EmailHeaders, { message })}

            <img class="attachment icon sec" />
            <img src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACMAAAAjCAYAAAAe2bNZAAAABHNCSVQICAgIfAhkiAAAAYJJREFUWIXt1j9KA0EUBvBvnCK7B9BGEOz1CknIaBCsvYCdkMLCws46RVorPYEKgrCS4iUkxZ5AbSwEWy+wkMRn48qy7PzZzUZB5itn3g4/3rzdBPDx8fH5B4njOCSiTVvd2qohURQ1kiS5EULEk8lk21QrVg0JguAOwOH30ruUst1qtd5+FVMASaMFrQRjgBhBtc9MHMdhGIYPBggAbC0Wi6P8oqwTEkVRA8AtgANTHTMPlFIX+XXnzhBRdzQaHev2HTsCZu4rpc6K9pwwRNQVQtwDuCKik/x++voy874FMlBKnev2rQOcgYTpmQB6nU7nMoVYhjULKeyIE6YA8nM2gF6SJNd1QYyY8XjcZuZHAIGm5BPAC4AdC6RvuppstDMjpXxm5lfLszaIcUbyMV7TdDpdn81mJITYdT0wB7FejTOmKqgKxAlTFlQV4oxxBS0DKYWxgZaFACV/KJvN5sd8Pt8D8JSDaD/xZVLpL0S2Q3V0ZOkMh8MNIjr9U4SPj48mX+Wi1A+9j/jxAAAAAElFTkSuQmCC" class="marked icon sec">
            <div class="tags tags-list">
              ${message.tags.map((tag) => h(Tag, tag))}
            </div>
            <div class="subject">${message.subject}</div>
            <div class="preview">${message.preview}</div>
        </div>
        <div class="remote_images"><img class="close_show_images button" /></div>
        <div class="body">
          <span class="body_part">${h(EmailBodySandboxed, { message })}</span>
          <div class="sibling_container" id="3">
            <div class="message">Alternative part (type: text/html) - potentially sketchy.</div>
          </div>
        </div>
        <div class="draft_edit"><span class="draft_edit_button button"></span></div>
    </div>
</div>
  `

  const EmailView = (props) => hx`
  <div id="message_container">
  ${console.log(props)}
  ${props.messages.map((message) => h(EmailMessage, { message }))}
  </div>
  `

  /* --------
   * State management
   * --------
   */
  const State = {
    messages: []
  }

  /* --------
   * Public API
   * --------
   */

  /** @module Astroid */
  window['Astroid'] = {

    /**
     * @typedef {{
     *            id:string,
     *            from: Contact,
     *            foobar:number|string
     *          }}
     */

    /**
     * Initialize the Astroid System and UI
     */
    init() {
      this.render()
    },

    /**
     * Force a re-render of the UI
     */
    render() {
      preact.render(h(EmailView, State), document.body, document.body.lastChild)
    },

    /**
     * Add a message. If it already exists, it will be updated.
     * @param {Message} message - the email message to add
     * @return {void}
     */
    add_message(message) {
      State.messages = R.append(message, State.messages)
      this.render()
    },

    /**
     * Clears all messages
     * @return {void}
     */
    clear_messages() {
      State.messages = []
      this.render()
    }
  }
}()

