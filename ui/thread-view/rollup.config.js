import alias from 'rollup-plugin-alias'
import babel from 'rollup-plugin-babel'
import commonjs from 'rollup-plugin-commonjs'
import eslint from 'rollup-plugin-eslint'
import execute from 'rollup-plugin-execute'
import livereload from 'rollup-plugin-livereload'
import nodeResolve from 'rollup-plugin-node-resolve'
import path from 'path'
import replace from 'rollup-plugin-replace'
import scss from 'rollup-plugin-scss'
import serve from 'rollup-plugin-serve'
import uglify from 'rollup-plugin-uglify'


const production = !process.env.ROLLUP_WATCH

export default {
  watch: {
    chokidar: true,
    include: 'src/*'
  },
  globals: {
    react: ['React', 'Component', 'createElement']
  },
  plugins: [

    eslint({ exclude: ['src/**.scss']}),
    process.env.NODE_ENV && replace({ 'process.env.NODE_ENV': JSON.stringify(process.env.NODE_ENV) }),
    alias({
      'react': path.resolve(__dirname, 'node_modules', 'preact-compat', 'dist', 'preact-compat.es.js'),
      'react-dom': path.resolve(__dirname, 'node_modules', 'preact-compat', 'dist', 'preact-compat.es.js'),
      'create-react-class': path.resolve(__dirname, 'node_modules', 'preact-compat', 'lib', 'create-react-class.js')
    }),

    babel({
      exclude: [
        'src/**.scss',
        'node_modules/!(' +
        'google-map-react|preact|preact-compat|react-redux' +
        ')/**'
      ]
    }),

    nodeResolve(),

    commonjs({
      include: 'node_modules/**',
      namedExports: {
        'node_modules/react-dom/index.js': [
          'render'
        ],
        /*
        "node_modules/react/react.js": [
          "Component",
          "createElement"
        ],
        */
        'node_modules/preact/src/preact.js': [
          'h',
          'createElement',
          'cloneElement',
          'Component',
          'render',
          'rerender',
          'options'
        ],
        'node_modules/kefir/dist/kefir.js': [
          'Observable',
          'Property',
          'Stream',
          'combine',
          'concat',
          'constant',
          'fromEvents',
          'interval',
          'later',
          'merge',
          'never'
        ]
      }
    }),

    process.env.NODE_ENV === 'production' && uglify({
      compress: {
        passes: 3
      }
    }),

    // collectSass({extract: 'dist/thread-view.css'}),

    scss({
      output: 'dist/thread-view.css'
    }),
    //!production && execute('cp src/dev.html dist/dev.html && cp src/stub.js dist/stub.js'),
    execute('cp src/index.html dist/index.html'),

    !production && serve('dist'),
    !production && livereload('dist')

  ].filter(x => x)
}
