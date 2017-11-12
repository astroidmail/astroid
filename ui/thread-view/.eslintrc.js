module.exports = {
  'globals': {
    'process': true
  },
  'env': {
    'browser': true,
    'es6': true
  },
  'extends': 'eslint:recommended',
  'parserOptions': {
    'sourceType': 'module',
    'ecmaFeatures': {
      'experimentalObjectRestSpread': true
    }
  },

  'rules': {
    'indent': [
      'error',
      2,
      {
        'MemberExpression': 1,
        'ObjectExpression': 'first'
      }

    ],
    'linebreak-style': [
      'error',
      'unix'
    ],
    'quotes': [
      'error',
      'single'
    ],
    'semi': [
      'error',
      'never'
    ]
  }
}
