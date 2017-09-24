/**
 * Test system used to mock the C++ layer
 */
window['AstroidStubs'] = {
  init() {
    this.add_messages()
  },

  add_messages() {
    Astroid.add_message({
      id: 'CADYYYYYYYYYYYYYYhwHMvKwKXQsu-vn77777777777SuU_khq13FAxA@mail.gmail.com',
      from: { address: 'alice@example.com', name: 'Alice' },
      to: [{ address: 'bob@example.com' }, { address: 'mary@example.com', name: 'Mary Jane' }],
      cc: [{ name: 'The Real Joe', address: 'joe@example.io' }, {
        address: 'stallman@example.com',
        name: 'Richard Stallman'
      }],
      date: { pretty: 'Jun 10', verbose: 'June 10, 2017 3:00 pm', timestamp: '' },
      subject: 'Hello there',
      gravatar: 'https://www.gravatar.com/avatar/b653535c5a4f7ea4c6f1a97dfe0ce670?d=retro&amp;s=48',

      tags: [{ label: 'signed' }, { label: 'inbox' }],

      focused: true,
      missing_content: false,

      preview: 'Hello...',
      body: `
Thanks for telling me about this!<br><br>On Saturday, 10 Aug 2017 10:00:00 UTC gauteh wrote:<br>
<blockquote class="level_001">
  <p><em>Astroid</em> is a lightweight and fast <strong>Mail User Agent</strong> that provides a
                      graphical interface to searching, displaying and composing email, organized in
                      threads and tags. <em>Astroid</em> uses the <a href="http://notmuchmail.org/">notmuch</a>
                      backend for blazingly fast searches through tons of email. <em>Astroid</em>
                      searches, displays and composes emails - and rely on other programs for
                      fetching, syncing and sending email. Check out <a
      href="https://github.com/astroidmail/astroid/wiki/Astroid-in-your-general-mail-setup">Astroid
                                                                                            in your
                                                                                            general
                                                                                            mail
                                                                                            setup</a>
                      for a suggested complete e-mail solution.</p>
</blockquote>
<script>alert('I should not appear')</script>
      `
    })

    Astroid.add_message({
      id: 'CADVXXXXXXXXXXXXXxwHMvKwKXQsu-vn99999999999U_khq13FAxA@mail.gmail.com',

      from: { address: 'alice@example.com', name: 'Alice' },
      to: [{ address: 'bob@example.com', name: 'Bob' }],
      date: { pretty: 'Jun 10', verbose: 'June 10, 2017 3:00 pm', timestamp: '' },
      subject: 'Re: Hello there',
      gravatar: 'https://www.gravatar.com/avatar/1eff04c3b5ba7684e7def095ae51975b?d=retro&amp;s=48',

      tags: [{ label: 'inbox' }],

      focused: false,
      missing_content: true,

      preview: 'Hello...',
      body: 'Hello there how are you'

    })
  }

}
