ace.define("ace/mode/npp_highlight_rules", [
  "require", "exports", "module", "ace/lib/oop", "ace/mode/text_highlight_rules"
], function(require, exports, module) {
  "use strict";
  const oop = require("ace/lib/oop");
  const TextHighlightRules = require("ace/mode/text_highlight_rules").TextHighlightRules;

  const NppHighlightRules = function() {
    this.$rules = {
      start: [
        { token: "comment", regex: "//.*$" },
        { token: "string", regex: '"(?:\\\\.|[^"\\\\])*"' },
        { token: "string", regex: "'(?:\\\\.|[^'\\\\])*'" },
        { token: "constant.numeric", regex: "\\b(?:0[xX][0-9a-fA-F]+|[0-9]+(?:\\.[0-9]+)?)\\b" },
        { token: "constant.language", regex: "\\b(?:true|false|null|none)\\b" },
        { token: "keyword", regex: "\\b(?:fn|let|mut|const|if|else|while|for|in|return|break|continue|match|struct|enum|import|as|pub|private|extern|type|impl|trait|self)\\b" },
        { token: "support.type", regex: "\\b(?:i64|i32|i16|i8|u64|u32|u16|u8|usize|isize|f64|f32|bool|string|str|char|void|Result|Option|Vec)\\b" },
        { token: "support.function", regex: "\\b(?:print|input|str_i64|len|assert|panic|json_[A-Za-z0-9_]+|http_get|http_get_text|gfx_[A-Za-z0-9_]+)\\b" },
        { token: "keyword.operator", regex: "===?|!==?|==|!=|<=|>=|=>|->|\\.\\.|\\+|-|\\*|/|%|&&|\\|\\||!|=|<|>" },
        { token: "paren.lparen", regex: "[({\\[]" },
        { token: "paren.rparen", regex: "[)}\\]]" },
        { token: "punctuation.operator", regex: "[;,.:]" },
        { token: "identifier", regex: "[A-Za-z_$][A-Za-z0-9_$]*" },
        { token: "text", regex: "\\s+" }
      ]
    };
    this.normalizeRules();
  };
  oop.inherits(NppHighlightRules, TextHighlightRules);
  exports.NppHighlightRules = NppHighlightRules;
});

ace.define("ace/mode/npp", [
  "require", "exports", "module", "ace/lib/oop", "ace/mode/text", "ace/mode/npp_highlight_rules"
], function(require, exports, module) {
  "use strict";
  const oop = require("ace/lib/oop");
  const TextMode = require("ace/mode/text").Mode;
  const Mode = function() {
    this.HighlightRules = require("ace/mode/npp_highlight_rules").NppHighlightRules;
    this.lineCommentStart = "//";
  };
  oop.inherits(Mode, TextMode);
  exports.Mode = Mode;
});

// NPP Studio theme: near-black editor with a restrained yellow syntax accent.
ace.define("ace/theme/npp", [
  "require", "exports", "module", "ace/lib/dom"
], function(require, exports, module) {
  "use strict";
  exports.isDark = true;
  exports.cssClass = "ace-npp";
  exports.cssText = [
    ".ace-npp { color: #e8e9e1; background-color: #111411; }",
    ".ace-npp .ace_gutter { background: #171d18; color: #788477; border-right: 1px solid #2a342d; }",
    ".ace-npp .ace_print-margin { width: 1px; background: #28332a; }",
    ".ace-npp .ace_cursor { color: #d9ad55; border-left: 2px solid #d9ad55; }",
    ".ace-npp .ace_marker-layer .ace_active-line { background: #20271f; }",
    ".ace-npp .ace_gutter-active-line { background: #20271f; }",
    ".ace-npp .ace_marker-layer .ace_selection { background: #3c3b2b; }",
    ".ace-npp .ace_keyword, .ace-npp .ace_storage { color: #d9ad55; font-weight: 600; }",
    ".ace-npp .ace_constant.ace_numeric, .ace-npp .ace_constant.ace_language { color: #c7ab6c; }",
    ".ace-npp .ace_string { color: #a8c199; }",
    ".ace-npp .ace_comment { color: #798679; font-style: italic; }",
    ".ace-npp .ace_support.ace_function { color: #e0bd71; }",
    ".ace-npp .ace_support.ace_type, .ace-npp .ace_storage.ace_type { color: #a4b5a4; }",
    ".ace-npp .ace_paren, .ace-npp .ace_punctuation { color: #bdc8b9; }",
    ".ace-npp .ace_indent-guide { background: linear-gradient(to bottom, transparent, transparent 50%, #2c382d 50%, #252619); background-size: 1px 4px; }",
    ".ace-npp .ace_invisible { color: #566256; }"
  ].join("\n");
  const dom = require("ace/lib/dom");
  dom.importCssString(exports.cssText, exports.cssClass);
});

