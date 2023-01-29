import 'reveal.js/dist/reset.css';
import 'reveal.js/dist/reveal.css';
import 'reveal.js/dist/theme/moon.css';
import Reveal from 'reveal.js';
import Highlight from 'reveal.js/plugin/highlight/highlight.js';
import Markdown from 'reveal.js/plugin/markdown/markdown.esm.js';

let deck = new Reveal({
  plugins: [Highlight, Markdown],
})
deck.initialize();
